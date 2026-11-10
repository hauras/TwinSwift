#include "Ability/GA/SGA_ArcaneOrb.h"

#include "Actor/SArcaneOrb.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "SGameplayTags.h"
#include "TimerManager.h"

USGA_ArcaneOrb::USGA_ArcaneOrb()
{
	InputTag = SGameplayTags::InputTag_SecondaryAttack;

	ActivationOwnedTags.AddTag(SGameplayTags::State_Attacking);
	ActivationBlockedTags.AddTag(SGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(SGameplayTags::State_Stun);
	ActivationBlockedTags.AddTag(SGameplayTags::State_Groggy);

	// 다른 마법사 스킬과 같음: 공격 중(State.Attacking)이어도 막지 않고, 화염탄 중에 누르면 화염탄을 끊고 시전
	// 구체 시전 중 다시 누르면 이미 켜진 능력이라 새로 발동하지 않음
	CancelAbilitiesWithTag.AddTag(SGameplayTags::Ability_Mage_FireBolt);

	CooldownDuration = 6.f;
	CooldownTags.AddTag(SGameplayTags::Cooldown_Mage_ArcaneOrb);
}

bool USGA_ArcaneOrb::HasActiveOrb() const
{
	const ASArcaneOrb* Orb = ActiveOrb.Get();
	return Orb && !Orb->IsActorBeingDestroyed();
}

bool USGA_ArcaneOrb::CheckCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	return HasActiveOrb() || Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);
}

bool USGA_ArcaneOrb::FindPlacement(FTransform& OutTransform) const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();
	if (!Avatar || !World) return false;

	// 조준점을 수평으로 PlacementRange까지 당긴 지점
	const FVector Origin = Avatar->GetActorLocation();
	const FVector Offset = (GetAimPoint() - Origin).GetClampedToMaxSize2D(PlacementRange);
	const FVector Candidate = Origin + FVector(Offset.X, Offset.Y, 0.f);

	// 그 아래 바닥을 찾음. 벽·경사면(법선 Z < 0.7)이면 설치 안 함
	FHitResult FloorHit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Avatar);
	const bool bHitFloor = World->LineTraceSingleByObjectType(
		FloorHit,
		Candidate + FVector(0.f, 0.f, 300.f),
		Candidate - FVector(0.f, 0.f, 600.f),
		FCollisionObjectQueryParams(ECC_WorldStatic),
		Params);
	if (!bHitFloor || FloorHit.ImpactNormal.Z < 0.7f) return false;

	// 바닥에서 조금 띄움 (구체가 땅에 묻히지 않게)
	OutTransform = FTransform(FRotator::ZeroRotator, FloorHit.ImpactPoint + FVector(0.f, 0.f, 45.f));
	return true;
}

void USGA_ArcaneOrb::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!GetWorld() || !IsValid(GetAvatarActorFromActorInfo())
		|| !GetAbilitySystemComponentFromActorInfo() || !HasAuthority(&ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 구체가 있으면 이번 시전은 폭발, 없으면 설치 (시작할 때 정해 둠)
	bReleased = false;
	bDetonating = HasActiveOrb();
	PendingDetonation = bDetonating ? ActiveOrb : TWeakObjectPtr<ASArcaneOrb>();

	if (!bDetonating && (!OrbClass || !DamageEffectClass || !FindPlacement(PendingPlacement)))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FaceAimPoint();

	// 몽타주가 없으면 바로 설치·폭발
	UAnimMontage* Montage = bDetonating ? DetonateMontage.Get() : InstallMontage.Get();
	if (!Montage)
	{
		ReleaseOrb();
		if (IsActive())
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, !bReleased);
		}
		return;
	}

	// 손동작의 신호를 먼저 기다려 첫 프레임의 신호도 놓치지 않게
	UAbilityTask_WaitGameplayEvent* EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		SGameplayTags::Event_Montage_Hit,
		nullptr,
		true);
	EventTask->EventReceived.AddDynamic(this, &ThisClass::OnCastEvent);
	EventTask->ReadyForActivation();

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		Montage,
		CastPlayRate,
		NAME_None,
		true,
		1.f,
		bDetonating ? DetonateStartTime : InstallStartTime);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnCastEnded);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnCastEnded);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnCastCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnCastCancelled);
	MontageTask->ReadyForActivation();
}

void USGA_ArcaneOrb::ReleaseOrb()
{
	if (!IsActive() || bReleased) return;

	bReleased = true;

	if (bDetonating)
	{
		// 예고 중 자동 폭발·취소됐다면 다른 구체를 대신 만들지 않음
		ASArcaneOrb* Orb = PendingDetonation.Get();
		if (Orb && !Orb->IsActorBeingDestroyed())
		{
			Orb->Explode();
		}
		return;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(Avatar) || !ASC || !OrbClass || !GetWorld()) return;

	const FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
	if (!Spec.IsValid()) return;

	// 지연 생성: 쿨다운을 먼저 걸고 피해 스펙을 넘긴 뒤 BeginPlay가 돌게
	ASArcaneOrb* Orb = GetWorld()->SpawnActorDeferred<ASArcaneOrb>(
		OrbClass,
		PendingPlacement,
		Avatar,
		Cast<APawn>(Avatar),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Orb) return;

	// 쿨다운은 실제로 설치되는 순간 시작 (신호 전에 취소되면 쿨다운도 없음)
	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo) || !IsActive())
	{
		Orb->Destroy();
		return;
	}

	Orb->Initialize(ASC, Spec);
	ActiveOrb = Orb;
	UGameplayStatics::FinishSpawningActor(Orb, PendingPlacement);
}

void USGA_ArcaneOrb::OnCastEvent(FGameplayEventData Payload)
{
	ReleaseOrb();
	if (!IsActive()) return;

	// 신호 뒤 짧은 복귀 동작만 보여 주고 종료 (몽타주 끝까지 기다리지 않음)
	GetWorld()->GetTimerManager().SetTimer(
		RecoveryTimer,
		this,
		&ThisClass::OnCastEnded,
		FMath::Max(CastRecoveryTime, 0.01f),
		false);
}

void USGA_ArcaneOrb::OnCastEnded()
{
	if (!IsActive()) return;

	// 신호 전에 몽타주가 끝났으면(설치·폭발 못 함) 취소로 끝냄
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, !bReleased);
}

void USGA_ArcaneOrb::OnCastCancelled()
{
	if (!IsActive()) return;

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USGA_ArcaneOrb::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RecoveryTimer);
	}
	PendingDetonation.Reset();

	// 교체로 시전이 취소돼도 이미 설치된 구체는 남김
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
