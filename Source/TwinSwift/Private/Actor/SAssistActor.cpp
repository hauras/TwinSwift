#include "Actor/SAssistActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "SGameplayTags.h"
#include "Ability/SAbilityFunctionLibrary.h"
#include "Ability/SAttributeSet.h"
#include "Ability/SGE_Damage.h"
#include "Engine/OverlapResult.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Projectile/SProjectile.h"
#include "Pool/SActorPool.h"
#include "TimerManager.h"

namespace
{
	// 가까운 적 고르기 후보: 액터 + 중심까지 거리 제곱 (비교만 하니 제곱근을 안 구함)
	struct FCandidate
	{
		AActor* Actor = nullptr;
		float DistSq = 0.f;
	};
}

ASAssistActor::ASAssistActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
	SetRootComponent(Body);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Weapon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Weapon"));
	Weapon->SetupAttachment(Body, TEXT("WeaponSocket"));
	Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 잔상 재질(반투명)은 나나이트로 못 그림 → 이 컴포넌트만 일반 메시로 (검 에셋의 나나이트는 그대로, 들고 있는 검에 영향 없음)
	Weapon->bDisallowNanite = true;

	DamageEffectClass = USGE_Damage::StaticClass();
}

void ASAssistActor::StartAssist(UAbilitySystemComponent* InSourceASC, const FVector& InAimDirection)
{
	SourceASC = InSourceASC;

	// 겨냥 방향은 수평으로만 (위아래로 날아가지 않게). 0이면 기본 앞쪽
	const FVector FlatAim = FVector(InAimDirection.X, InAimDirection.Y, 0.f).GetSafeNormal();
	if (!FlatAim.IsNearlyZero())
	{
		AimDirection = FlatAim;
	}

	if (GhostMaterial)
	{
		for (int32 Index = 0; Index < Body->GetNumMaterials(); ++Index)
		{
			Body->SetMaterial(Index, GhostMaterial);
		}
		for (int32 Index = 0; Index < Weapon->GetNumMaterials(); ++Index)
		{
			Weapon->SetMaterial(Index, GhostMaterial);
		}
	}
	if (AssistAnimation)
	{
		Body->PlayAnimation(AssistAnimation, false);

		// 준비 동작을 건너뛰고 중간부터 (HitTimes는 이 시점부터 잼)
		if (AnimStartTime > 0.f)
		{
			Body->SetPosition(AnimStartTime, false);
		}
	}

	// 타격 시각을 빠른 순으로 정리하고 첫 타격부터 하나씩 예약
	SortedHitTimes = HitTimes;
	SortedHitTimes.Sort();
	NextHitIndex = 0;
	ScheduleNextHit();

	// 마지막 타격 전에 사라지지 않게 최소 마지막 타격 + 0.1초는 남음
	const float LastHitTime = SortedHitTimes.Num() > 0 ? SortedHitTimes.Last() : 0.f;
	SetLifeSpan(FMath::Max(LifeTime, LastHitTime + 0.1f));
}

void ASAssistActor::ScheduleNextHit()
{
	if (!SortedHitTimes.IsValidIndex(NextHitIndex))
	{
		return;
	}

	// 다음 타격까지 남은 시간 = 그 타격 시각 - 지금까지 지난 시간
	const float Elapsed = NextHitIndex > 0 ? SortedHitTimes[NextHitIndex - 1] : 0.f;
	const float Delay = SortedHitTimes[NextHitIndex] - Elapsed;

	// 타이머는 0초 이하면 걸리지 않음 → 그때는 바로 타격
	if (Delay > 0.f)
	{
		GetWorldTimerManager().SetTimer(HitTimer, this, &ThisClass::OnHitTimer, Delay, false);
	}
	else
	{
		OnHitTimer();
	}
}

void ASAssistActor::OnHitTimer()
{
	++NextHitIndex;
	DoHit();
	ScheduleNextHit();
}

void ASAssistActor::DoHit()
{
	UAbilitySystemComponent* ASC = SourceASC.Get();
	if (!ASC || !DamageEffectClass)
	{
		return;
	}

	if (HitEffect)
	{
		USAbilityFunctionLibrary::SpawnOneShotEffect(GetWorld(), HitEffect, GetActorLocation());
	}

	if (ProjectileClass)
	{
		FireProjectile(ASC);
	}
	else
	{
		HitInRadius(ASC);
	}
}

void ASAssistActor::HitInRadius(UAbilitySystemComponent* ASC)
{
	TArray<AActor*> Enemies;
	GatherEnemies(GetActorLocation(), Radius, Enemies);

	for (AActor* Target : Enemies)
	{
		// 산산조각(TryShatterFrost)을 부르지 않음: 어시스트는 표식을 소비하지 않음
		const FGameplayEffectSpecHandle Spec = MakeDamageSpec(ASC);
		if (Spec.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToTarget(
				*Spec.Data.Get(),
				UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target));
		}
	}
}

