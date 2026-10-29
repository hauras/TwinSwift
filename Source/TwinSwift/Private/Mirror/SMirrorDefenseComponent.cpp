#include "Mirror/SMirrorDefenseComponent.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Ability/GA/SGA_Dodge.h"
#include "Ability/GA/SGA_GroundSlam.h"
#include "Character/SMirrorBoss.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SGameplayTags.h"
#include "TimerManager.h"

USMirrorDefenseComponent::USMirrorDefenseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USMirrorDefenseComponent::BeginPlay()
{
	Super::BeginPlay();

	GroundSlamHandle = USGA_GroundSlam::OnTelegraphed.AddUObject(this, &ThisClass::HandleGroundSlam);
}

void USMirrorDefenseComponent::HandleGroundSlam(const FVector& LandingLocation, float Radius, UGameplayAbility* SourceAbility)
{
	// 보스가 복사해 쓴 내려찍기는 자기 공격이라 피하지 않음
	if (SourceAbility && SourceAbility->GetAvatarActorFromActorInfo() == GetOwner()) return;

	NotifyIncomingAttack(LandingLocation, Radius, SourceAbility);
}

bool USMirrorDefenseComponent::CanPrepareEvade(const ASMirrorBoss* Boss) const
{
	if (!Boss || Boss->IsDead() || !Boss->IsInCombat()) return false;

	const UAbilitySystemComponent* ASC = Boss->GetAbilitySystemComponent();
	if (!ASC) return false;

	// 플레이어 회피와 같은 규칙(경직·사망만 막음) + 근접 공격 중(State.Attacking)
	// → 칼을 휘두르는 중에 맞힌 내려찍기는 못 피함 (플레이어가 노릴 틈). 마법 시전·효과만 남은 상태는 피함 (회피가 끊음, 시전 확정된 번개는 남음)
	FGameplayTagContainer BlockTags;
	BlockTags.AddTag(SGameplayTags::State_Groggy);
	BlockTags.AddTag(SGameplayTags::State_Stun);
	BlockTags.AddTag(SGameplayTags::State_Attacking);
	BlockTags.AddTag(SGameplayTags::State_Dead);
	BlockTags.AddTag(SGameplayTags::Cooldown_Dodge);
	if (ASC->HasAnyMatchingGameplayTags(BlockTags)) return false;

	return true;
}

bool USMirrorDefenseComponent::IsInsideAttack(const ASMirrorBoss* Boss) const
{
	const UCapsuleComponent* Capsule = Boss ? Boss->GetCapsuleComponent() : nullptr;
	if (!Capsule) return false;

	const FVector Location = Boss->GetActorLocation();
	// 원형 지면 공격용. 다른 층에 있는 보스까지 회피하지 않게 높이도 확인.
	if (FMath::Abs(Location.Z - PendingAttackCenter.Z) > Capsule->GetScaledCapsuleHalfHeight()) return false;
	const float DangerDistance = PendingAttackRadius + Capsule->GetScaledCapsuleRadius();
	return FVector::DistSquared2D(Location, PendingAttackCenter) <= FMath::Square(DangerDistance);
}

const USGA_Dodge* USMirrorDefenseComponent::FindDodgeAbility(
	const ASMirrorBoss* Boss,
	FGameplayAbilitySpecHandle& OutHandle) const
{
	UAbilitySystemComponent* ASC = Boss ? Boss->GetAbilitySystemComponent() : nullptr;
	if (!ASC) return nullptr;

	const bool bWantTeleport = Boss->GetActiveSlot() == ETwinSlot::Mage;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		const USGA_Dodge* Dodge = Cast<USGA_Dodge>(Spec.Ability);
		if (Dodge && Dodge->IsInstantDodge() == bWantTeleport
			&& !Spec.IsActive()
			&& Dodge->CanActivateAbility(Spec.Handle, ASC->AbilityActorInfo.Get()))
		{
			OutHandle = Spec.Handle;
			return Dodge;
		}
	}
	return nullptr;
}

