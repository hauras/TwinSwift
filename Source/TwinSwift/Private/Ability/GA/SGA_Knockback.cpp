#include "Ability/GA/SGA_Knockback.h"

#include "AbilitySystemComponent.h"
#include "SGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"

USGA_Knockback::USGA_Knockback()
{
	// Event.Knockback 이벤트가 오면 발동 (보낸 Payload가 TriggerEventData로 들어옴)
	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = SGameplayTags::Event_Knockback;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);

	// 밀리는 동안 경직: 이동 입력(IsMoveInputBlocked)·회피가 이 태그로 이미 막힘
	ActivationOwnedTags.AddTag(SGameplayTags::State_Stun);

	// 쓰러진 캐릭터는 안 밂
	ActivationBlockedTags.AddTag(SGameplayTags::State_Dead);
}

void USGA_Knockback::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!TriggerEventData
		|| !Character
		|| TriggerEventData->EventMagnitude <= 0.f
		|| !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 하던 공격을 끊음 (공격 몽타주의 루트 모션과 밀림이 겹치지 않게). 이 능력은 빼고
	GetAbilitySystemComponentFromActorInfo()->CancelAbilities(nullptr, nullptr, this);

	// 민 쪽 → 나 (수평만). 민 쪽이 없거나 겹쳐 있으면 캐릭터 뒤쪽
	FVector Direction = FVector::ZeroVector;
	if (const AActor* Instigator = TriggerEventData->Instigator.Get())
	{
		Direction = (Character->GetActorLocation() - Instigator->GetActorLocation()).GetSafeNormal2D();
	}
	if (Direction.IsNearlyZero())
	{
		Direction = -Character->GetActorForwardVector();
	}

	// 민 쪽을 바라보게 → 뒤로 밀리는 모션이 자연스러움
	Character->SetActorRotation((-Direction).Rotation());

	// 능력은 밀림이 끝나면(0.25초) 끝나지만 모션은 끝까지 → 능력이 끝나도 멈추지 않게 (bStopWhenAbilityEnds = false)
	if (KnockbackMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			KnockbackMontage,
			1.f,
			NAME_None,
			false);
		MontageTask->ReadyForActivation();
	}

	// 정해진 시간 동안 일정한 힘으로 밀기 (마찰·입력과 상관없이 데이터에 적은 거리만큼). 끝나면 멈춰 섬 (속도 제한 0)
	UAbilityTask_ApplyRootMotionConstantForce* MoveTask = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
		this,
		NAME_None,
		Direction,
		TriggerEventData->EventMagnitude / KnockbackDuration,
		KnockbackDuration,
		false,
		nullptr,
		ERootMotionFinishVelocityMode::ClampVelocity,
		FVector::ZeroVector,
		0.f,
		true);
	MoveTask->OnFinish.AddDynamic(this, &ThisClass::OnKnockbackFinished);
	MoveTask->ReadyForActivation();
}

void USGA_Knockback::OnKnockbackFinished()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}
