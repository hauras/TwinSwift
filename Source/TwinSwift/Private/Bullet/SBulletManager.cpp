
#include "Bullet/SBulletManager.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "EngineUtils.h"
#include "SGameplayTags.h"
#include "Ability/SAbilityFunctionLibrary.h"
#include "Ability/SGE_Damage.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

ASBulletManager::ASBulletManager()
{
	PrimaryActorTick.bCanEverTick = true;

	BulletMeshes = CreateDefaultSubobject<UInstancedStaticMeshComponent>("BulletMeshes");
	SetRootComponent(BulletMeshes);
	BulletMeshes->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BulletMeshes->SetCastShadow(false);

	// 매 프레임 인스턴스 위치를 바꾸므로 움직이는 컴포넌트로
	BulletMeshes->SetMobility(EComponentMobility::Movable);

	// 인스턴스마다 float 1개 (색). 재질의 PerInstanceCustomData 0번
	BulletMeshes->SetNumCustomDataFloats(1);
	DamageEffectClass = USGE_Damage::StaticClass();
}

ASBulletManager* ASBulletManager::Get(const UObject* WorldContext)
{
	if (!GEngine)
	{
		return nullptr;
	}
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull);
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ASBulletManager> It(World); It; ++It)
	{
		return *It;
	}

	return nullptr;
}

void ASBulletManager::SpawnBullet(
	const FVector& Location,
	const FVector& Velocity,
	EBulletColor Color,
	UAbilitySystemComponent* SourceASC,
	float DamageMultiplier,
	float LifeTime,
	float KnockbackStrength)
{
	// 도는 중에 쏴서 아직 배열에 안 들어간 탄도 같이 셈 (안 세면 그 순간 한도를 넘음)
	if (Bullets.Num() + PendingBullets.Num() >= MaxBullets)
	{
		return;
	}

	// 도는 중이면 따로 모았다가 끝나고 넣음 (배열이 늘며 메모리를 옮기면 돌던 참조가 깨짐)
	FSBullet& Bullet = bIteratingBullets ? PendingBullets.AddDefaulted_GetRef() : Bullets.AddDefaulted_GetRef();
	Bullet.Location = Location;
	Bullet.Velocity = Velocity;
	Bullet.Color = Color;
	Bullet.DamageMultiplier = DamageMultiplier;
	Bullet.SourceASC = SourceASC;
	Bullet.LifeRemaining = LifeTime > 0.f ? LifeTime : BulletLifeTime;
	Bullet.Radius = BulletRadius;
	Bullet.KnockbackStrength = KnockbackStrength;
}

void ASBulletManager::ClearAllBullets()
{
	// 충돌 처리 안(피해 → 패배 → 전투 끝)에서 불리면 지금 지우지 않음: 돌아간 루프가 없는 칸을 지우다 멈춤
	if (bIteratingBullets)
	{
		bClearRequested = true;
		return;
	}

	bClearRequested = false;
	PendingBullets.Reset();
	Bullets.Reset();
	LastHitTimes.Reset();
	UpdateInstances();
}

void ASBulletManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Insights에서 이 이름으로 보임. 측정 명령은 LastUpdateSeconds로 평균을 냄
	TRACE_CPUPROFILER_EVENT_SCOPE(BulletManagerUpdate);
	const double Start = FPlatformTime::Seconds();

	MoveBullets(DeltaTime);
	HitPlayers();
	UpdateInstances();

	LastUpdateSeconds = FPlatformTime::Seconds() - Start;
}

void ASBulletManager::MoveBullets(float DeltaSeconds)
{
	LastMoveDeltaSeconds = DeltaSeconds;
	for (FSBullet& Bullet : Bullets)
	{
		Bullet.Location += DeltaSeconds * Bullet.Velocity;
		Bullet.LifeRemaining -= DeltaSeconds;
	}
}