void ASAssistActor::GatherEnemies(const FVector& Center, float SearchRadius, TArray<AActor*>& OutEnemies) const
{
	// 같은 편 판단과 겹침 검사 제외용: 나가는 캐릭터의 몸
	const UAbilitySystemComponent* ASC = SourceASC.Get();
	const AActor* SourceAvatar = ASC ? ASC->GetAvatarActor() : nullptr;

	// 반경 안의 Pawn 찾기 (칼날 판정·능력 범위 피해와 같은 기준). 분신 자신과 나가는 캐릭터는 뺌
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(AssistGather), false, this);
	Params.AddIgnoredActor(SourceAvatar);
	GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		Center,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(SearchRadius),
		Params);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		// 한 액터의 컴포넌트가 여럿 걸려도 한 번만
		AActor* Target = Overlap.GetActor();
		if (!Target || OutEnemies.Contains(Target))
		{
			continue;
		}
		if (!USAbilityFunctionLibrary::IsTargetable(SourceAvatar, Target))
		{
			continue;
		}
		OutEnemies.Add(Target);
	}
}

void ASAssistActor::FindNearestEnemies(
	const FVector& Center,
	float SearchRadius,
	int32 Count,
	TArray<AActor*>& OutTargets) const
{
	if (Count <= 0)
	{
		return;
	}

	TArray<AActor*> Enemies;
	GatherEnemies(Center, SearchRadius, Enemies);

	// 지금까지 본 적 중 가장 가까운 Count명. 맨 위(HeapTop)가 그중 가장 먼 적
	// TArray 힙은 비교가 참인 쪽이 위로 감 → "더 멀면 참"으로 줘서 최대 힙
	auto FartherFirst = [](const FCandidate& A, const FCandidate& B) { return A.DistSq > B.DistSq; };
	TArray<FCandidate> Heap;
	Heap.Reserve(Count);

	for (AActor* Enemy : Enemies)
	{
		const FCandidate Candidate{ Enemy, static_cast<float>(FVector::DistSquared(Center, Enemy->GetActorLocation())) };

		// 덜 찼으면 그냥 넣음
		if (Heap.Num() < Count)
		{
			Heap.HeapPush(Candidate, FartherFirst);
		}
		// 꽉 찼으면 지금 가장 먼 적보다 가까울 때만 바꿈 → 후보 n명에 O(n log k)
		else if (Candidate.DistSq < Heap.HeapTop().DistSq)
		{
			Heap.HeapPopDiscard(FartherFirst);
			Heap.HeapPush(Candidate, FartherFirst);
		}
	}

	// 힙은 맨 위만 정해져 있고 나머지 순서는 섞여 있음 → 가까운 순으로 정렬 (k명뿐이라 싸다)
	Heap.Sort([](const FCandidate& A, const FCandidate& B) { return A.DistSq < B.DistSq; });
	for (const FCandidate& Candidate : Heap)
	{
		OutTargets.Add(Candidate.Actor);
	}
}

void ASAssistActor::FireProjectile(UAbilitySystemComponent* ASC)
{
	UWorld* World = GetWorld();
	AActor* SourceAvatar = ASC->GetAvatarActor();
	const FGameplayEffectSpecHandle Spec = MakeDamageSpec(ASC);
	if (!World || !SourceAvatar || !Spec.IsValid())
	{
		return;
	}

	// 분신 발밑(메시 위치)에서 ProjectileHeight만큼 위
	const FVector Origin = GetActorLocation() + FVector(0.f, 0.f, ProjectileHeight);

	// 유도: 이번 발의 표적 (없으면 겨냥 방향으로 곧게)
	AActor* HomingTarget = bHomingToNearest ? PickHomingTarget() : nullptr;

	// 처음 방향: 표적 쪽(없으면 겨냥 방향)을 기준으로, 유도면 발 순서대로 부채꼴로 벌림 → 휘어 들어가는 게 보임
	FVector Direction = AimDirection;
	if (HomingTarget)
	{
		const FVector ToTarget = (HomingTarget->GetActorLocation() - Origin).GetSafeNormal2D();
		Direction = ToTarget.IsNearlyZero() ? AimDirection : ToTarget;
		Direction = Direction.RotateAngleAxis(GetFanAngle(NextHitIndex - 1), FVector::UpVector);
	}
	const FTransform SpawnTransform(Direction.Rotation(), Origin);

	// 풀에서 꺼냄. 주인 = 나가는 캐릭터 → 투사체가 같은 편 판단·쏜 캐릭터 통과를 그 캐릭터 기준으로 함
	USActorPool* Pool = World->GetSubsystem<USActorPool>();
	ASProjectile* Projectile = Pool ? Pool->Acquire<ASProjectile>(
		ProjectileClass,
		SpawnTransform,
		SourceAvatar,
		Cast<APawn>(SourceAvatar)) : nullptr;
	if (!Projectile)
	{
		return;
	}

	// 스펙에 산산조각이 없음: 어시스트는 표식을 소비하지 않음
	Projectile->InitializeProjectile(ASC, { Spec });
	Projectile->SetHomingTarget(HomingTarget);
	Projectile->Launch();
}

