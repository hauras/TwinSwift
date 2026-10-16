#include "Ability/GA/SGA_CastAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "SGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Ability/SGE_Cooldown.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraFunctionLibrary.h"

namespace
{
	// 손 앞쪽으로 조금 띄워서 시작 (손 안에서 생기면 몸에 바로 닿아 보임)
	constexpr float MuzzleForwardOffset = 20.f;

	// 손 → 커서 거리가 이보다 짧으면 몸 가운데 → 커서 방향을 씀 (cm)
	constexpr float MinHandAimDistance = 150.f;
}

USGA_CastAbility::USGA_CastAbility()
{
	ActivationBlockedTags.AddTag(SGameplayTags::State_Stun);
	ActivationBlockedTags.AddTag(SGameplayTags::State_Dead);

	// 화염탄 중에 다른 시전을 누르면 화염탄을 끊고 시전 (화염탄끼리는 이미 돌고 있으면 새로 발동하지 않음)
	CancelAbilitiesWithTag.AddTag(SGameplayTags::Ability_Mage_FireBolt);
}

void USGA_CastAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 능력 인스턴스는 재사용되므로 매번 처음 상태로
	bCastFired = false;
	bRecoveryDone = false;

	// 시전 중에는 이동 입력을 막음. 시전 후 효과가 오래가도 RecoveryTime 뒤에는 풀려야 해서 ActivationOwnedTags 대신 직접 붙임
	SetAttacking(true);

	FaceAimPoint();

	if (!CastMontage)
	{
		OnCastEvent(FGameplayEventData());
		return;
	}

	// 손을 뻗는 프레임의 신호. 몽타주보다 먼저 기다리기 시작해야 놓치지 않음
	UAbilityTask_WaitGameplayEvent* CastTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		SGameplayTags::Event_Montage_Hit);
	CastTask->EventReceived.AddDynamic(this, &ThisClass::OnCastEvent);
	CastTask->ReadyForActivation();

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		CastMontage,
		MontagePlayRate,
		NAME_None,
		true,
		1.f,
		MontageStartTime);
	// 1차 교훈: 끝나기 시작(BlendOut)·끊김·취소 모두 종료 경로로
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageBlendOut);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->ReadyForActivation();
}

