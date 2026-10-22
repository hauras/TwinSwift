#include "Ability/GA/SGA_DashSlash.h"

#include "SGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

namespace
{
	// 돌진 중 길을 검사하는 간격(초). 빠르게 지나가도 적을 건너뛰지 않게 위치 사이를 쓸어서 검사
	constexpr float SweepInterval = 0.02f;
}

USGA_DashSlash::USGA_DashSlash()
{
	InputTag = SGameplayTags::InputTag_Warrior_Skill1;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(SGameplayTags::Ability_Warrior_DashSlash);
	SetAssetTags(AssetTags);

	// 돌진 중에는 이동 입력을 막음
	ActivationOwnedTags.AddTag(SGameplayTags::State_Attacking);

	ActivationBlockedTags.AddTag(SGameplayTags::State_Stun);
	ActivationBlockedTags.AddTag(SGameplayTags::State_Dead);

	// 콤보 중에 누르면 콤보를 끊고 돌진
	CancelAbilitiesWithTag.AddTag(SGameplayTags::Ability_Warrior_Combo);

	CooldownDuration = 6.f;
	CooldownTags.AddTag(SGameplayTags::Cooldown_Warrior_DashSlash);
}

void USGA_DashSlash::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	HitActors.Reset();
	bMoveFinished = false;
	bMontageFinished = DashMontage == nullptr;

	// 커서 쪽으로 돌고 그 방향으로 돌진
	FaceAimPoint();
	const FVector Direction = Character->GetActorForwardVector();

	if (DashMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			DashMontage,
			MontagePlayRate,
			NAME_None,
			true,
			1.f,
			MontageStartTime);
		// 1차 교훈: 끝나기 시작(BlendOut)·끊김·취소 모두 종료 경로로
		MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageEnded);
		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageEnded);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageEnded);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageEnded);
		MontageTask->ReadyForActivation();
	}

	// 정해진 시간 동안 일정한 힘으로 밀기 (끝나면 걷기 속도로 줄여서 미끄러지지 않게)
	UAbilityTask_ApplyRootMotionConstantForce* MoveTask = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
		this,
		NAME_None,
		Direction,
		DashDistance / DashDuration,
		DashDuration,
		false,
		nullptr,
		ERootMotionFinishVelocityMode::ClampVelocity,
		FVector::ZeroVector,
		Character->GetCharacterMovement()->MaxWalkSpeed,
		true);
	MoveTask->OnFinish.AddDynamic(this, &ThisClass::OnMoveFinished);
	MoveTask->ReadyForActivation();

	// 출발 자리에 붙어 있는 적도 맞도록 시작 위치에서 한 번 검사
	LastSweepLocation = Character->GetActorLocation();
	SweepPath();

	// 첫 검사의 피해(산산조각 등)가 이어져 능력이 이미 끝났을 수 있음 → 끝났으면 타이머를 켜지 않음 (켜면 끝난 능력이 계속 벰)
	if (!IsActive())
	{
		return;
	}
	GetWorld()->GetTimerManager().SetTimer(SweepTimer, this, &ThisClass::SweepPath, SweepInterval, true);
}

void USGA_DashSlash::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 취소돼도 검사 타이머가 남지 않게
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SweepTimer);
	}
	HitActors.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USGA_DashSlash::SweepPath()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UWorld* World = GetWorld();
	if (!IsActive() || !Character || !World)
	{
		return;
	}

	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	const FCollisionShape Shape = FCollisionShape::MakeCapsule(
		Capsule->GetScaledCapsuleRadius() + HitPadding,
		Capsule->GetScaledCapsuleHalfHeight());

	const FVector Start = LastSweepLocation;
	const FVector End = Character->GetActorLocation();
	LastSweepLocation = End;

	// 칼날 판정(ANS_WeaponTrace)과 같은 기준: Pawn 오브젝트만
	TArray<FHitResult> Hits;
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(DashSlash), false, Character);
	World->SweepMultiByObjectType(
		Hits,
		Start,
		End,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		Shape,
		Params);

	for (const FHitResult& Hit : Hits)
	{
		AActor* Target = Hit.GetActor();
		if (!Target || HitActors.Contains(Target))
		{
			continue;
		}

		// 같은 편은 ApplyDamageTo가 거름. 같은 편도 목록에 넣어 다시 검사하지 않음
		HitActors.Add(Target);
		ApplyDamageTo(Target, DamageMultiplier);
	}
}

void USGA_DashSlash::OnMoveFinished()
{
	// 마지막 구간까지 검사하고 멈춤
	SweepPath();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SweepTimer);
	}

	bMoveFinished = true;
	TryEndDash();
}

void USGA_DashSlash::OnMontageEnded()
{
	bMontageFinished = true;
	TryEndDash();
}

void USGA_DashSlash::TryEndDash()
{
	if (bMoveFinished && bMontageFinished && IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}