AActor* ASAssistActor::PickHomingTarget()
{
	// 첫 발에서 한 번만: 발마다 표적을 미리 정함 (예상 피해로 배분)
	if (!bHomingTargetsChosen)
	{
		bHomingTargetsChosen = true;
		AssignShotTargets();
	}

	// 이번 발의 표적. 날아가기 전에 사라졌거나 죽었으면 그 뒤 발의 표적 중 남은 적
	// 표적은 첫 발에서 저장해 둔 것 → 쓰는 지금 다시 확인 (보스는 죽어도 액터가 남아 포인터만으로는 모름)
	const UAbilitySystemComponent* ASC = SourceASC.Get();
	const AActor* SourceAvatar = ASC ? ASC->GetAvatarActor() : nullptr;
	const int32 ShotIndex = FMath::Max(0, NextHitIndex - 1);
	for (int32 Index = ShotIndex; Index < ShotTargets.Num(); ++Index)
	{
		AActor* Target = ShotTargets[Index].Get();
		if (USAbilityFunctionLibrary::IsTargetable(SourceAvatar, Target))
		{
			return Target;
		}
	}
	return nullptr;
}

void ASAssistActor::AssignShotTargets()
{
	// 예상 피해 배분 (오버킬 방지): 가까운 적부터, 그 적의 지금 체력이 0이 될 만큼만 탄을 주고 남은 탄은 다음 적
	//   예) 1발 40, 가까운 순 [A 체력 50, B 300] → A 2발, B 1발
	// 쏘는 순간에 다 정해서 투사체는 정해진 표적만 따라감 (날아가는 도중 표적을 바꾸지 않음)
	const int32 ShotCount = SortedHitTimes.Num();
	ShotTargets.Reset();

	// 한 발에 한 명씩 가도 최대 ShotCount명이면 충분
	TArray<AActor*> Nearest;
	FindNearestEnemies(GetActorLocation(), HomingSearchRadius, ShotCount, Nearest);
	if (Nearest.Num() == 0)
	{
		return;
	}

	// 1발 예상 피해 = 나가는 캐릭터의 공격력 × 배율 (USDamageExecution과 같은 식)
	const UAbilitySystemComponent* ASC = SourceASC.Get();
	const float AttackPower = ASC ? ASC->GetNumericAttribute(USAttributeSet::GetAttackPowerAttribute()) : 0.f;
	const float DamagePerShot = AttackPower * DamageMultiplier;

	for (AActor* Enemy : Nearest)
	{
		if (ShotTargets.Num() >= ShotCount)
		{
			break;
		}

		// 피해를 모르면(0) 한 명에 한 발씩
		const UAbilitySystemComponent* EnemyASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Enemy);
		float RemainingHealth = EnemyASC ? EnemyASC->GetNumericAttribute(USAttributeSet::GetHealthAttribute()) : 0.f;
		do
		{
			ShotTargets.Add(Enemy);
			RemainingHealth -= DamagePerShot;
		}
		while (DamagePerShot > 0.f && RemainingHealth > 0.f && ShotTargets.Num() < ShotCount);
	}

	// 모두 쓰러뜨릴 만큼 주고도 남은 탄 → 가장 가까운 적
	while (ShotTargets.Num() < ShotCount)
	{
		ShotTargets.Add(Nearest[0]);
	}
}

float ASAssistActor::GetFanAngle(int32 ShotIndex) const
{
	// 발이 N개면 -HomingFanAngle ~ +HomingFanAngle을 고르게 나눔 (3발: -30, 0, +30). 1발이면 0
	const int32 ShotCount = SortedHitTimes.Num();
	if (ShotCount <= 1)
	{
		return 0.f;
	}
	const float Alpha = static_cast<float>(ShotIndex) / static_cast<float>(ShotCount - 1);
	return FMath::Lerp(-HomingFanAngle, HomingFanAngle, Alpha);
}

FGameplayEffectSpecHandle ASAssistActor::MakeDamageSpec(UAbilitySystemComponent* ASC) const
{
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(DamageEffectClass, 1.f, ASC->MakeEffectContext());
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(SGameplayTags::SetByCaller_DamageMultiplier, DamageMultiplier);
	}
	return Spec;
}
