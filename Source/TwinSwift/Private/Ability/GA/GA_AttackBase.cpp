#include "Ability/GA/GA_AttackBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "SGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Ability/SAbilityFunctionLibrary.h"
#include "Ability/SGE_Damage.h"
#include "Animation/AnimInstance.h"
#include "Controller/SPlayerController.h"
#include "GameFramework/Character.h"

UGA_AttackBase::UGA_AttackBase()
{
	InputTag = SGameplayTags::InputTag_Attack;
	DamageEffectClass = USGE_Damage::StaticClass();

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(SGameplayTags::Ability_Warrior_Combo);
	SetAssetTags(AssetTags);

	// 공격 중에는 State.Attacking → 컨트롤러가 이동 입력을 막음 (멈추게 하진 않음)
	ActivationOwnedTags.AddTag(SGameplayTags::State_Attacking);

	ActivationBlockedTags.AddTag(SGameplayTags::State_Stun);
	ActivationBlockedTags.AddTag(SGameplayTags::State_Dead);
}

void UGA_AttackBase::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (ComboSteps.Num() == 0 || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 어느 타부터 칠지: 직전 타가 끝나고 ComboResetTime 안이면 다음 타, 아니면 1타
	CurrentStep = 0;
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastFinishedTime <= ComboResetTime && ComboSteps.IsValidIndex(LastFinishedStep + 1))
	{
		CurrentStep = LastFinishedStep + 1;
	}

	// 판정 이벤트는 능력이 끝날 때까지 계속 받음 (타가 바뀌어도 같은 대기)
	UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		SGameplayTags::Event_Montage_Hit,
		nullptr,
		false,
		true);
	HitTask->EventReceived.AddDynamic(this, &ThisClass::OnHitEvent);
	HitTask->ReadyForActivation();

	PlayCurrentStep();
}

void UGA_AttackBase::PlayCurrentStep()
{
	UAnimMontage* Montage = ComboSteps[CurrentStep].Montage;
	if (!Montage)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	bNextStepQueued = false;
	FaceAimPoint();

	// 이전 타의 태스크를 먼저 정리 → 새 몽타주가 이전 것을 끊어도 "끊김" 콜백이 오지 않음
	if (MontageTask)
	{
		MontageTask->EndTask();
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnStepBlendOut);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnStepBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnStepCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnStepCancelled);
	MontageTask->ReadyForActivation();

	// 다음 타가 있으면 입력 대기. 능력 실행 중에 같은 키를 또 누르면 ASC가 InputPressed 이벤트를 보내 줌
	if (InputTask)
	{
		InputTask->EndTask();
		InputTask = nullptr;
	}
	if (ComboSteps.IsValidIndex(CurrentStep + 1))
	{
		InputTask = UAbilityTask_WaitInputPress::WaitInputPress(this, false);
		InputTask->OnPress.AddDynamic(this, &ThisClass::OnComboInputPressed);
		InputTask->ReadyForActivation();
	}
}

void UGA_AttackBase::OnComboInputPressed(float TimeWaited)
{
	bNextStepQueued = true;
}

void UGA_AttackBase::OnStepBlendOut()
{
	if (IsStaleMontageCallback())
	{
		return;
	}

	if (bNextStepQueued && ComboSteps.IsValidIndex(CurrentStep + 1))
	{
		CurrentStep++;
		PlayCurrentStep();
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_AttackBase::OnStepCancelled()
{
	if (IsStaleMontageCallback())
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_AttackBase::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 정상으로 끝난 타만 콤보로 이어 줌. 끊기거나 취소됐으면 다음엔 1타부터
	LastFinishedStep = bWasCancelled ? INDEX_NONE : CurrentStep;
	LastFinishedTime = GetWorld() ? GetWorld()->GetTimeSeconds() : -1000.0;
	MontageTask = nullptr;
	InputTask = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_AttackBase::OnHitEvent(FGameplayEventData Payload)
{
	AActor* Target = const_cast<AActor*>(Payload.Target.Get());
	if (!Target || USAbilityFunctionLibrary::IsFriend(GetAvatarActorFromActorInfo(), Target))
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetASC || !DamageEffectClass)
	{
		return;
	}

	const FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
	if (!Spec.IsValid())
	{
		return;
	}

	Spec.Data->SetSetByCallerMagnitude(SGameplayTags::SetByCaller_DamageMultiplier, ComboSteps[CurrentStep].DamageMultiplier);
	GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
}

bool UGA_AttackBase::IsStaleMontageCallback() const
{
	// 다음 타로 넘어간 뒤, 이전 타 몽타주가 블렌드 아웃을 마치며 늦게 보내는 콜백이 있음
	// 그때 현재 타 몽타주는 아직 정상 재생 중 → 이전 타의 콜백이므로 무시 (안 하면 한 타를 건너뜀)
	const UAnimInstance* AnimInstance = CurrentActorInfo ? CurrentActorInfo->GetAnimInstance() : nullptr;
	const UAnimMontage* StepMontage = ComboSteps.IsValidIndex(CurrentStep) ? ComboSteps[CurrentStep].Montage.Get() : nullptr;
	return AnimInstance && StepMontage && AnimInstance->Montage_IsPlaying(StepMontage);
}

void UGA_AttackBase::FaceAimPoint() const
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	const ASPlayerController* PlayerController = Character ? Cast<ASPlayerController>(Character->GetController()) : nullptr;
	if (!PlayerController)
	{
		return;
	}

	const FVector ToAim = PlayerController->GetAimPoint() - Character->GetActorLocation();
	if (ToAim.SizeSquared2D() < 1.f)
	{
		return;
	}

	Character->SetActorRotation(FRotator(0.f, ToAim.Rotation().Yaw, 0.f));
}
