#include "Ability/GA/GA_AttackBase.h"

#include "AbilitySystemComponent.h"
#include "SGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Animation/AnimInstance.h"

UGA_AttackBase::UGA_AttackBase()
{
	InputTag = SGameplayTags::InputTag_Attack;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(SGameplayTags::Ability_Warrior_Combo);
	SetAssetTags(AssetTags);

	// 공격 중에는 State.Attacking → 컨트롤러가 이동 입력을 막음 (멈추게 하진 않음)
	ActivationOwnedTags.AddTag(SGameplayTags::State_Attacking);

	ActivationBlockedTags.AddTag(SGameplayTags::State_Stun);
	ActivationBlockedTags.AddTag(SGameplayTags::State_Dead);

	// 스킬 시전 중(State.Attacking)에는 기본 공격이 새로 나가지 않음
	// 안 막으면 기본 공격 몽타주가 스킬 몽타주를 끊어서 스킬은 쿨다운만 돌고 효과가 안 나감
	// (콤보 다음 타는 이미 돌고 있는 이 능력 안에서 입력을 받으므로 막히지 않음)
	ActivationBlockedTags.AddTag(SGameplayTags::State_Attacking);
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

	// 타마다 열리는 Window.Combo 창도 능력이 끝날 때까지 계속 받음 (한 번만 받고 끝내지 않음)
	UAbilityTask_WaitGameplayTagAdded* ComboWindowTask = UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(
		this,
		SGameplayTags::Window_Combo,
		nullptr,
		false);
	ComboWindowTask->Added.AddDynamic(this, &ThisClass::OnComboWindowOpened);
	ComboWindowTask->ReadyForActivation();

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

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		Montage,
		ComboSteps[CurrentStep].PlayRate);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnStepBlendOut);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnStepBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnStepCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnStepCancelled);
	MontageTask->ReadyForActivation();

	WaitComboInput();
}

void UGA_AttackBase::WaitComboInput()
{
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
	NextStepQueuedTime = GetWorld()->GetRealTimeSeconds();

	// 창이 이미 열려 있으면 바로 다음 타 (창이 아직이면 열리는 순간 OnComboWindowOpened에서)
	const int32 StepBefore = CurrentStep;
	TryAdvanceCombo();

	// 같은 타에 머물렀으면 다시 받을 준비 (WaitInputPress는 한 번 받으면 끝남)
	// 안 하면 일찍 누른 예약이 0.3초 뒤 만료된 다음 다시 눌러도 받을 곳이 없어 다음 타가 안 나감
	// 다음 타로 넘어갔으면 PlayCurrentStep이 새 타의 대기를 이미 등록함
	if (CurrentStep == StepBefore && IsActive())
	{
		WaitComboInput();
	}
}

void UGA_AttackBase::OnComboWindowOpened()
{
	TryAdvanceCombo();
}

bool UGA_AttackBase::HasValidQueue()
{
	if (!bNextStepQueued)
	{
		return false;
	}

	// 실제 시간 기준 (완벽 회피 슬로 중에도 0.3초가 늘지 않게, 입력 버퍼와 같음)
	if (GetWorld()->GetRealTimeSeconds() - NextStepQueuedTime > ComboInputBufferTime)
	{
		bNextStepQueued = false;
		return false;
	}
	return true;
}

void UGA_AttackBase::TryAdvanceCombo()
{
	if (!HasValidQueue() || !ComboSteps.IsValidIndex(CurrentStep + 1))
	{
		return;
	}

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || !ASC->HasMatchingGameplayTag(SGameplayTags::Window_Combo))
	{
		return;
	}

	// 후딜을 끊고 다음 타. PlayCurrentStep이 예약을 지우므로, 이전 몽타주의 창이 닫히며 다시 불려도 두 타를 건너뛰지 않음
	CurrentStep++;
	PlayCurrentStep();
}

void UGA_AttackBase::OnStepBlendOut()
{
	if (IsStaleMontageCallback())
	{
		return;
	}

	if (HasValidQueue() && ComboSteps.IsValidIndex(CurrentStep + 1))
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
	// 같은 편 무시·GE 적용은 부모(USGameplayAbility)의 공통 도우미가 함
	ApplyDamageTo(const_cast<AActor*>(Payload.Target.Get()), ComboSteps[CurrentStep].DamageMultiplier);
}

bool UGA_AttackBase::IsStaleMontageCallback() const
{
	// 다음 타로 넘어간 뒤, 이전 타 몽타주가 블렌드 아웃을 마치며 늦게 보내는 콜백이 있음
	// 그때 현재 타 몽타주는 아직 정상 재생 중 → 이전 타의 콜백이므로 무시 (안 하면 한 타를 건너뜀)
	const UAnimInstance* AnimInstance = CurrentActorInfo ? CurrentActorInfo->GetAnimInstance() : nullptr;
	const UAnimMontage* StepMontage = ComboSteps.IsValidIndex(CurrentStep) ? ComboSteps[CurrentStep].Montage.Get() : nullptr;
	return AnimInstance && StepMontage && AnimInstance->Montage_IsPlaying(StepMontage);
}