void USGA_CastAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 취소·끊김으로 끝나도 이동 막기가 남지 않게
	SetAttacking(false);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USGA_CastAbility::TryFinishCast()
{
	if (bRecoveryDone && IsCastFinished() && IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

TArray<FGameplayEffectSpecHandle> USGA_CastAbility::MakeHitSpecs() const
{
	TArray<FGameplayEffectSpecHandle> Specs;

	const FGameplayEffectSpecHandle DamageSpec = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
	if (DamageSpec.IsValid())
	{
		DamageSpec.Data->SetSetByCallerMagnitude(SGameplayTags::SetByCaller_DamageMultiplier, DamageMultiplier);
		Specs.Add(DamageSpec);
	}

	const FGameplayEffectSpecHandle MarkSpec = MakeMarkSpec();
	if (MarkSpec.IsValid())
	{
		Specs.Add(MarkSpec);
	}
	return Specs;
}

void USGA_CastAbility::ApplyMarkTo(AActor* Target) const
{
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	const FGameplayEffectSpecHandle MarkSpec = MakeMarkSpec();
	if (SourceASC && TargetASC && MarkSpec.IsValid())
	{
		SourceASC->ApplyGameplayEffectSpecToTarget(*MarkSpec.Data.Get(), TargetASC);
	}
}

FGameplayEffectSpecHandle USGA_CastAbility::MakeMarkSpec() const
{
	// 표식: 공통 "시간 태그" GE(USGE_Cooldown)에 태그와 시간을 넣음 (내려찍기 균열과 같은 방식)
	if (!MarkTag.IsValid() || MarkDuration <= 0.f)
	{
		return FGameplayEffectSpecHandle();
	}

	const FGameplayEffectSpecHandle MarkSpec = MakeOutgoingGameplayEffectSpec(USGE_Cooldown::StaticClass(), GetAbilityLevel());
	if (MarkSpec.IsValid())
	{
		MarkSpec.Data->DynamicGrantedTags.AddTag(MarkTag);
		MarkSpec.Data->SetSetByCallerMagnitude(SGameplayTags::SetByCaller_Duration, MarkDuration);
	}
	return MarkSpec;
}

void USGA_CastAbility::OnCastEvent(FGameplayEventData Payload)
{
	if (bCastFired)
	{
		return;
	}
	bCastFired = true;

	// 시전 순간 다시 커서를 봄 (시전 중 커서가 움직였을 수 있음)
	FaceAimPoint();

	FVector Origin;
	FVector Direction;
	ComputeAim(Origin, Direction);

	if (LaunchEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetAvatarActorFromActorInfo(), LaunchEffect, Origin, Direction.Rotation());
	}

	OnCast(Origin, Direction);

	// 마무리 동작을 다 기다리지 않고 RecoveryTime 뒤 움직일 수 있게 (몽타주는 능력이 끝나면 블렌드 아웃)
	UAbilityTask_WaitDelay* RecoveryTask = UAbilityTask_WaitDelay::WaitDelay(this, RecoveryTime);
	RecoveryTask->OnFinish.AddDynamic(this, &ThisClass::OnRecoveryFinished);
	RecoveryTask->ReadyForActivation();
}

void USGA_CastAbility::OnRecoveryFinished()
{
	bRecoveryDone = true;
	SetAttacking(false);
	TryFinishCast();
}

void USGA_CastAbility::OnMontageBlendOut()
{
	// 신호를 받기 전에 몽타주가 끝남 → 시전 못 하고 종료
	if (!bCastFired)
	{
		OnMontageInterrupted();
	}
}

void USGA_CastAbility::OnMontageInterrupted()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void USGA_CastAbility::ComputeAim(FVector& OutOrigin, FVector& OutDirection) const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		OutOrigin = FVector::ZeroVector;
		OutDirection = FVector::ForwardVector;
		return;
	}

	// 이 능력을 가진 캐릭터의 몸 (캐릭터마다 ASC가 따로라 마법사 메시)
	const USkeletalMeshComponent* Mesh = Cast<USkeletalMeshComponent>(GetOwningComponentFromActorInfo());
	const FVector Hand = Mesh && Mesh->DoesSocketExist(MuzzleBone) ? Mesh->GetSocketLocation(MuzzleBone) : Avatar->GetActorLocation();
	const FVector AimPoint = GetAimPoint();

	// 방향은 "손 → 커서" (수평). 몸 가운데 → 커서로 하면 손이 오른쪽에 있는 만큼 커서 옆으로 평행하게 빗나감
	OutDirection = AimPoint - Hand;
	OutDirection.Z = 0.f;

	// 커서가 손에 너무 가까우면(발밑·손 뒤) 손 기준 방향이 크게 틀어짐 → 그때만 몸 가운데 기준
	if (OutDirection.SizeSquared() < FMath::Square(MinHandAimDistance))
	{
		OutDirection = AimPoint - Avatar->GetActorLocation();
		OutDirection.Z = 0.f;
	}
	if (!OutDirection.Normalize())
	{
		OutDirection = Avatar->GetActorForwardVector();
	}

	OutOrigin = Hand + OutDirection * MuzzleForwardOffset;
}

void USGA_CastAbility::SetAttacking(bool bAttacking)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || bAttacking == bAttackingTagAdded)
	{
		return;
	}

	if (bAttacking)
	{
		ASC->AddLooseGameplayTag(SGameplayTags::State_Attacking);
	}
	else
	{
		ASC->RemoveLooseGameplayTag(SGameplayTags::State_Attacking);
	}
	bAttackingTagAdded = bAttacking;
}