void ASBulletManager::HitPlayers()
{
	TArray<APawn*, TInlineAllocator<2>> Targets;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APawn* Pawn = It->Get() ? It->Get()->GetPawn() : nullptr;

		if (Pawn)
		{
			Targets.Add(Pawn);
		}
	}

	// 피해 처리가 즉시 실행돼서 TryHit 안에서 패배·전투 끝까지 갈 수 있음 → 도는 동안은 배열을 지우거나 늘리지 않음
	bIteratingBullets = true;
	for (int32 Index = Bullets.Num() - 1; Index >= 0 && !bClearRequested; --Index)
	{
		FSBullet& Bullet = Bullets[Index];
		if (Bullet.LifeRemaining <= 0.f)
		{
			RemoveBulletAt(Index);
			continue;
		}

		for (APawn* Target : Targets)
		{
			if (TryHit(Bullet, Target))
			{
				RemoveBulletAt(Index);
				break;
			}
		}
	}
	bIteratingBullets = false;

	// 도는 중에 생긴 일을 이제 처리
	if (bClearRequested)
	{
		ClearAllBullets();
	}
	else if (PendingBullets.Num() > 0)
	{
		Bullets.Append(PendingBullets);
		PendingBullets.Reset();
	}
}

bool ASBulletManager::TryHit(FSBullet& Bullet, APawn* Target)
{
	// 1. 닿았나: 수평 원 + 높이 범위 (캡슐이 위아래로 길어서). 제곱끼리 비교해 제곱근을 피함
	// 지금 위치만 보면 빠른 탄·프레임 튐에서 한 번에 몸을 건너뜀 → 이번 프레임 이동 선분(이전 위치 → 지금 위치)에서 몸에 가장 가까운 점으로
	const ACharacter* Character = Cast<ACharacter>(Target);
	const UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;

	if (!Capsule)
	{
		return false;
	}

	const FVector BodyLocation = Character->GetActorLocation();
	const FVector PreviousLocation = Bullet.Location - Bullet.Velocity * LastMoveDeltaSeconds;
	const FVector ClosestOnPath = FMath::ClosestPointOnSegment(
		FVector(BodyLocation.X, BodyLocation.Y, 0.f),
		FVector(PreviousLocation.X, PreviousLocation.Y, 0.f),
		FVector(Bullet.Location.X, Bullet.Location.Y, 0.f));
	const float ReachRadius = Bullet.Radius + Capsule->GetScaledCapsuleRadius();
	if (FVector::DistSquared2D(ClosestOnPath, BodyLocation) > FMath::Square(ReachRadius))
	{
		return false;
	}
	if (FMath::Abs(Bullet.Location.Z - BodyLocation.Z) > Capsule->GetScaledCapsuleHalfHeight() + Bullet.Radius)
	{
		return false;
	}

	// 2. 맞는 쪽 ASC는 피해 전에 잡아 둠 (이 피해로 쓰러지면 강제 교체돼 "지금 ASC"가 바뀜)
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetASC)
	{
		return false;
	}

	// 쏜 보스가 사라졌으면 이 탄은 그냥 지움
	UAbilitySystemComponent* SourceASC = Bullet.SourceASC.Get();
	if (!SourceASC)
	{
		return true;
	}

	// 3. 같은 편·쓰러진 캐릭터는 통과 (보스·플레이어 공격과 같은 기준)
	if (!USAbilityFunctionLibrary::IsTargetable(SourceASC->GetAvatarActor(), Target))
	{
		return false;
	}

	// 4. 색으로 막기: 주황은 전사, 파랑은 마법사가 막음 → 피해 없이 탄만 사라짐
	const bool bBlockedByColor =
		(Bullet.Color == EBulletColor::Orange && TargetASC->HasMatchingGameplayTag(SGameplayTags::Character_Warrior))
		|| (Bullet.Color == EBulletColor::Blue && TargetASC->HasMatchingGameplayTag(SGameplayTags::Character_Mage));
	if (bBlockedByColor)
	{
		return true;
	}

	// 5. 방금 맞았으면 이번 탄은 통과 (탄 여러 발이 겹쳐 한 번에 여러 번 맞지 않게)
	const double Now = GetWorld()->GetTimeSeconds();
	const double* LastHitTime = LastHitTimes.Find(TargetASC);
	if (LastHitTime && Now - *LastHitTime < HitCooldown)
	{
		return false;
	}

	// 6. 무적이었는지 피해 전에 봐 둠
	const bool bInvincible = TargetASC->HasMatchingGameplayTag(SGameplayTags::State_Invincible);

	// 7. 피해 GE: 무적이어도 넣음 → AttributeSet이 피해는 버리고 완벽 회피(슬로)는 판정
	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.f, Context);
	if (!Spec.IsValid())
	{
		return true;
	}
	Spec.Data->SetSetByCallerMagnitude(SGameplayTags::SetByCaller_DamageMultiplier, Bullet.DamageMultiplier);
	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	LastHitTimes.Add(TargetASC, Now);

	// 8. 밀어내기: 매니저는 신호만 보내고 미는 건 맞은 캐릭터의 능력(SGA_Knockback)이 함
	// 무적이면 통과라 안 밂. 방금 피해로 쓰러졌을 수 있어 GE 뒤에 다시 확인
	// 잡아 둔 TargetASC에 직접 보냄 (액터로 보내면 "지금 ASC"를 새로 찾아서, 강제 교체 뒤면 다른 캐릭터가 밀림)
	if (Bullet.KnockbackStrength > 0.f
		&& !bInvincible
		&& !TargetASC->HasMatchingGameplayTag(SGameplayTags::State_Dead))
	{
		FGameplayEventData Payload;
		Payload.EventTag = SGameplayTags::Event_Knockback;
		Payload.Instigator = SourceASC->GetAvatarActor();
		Payload.Target = Target;
		Payload.EventMagnitude = Bullet.KnockbackStrength;
		TargetASC->HandleGameplayEvent(SGameplayTags::Event_Knockback, &Payload);
	}

	// 9. 무적이면 탄 통과 (교체 무적으로 탄 벽을 뚫음), 아니면 몸에 맞고 사라짐
	return !bInvincible;
}

