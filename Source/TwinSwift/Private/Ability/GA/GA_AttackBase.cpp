#include "Ability/GA/GA_AttackBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "SGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Ability/SAbilityFunctionLibrary.h"
#include "Ability/SGE_Damage.h"
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
	if (!AttackMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FaceAimPoint();

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		AttackMontage);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageBlendOut);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		SGameplayTags::Event_Montage_Hit,
		nullptr,
		false,
		true);
	HitTask->EventReceived.AddDynamic(this, &ThisClass::OnHitEvent);
	HitTask->ReadyForActivation();
}

void UGA_AttackBase::OnMontageBlendOut()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_AttackBase::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
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

	Spec.Data->SetSetByCallerMagnitude(SGameplayTags::SetByCaller_DamageMultiplier, DamageMultiplier);
	GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
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