void USMirrorDefenseComponent::NotifyIncomingAttack(
	const FVector& AttackCenter,
	float AttackRadius,
	UGameplayAbility* SourceAbility)
{
	const ASMirrorBoss* Boss = Cast<ASMirrorBoss>(GetOwner());
	if (bEvadePending || !CanPrepareEvade(Boss) || AttackCenter.ContainsNaN()
		|| !FMath::IsFinite(AttackRadius) || AttackRadius <= 0.f) return;

	PendingAttackCenter = AttackCenter;
	PendingAttackRadius = AttackRadius;
	if (!IsInsideAttack(Boss)) return;

	FGameplayAbilitySpecHandle Handle;
	if (!FindDodgeAbility(Boss, Handle)) return;

	PendingSourceAbility = SourceAbility;
	bTrackSourceAbility = SourceAbility != nullptr;
	bEvadePending = true;
	GetWorld()->GetTimerManager().SetTimer(
		EvadeTimer,
		this,
		&ThisClass::TryEvade,
		FMath::Max(ReactionDelay, 0.01f),
		false);
}

bool USMirrorDefenseComponent::FindSafeDirection(
	const ASMirrorBoss* Boss,
	const FVector& Center,
	float DangerRadius,
	float DodgeDistance,
	FVector& OutDirection) const
{
	if (!Boss || DodgeDistance <= 0.f) return false;
	const UWorld* World = GetWorld();
	const UCapsuleComponent* Capsule = Boss->GetCapsuleComponent();
	const UCharacterMovementComponent* Movement = Boss->GetCharacterMovement();
	if (!World || !Capsule || !Movement) return false;

	const FVector Start = Boss->GetActorLocation();
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	FVector Away = (Start - Center).GetSafeNormal2D();
	if (Away.IsNearlyZero()) Away = Boss->GetActorRightVector().GetSafeNormal2D();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MirrorEvade), false, Boss);
	const FCollisionShape Shape = FCollisionShape::MakeCapsule(
		FMath::Max(1.f, Radius - 1.f),
		FMath::Max(1.f, HalfHeight - 1.f));
	const float SafeRadius = DangerRadius + Radius + FMath::Max(0.f, SafetyMargin);
	const float FloorZ = Start.Z - HalfHeight;
	// 바깥 방향을 우선하고, 막혔으면 양옆 후보를 비교. 위험한 방향으로 억지 회피하지 않음.
	const float Angles[] = { 0.f, 45.f, -45.f, 90.f, -90.f, 135.f, -135.f, 180.f };
	float BestScore = -TNumericLimits<float>::Max();
	bool bFound = false;
	for (float Angle : Angles)
	{
		const FVector Direction = Away.RotateAngleAxis(Angle, FVector::UpVector);
		const FVector End = Start + Direction * DodgeDistance;
		const float EndDistance = FVector::DistSquared2D(End, Center);
		if (EndDistance < FMath::Square(SafeRadius)) continue;

		FHitResult Wall;
		if (World->SweepSingleByProfile(
			Wall, Start, End, FQuat::Identity,
			Capsule->GetCollisionProfileName(), Shape, Params)) continue;
		if (World->OverlapBlockingTestByProfile(
			End, FQuat::Identity, Capsule->GetCollisionProfileName(), Shape, Params)) continue;

		// 경로와 도착점 아래에 걸을 수 있는 바닥이 있어야 함 (낭떠러지·큰 높이 차이 제외).
		bool bGroundSafe = true;
		for (int32 Step = 1; Step <= 4; ++Step)
		{
			const FVector Point = FMath::Lerp(Start, End, Step / 4.f);
			FHitResult Ground;
			const bool bHit = World->LineTraceSingleByObjectType(
				Ground,
				FVector(Point.X, Point.Y, FloorZ + Movement->MaxStepHeight + 5.f),
				FVector(Point.X, Point.Y, FloorZ - Movement->MaxStepHeight - 5.f),
				FCollisionObjectQueryParams(ECC_WorldStatic),
				Params);
			if (!bHit || !Movement->IsWalkable(Ground)
				|| FMath::Abs(Ground.ImpactPoint.Z - FloorZ) > Movement->MaxStepHeight)
			{
				bGroundSafe = false;
				break;
			}
		}
		if (!bGroundSafe) continue;

		// 같은 탈출 거리라면 적은 회전으로 범위 바깥에 나가는 쪽을 고름.
		const float Score = FMath::Sqrt(EndDistance) - FMath::Abs(Angle) * 0.1f;
		if (Score > BestScore)
		{
			BestScore = Score;
			OutDirection = Direction;
			bFound = true;
		}
	}
	return bFound;
}