void ASBulletManager::RemoveBulletAt(int32 Index)
{
	Bullets.RemoveAtSwap(Index, 1, EAllowShrinking::No);
}

void ASBulletManager::UpdateInstances()
{
	// 남은 탄 위치 → 변환 배열 (멤버를 재사용해 매 프레임 할당하지 않음). 엔진 구는 지름 100cm → 크기 = 반경 × 2 / 100
	InstanceTransforms.Reset();
	InstanceColors.Reset();
	for (const FSBullet& Bullet : Bullets)
	{
		InstanceTransforms.Emplace(FRotator::ZeroRotator, Bullet.Location, FVector(Bullet.Radius / 50.f));
		InstanceColors.Add(Bullet.Color == EBulletColor::Blue ? 1.f : 0.f);
	}

	// 인스턴스 수는 늘리기만 함 (매 프레임 추가·삭제는 비쌈)
	const int32 InstanceCount = BulletMeshes->GetInstanceCount();
	if (InstanceTransforms.Num() > InstanceCount)
	{
		// 모자란 만큼만 추가 (월드 좌표)
		TArray<FTransform> Added(InstanceTransforms.GetData() + InstanceCount, InstanceTransforms.Num() - InstanceCount);
		BulletMeshes->AddInstances(Added, false, true);
	}
	else
	{
		// 남는 칸은 크기 0으로 숨김
		const FTransform Hidden(FRotator::ZeroRotator, GetActorLocation(), FVector::ZeroVector);
		while (InstanceTransforms.Num() < InstanceCount)
		{
			InstanceTransforms.Add(Hidden);
		}
	}

	// 색도 인스턴스 수만큼 (숨긴 칸은 아무 값)
	InstanceColors.SetNumZeroed(InstanceTransforms.Num());

	// 전부 한 번에 넘김. 색 먼저(렌더 갱신 없이) → 위치(시작 번호 0, 월드 좌표, 렌더 갱신, 순간이동)
	if (InstanceTransforms.Num() > 0)
	{
		BulletMeshes->SetCustomData(0, InstanceColors.Num() - 1, InstanceColors, false);
		BulletMeshes->BatchUpdateInstancesTransforms(0, InstanceTransforms, true, true, true);
	}
}