void USMirrorDefenseComponent::TryEvade()
{
	bEvadePending = false;
	ASMirrorBoss* Boss = Cast<ASMirrorBoss>(GetOwner());
	const bool bAttackStillActive = !bTrackSourceAbility
		|| (PendingSourceAbility.IsValid() && PendingSourceAbility->IsActive());
	PendingSourceAbility.Reset();
	bTrackSourceAbility = false;
	if (!bAttackStillActive || !CanPrepareEvade(Boss) || !IsInsideAttack(Boss)) return;

	FGameplayAbilitySpecHandle Handle;
	const USGA_Dodge* Dodge = FindDodgeAbility(Boss, Handle);
	FVector Direction;
	if (!Dodge || !FindSafeDirection(Boss, PendingAttackCenter, PendingAttackRadius, Dodge->GetDodgeDistance(), Direction)) return;

	ActivateDodge(Boss, Handle, Dodge, Direction);
}

bool USMirrorDefenseComponent::TryEvadeFrom(const FVector& Center)
{
	ASMirrorBoss* Boss = Cast<ASMirrorBoss>(GetOwner());
	if (bEvadePending || !CanPrepareEvade(Boss)) return false;

	FGameplayAbilitySpecHandle Handle;
	const USGA_Dodge* Dodge = FindDodgeAbility(Boss, Handle);
	if (!Dodge) return false;

	// 지금 거리보다 멀어지는 방향만 (위협 쪽으로 넘어가지 않게)
	const float CurrentDistance = FVector::Dist2D(Boss->GetActorLocation(), Center);
	const float Margin = Boss->GetCapsuleComponent()->GetScaledCapsuleRadius() + SafetyMargin;
	FVector Direction;
	if (!FindSafeDirection(Boss, Center, FMath::Max(0.f, CurrentDistance - Margin), Dodge->GetDodgeDistance(), Direction)) return false;

	return ActivateDodge(Boss, Handle, Dodge, Direction);
}

bool USMirrorDefenseComponent::ActivateDodge(ASMirrorBoss* Boss, const FGameplayAbilitySpecHandle& Handle, const USGA_Dodge* Dodge, const FVector& Direction)
{
	UAbilitySystemComponent* ASC = Boss->GetAbilitySystemComponent();
	Boss->SetReplayMoveDirection(Direction);
	const bool bActivated = ASC->TryActivateAbility(Handle);

	// 회피 방향은 ActivateAbility 안에서 읽으므로 다음 회피에 남기지 않음
	Boss->SetReplayMoveDirection(FVector::ZeroVector);
	if (bActivated)
	{
		UE_LOG(LogTemp, Display, TEXT("[거울 방어] %s: %s"),
			*Boss->GetName(), Dodge->IsInstantDodge() ? TEXT("순간이동") : TEXT("구르기"));
	}
	return bActivated;
}

bool USMirrorDefenseComponent::IsDefending() const
{
	if (bEvadePending) return true;
	const ASMirrorBoss* Boss = Cast<ASMirrorBoss>(GetOwner());
	const UAbilitySystemComponent* ASC = Boss ? Boss->GetAbilitySystemComponent() : nullptr;
	if (!ASC) return false;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.IsActive() && Cast<USGA_Dodge>(Spec.Ability)) return true;
	}
	return false;
}

void USMirrorDefenseComponent::CancelPendingEvade()
{
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(EvadeTimer);
	bEvadePending = false;
	bTrackSourceAbility = false;
	PendingSourceAbility.Reset();
	if (ASMirrorBoss* Boss = Cast<ASMirrorBoss>(GetOwner()))
	{
		Boss->SetReplayMoveDirection(FVector::ZeroVector);
	}
}

void USMirrorDefenseComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 알림은 static이라 레벨이 바뀌어도 남음 → 꼭 풂
	USGA_GroundSlam::OnTelegraphed.Remove(GroundSlamHandle);
	CancelPendingEvade();
	Super::EndPlay(EndPlayReason);
}
