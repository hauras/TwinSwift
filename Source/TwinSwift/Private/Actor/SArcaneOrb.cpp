
#include "Actor/SArcaneOrb.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "NiagaraComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Ability/SAbilityFunctionLibrary.h"
#include "Engine/OverlapResult.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "SGameplayTags.h"
#include "EngineUtils.h"
#include "Game/SBossEncounter.h"
#include "Ability/SAttributeSet.h"
#include "Materials/MaterialInstanceDynamic.h"

ASArcaneOrb::ASArcaneOrb()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	OrbMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OrbMesh"));
	OrbMesh->SetupAttachment(SceneRoot);
	OrbEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("OrbEffect"));
	OrbEffect->SetupAttachment(SceneRoot);
	OrbEffect->SetAutoActivate(false);

	RangeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RangeMesh"));
	RangeMesh->SetupAttachment(SceneRoot);
	RangeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RangeMesh->SetGenerateOverlapEvents(false);
	RangeMesh->SetCastShadow(false);

	OrbMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OrbMesh->SetGenerateOverlapEvents(false);
}

void ASArcaneOrb::BeginPlay()
{
	Super::BeginPlay();

	SpawnTime = GetWorld()->GetTimeSeconds();

	UAbilitySystemComponent* ASC = SourceASC.Get();
	if (!ASC || !DamageSpec.IsValid()
		|| ASC->HasMatchingGameplayTag(SGameplayTags::State_Dead))
	{
		Cancel();
		return;
	}
	SourceDeadHandle = ASC->RegisterGameplayTagEvent(
		SGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::OnSourceDead);
	for (TActorIterator<ASBossEncounter> It(GetWorld()); It; ++It)
	{
		Encounter = *It;
		EncounterHandle = It->OnEncounterStateChanged.AddUObject(this, &ThisClass::OnEncounterChanged);
		const EBossEncounterState State = It->GetState();
		if (State == EBossEncounterState::Cleared || State == EBossEncounterState::Victory
			|| State == EBossEncounterState::Defeat)
		{
			Cancel();
			return;
		}
		break;
	}

	SetupRangeIndicator();

	BaseEffectScale = OrbEffect->GetRelativeScale3D();
	BaseMeshScale = OrbMesh->GetRelativeScale3D();
	if (OrbEffect->GetAsset())
	{
		OrbMesh->SetVisibility(false);
		OrbEffect->Activate(true);
	}
	UpdateChargeVisual();
	if (ChargeDuration > 0.f)
	{
		GetWorldTimerManager().SetTimer(
			ChargeVisualTimer, this, &ThisClass::UpdateChargeVisual, 1.f / 30.f, true);
	}

	GetWorldTimerManager().SetTimer(
		ExplosionTimer,
		this,
		&ASArcaneOrb::Explode,
		FMath::Max(LifeTime, 0.01f),
		false);
}

void ASArcaneOrb::Explode()
{
	if (bResolved) return;

	bResolved = true;
	GetWorldTimerManager().ClearTimer(ExplosionTimer);

	TArray<AActor*> Targets;
	FindTargets(Targets);

	const float ChargeRatio = GetChargeRatio();
	const float DamageMultiplier = FMath::Lerp(
		MinDamageMultiplier,
		MaxDamageMultiplier,
		ChargeRatio);
	// 설치 때 저장한 원본은 유지하고, 대상마다 복사한 스펙에 충전 배율을 넣음
	if (HasAuthority() && DamageSpec.IsValid())
	{
		for (AActor* Target : Targets)
		{
			UAbilitySystemComponent* ASC = SourceASC.Get();
			if (!ASC) break;

			if (!IsValid(Target)
				|| !USAbilityFunctionLibrary::IsTargetable(ASC->GetAvatarActor(), Target))
			{
				continue;
			}

			UAbilitySystemComponent* TargetASC =
				UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
			if (!TargetASC) continue;

			FGameplayEffectSpec ExplosionSpec(*DamageSpec.Data.Get());
			ExplosionSpec.SetSetByCallerMagnitude(
				SGameplayTags::SetByCaller_DamageMultiplier,
				DamageMultiplier);

			// 무적·반격 처리는 기존 피해 경로의 AttributeSet이 담당
			const float BeforeHealth = TargetASC->GetNumericAttribute(USAttributeSet::GetHealthAttribute());
			ASC->ApplyGameplayEffectSpecToTarget(ExplosionSpec, TargetASC);
			if (IsValid(Target) && BeforeHealth > TargetASC->GetNumericAttribute(USAttributeSet::GetHealthAttribute()))
			{
				const FVector TowardOrb = (GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
				USAbilityFunctionLibrary::SpawnOneShotEffect(
					this,
					HitEffect,
					Target->GetActorLocation() + TowardOrb * 45.f,
					FRotator::ZeroRotator,
					FVector(HitEffectScale));
			}
		}
	}

	SpawnExplosionWave(ChargeRatio);

	USAbilityFunctionLibrary::SpawnOneShotEffect(
		this,
		ExplosionEffect,
		GetActorLocation(),
		FRotator::ZeroRotator,
		FVector(ExplosionEffectScale * FMath::Lerp(0.6f, 1.f, ChargeRatio)));

	Destroy();
}

void ASArcaneOrb::Cancel()
{
	if (bResolved) return;

	bResolved = true;
	Destroy();
}

void ASArcaneOrb::Initialize(UAbilitySystemComponent* InSourceASC, const FGameplayEffectSpecHandle& InDamageSpec)
{
	SourceASC = InSourceASC;
	DamageSpec = InDamageSpec;
}

void ASArcaneOrb::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(ExplosionTimer);
	if (UAbilitySystemComponent* ASC = SourceASC.Get())
	{
		ASC->RegisterGameplayTagEvent(SGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
			.Remove(SourceDeadHandle);
	}
	if (ASBossEncounter* BoundEncounter = Encounter.Get())
	{
		BoundEncounter->OnEncounterStateChanged.Remove(EncounterHandle);
	}
	GetWorldTimerManager().ClearTimer(ChargeVisualTimer);
	Super::EndPlay(EndPlayReason);
}

float ASArcaneOrb::GetChargeRatio() const
{
	const UWorld* World = GetWorld();
	if (!World) return 0.f;

	if (ChargeDuration <= 0.f) return 1.f;

	const double ElapsedTime = World->GetTimeSeconds() - SpawnTime;

	return FMath::Clamp(
		static_cast<float>(ElapsedTime / ChargeDuration),
		0.f,
		1.f);
}

void ASArcaneOrb::FindTargets(TArray<AActor*>& OutTargets) const
{
	OutTargets.Reset();

	UWorld* World = GetWorld();
	UAbilitySystemComponent* ASC = SourceASC.Get();

	if (!World || !ASC || ExplosionRadius <= 0.f) return;

	AActor* SourceActor = ASC->GetAvatarActor();
	if (!IsValid(SourceActor)) return;

	// 구체 중심에서 반경 안의 Pawn을 검색
	TArray<FOverlapResult> Overlaps;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(SourceActor);

	World->OverlapMultiByObjectType(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(ExplosionRadius),
		Params);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();

		// 아군·죽은 대상·ASC가 없는 대상 제외
		if (!USAbilityFunctionLibrary::IsTargetable(SourceActor, Target))
		{
			continue;
		}

		// 한 캐릭터의 여러 컴포넌트가 잡혀도 한 번만 추가
		OutTargets.AddUnique(Target);
	}
}

void ASArcaneOrb::OnSourceDead(FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		Cancel();
	}
}

void ASArcaneOrb::OnEncounterChanged(EBossEncounterState State)
{
	if (State == EBossEncounterState::Cleared || State == EBossEncounterState::Victory
		|| State == EBossEncounterState::Defeat)
	{
		Cancel();
	}
}

void ASArcaneOrb::UpdateChargeVisual()
{
	const float Ratio = GetChargeRatio();
	const float Growth = FMath::Lerp(0.35f, 1.f, Ratio * Ratio * (3.f - 2.f * Ratio));
	if (RangeMaterial) RangeMaterial->SetScalarParameterValue(TEXT("Charge"), Ratio);
	OrbEffect->SetRelativeScale3D(BaseEffectScale * Growth);
	OrbMesh->SetRelativeScale3D(BaseMeshScale * Growth);

	// 다 찼으면 더 키울 필요 없음
	if (Ratio >= 1.f)
	{
		GetWorldTimerManager().ClearTimer(ChargeVisualTimer);
	}
}

void ASArcaneOrb::SetupRangeIndicator()
{
	if (!RangeMesh->GetStaticMesh()) return;
	FHitResult FloorHit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	if (UAbilitySystemComponent* ASC = SourceASC.Get())
	{
		if (AActor* Avatar = ASC->GetAvatarActor()) Params.AddIgnoredActor(Avatar);
	}
	const FVector Center = GetActorLocation();
	const bool bFloor = GetWorld()->LineTraceSingleByObjectType(
		FloorHit,
		Center + FVector(0.f, 0.f, 50.f),
		Center - FVector(0.f, 0.f, 500.f),
		FCollisionObjectQueryParams(ECC_WorldStatic),
		Params);
	if (!bFloor)
	{
		RangeMesh->SetVisibility(false);
		return;
	}
	// 바닥이 없는 곳이나 다른 층에 원이 잘못 뜨지 않게 바닥 높이를 확인
	RangeMesh->SetWorldLocation(FloorHit.ImpactPoint + FVector(0.f, 0.f, 3.f));
	RangeMesh->SetWorldRotation(FRotator::ZeroRotator);
	// 엔진 Plane 한 변 = 100cm. 원의 지름을 실제 피해 지름과 일치시킴
	const float DiameterScale = FMath::Max(ExplosionRadius * 2.f / 100.f, 0.01f);
	RangeMesh->SetWorldScale3D(FVector(DiameterScale, DiameterScale, 1.f));
	RangeMaterial = RangeMesh->CreateDynamicMaterialInstance(0);
}

void ASArcaneOrb::SpawnExplosionWave(float ChargeRatio)
{
	UWorld* World = GetWorld();
	if (!World || !RangeMaterial || !RangeMaterial->Parent || !RangeMesh->GetStaticMesh()) return;

	// 구체가 사라진 뒤에도 파동은 짧게 남아야 하므로 독립 액터로 생성
	AActor* Wave = World->SpawnActor<AActor>();
	if (!Wave) return;
	UStaticMeshComponent* Plane = NewObject<UStaticMeshComponent>(Wave, TEXT("ExplosionWave"));
	Wave->SetRootComponent(Plane);
	Wave->AddInstanceComponent(Plane);
	Plane->SetMobility(EComponentMobility::Movable);
	Plane->SetStaticMesh(RangeMesh->GetStaticMesh());
	Plane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Plane->SetGenerateOverlapEvents(false);
	Plane->SetCastShadow(false);

	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(RangeMaterial->Parent, Plane);
	if (!Material)
	{
		Wave->Destroy();
		return;
	}
	Material->SetScalarParameterValue(TEXT("BlastMode"), 1.f);
	Material->SetScalarParameterValue(TEXT("StartTime"), World->GetTimeSeconds());
	Material->SetScalarParameterValue(TEXT("Strength"), FMath::Lerp(0.65f, 1.f, ChargeRatio));
	Plane->SetMaterial(0, Material);
	Plane->RegisterComponent();
	Wave->SetActorLocation(RangeMesh->GetComponentLocation() + FVector(0.f, 0.f, 1.f));
	// 충전량에 상관없이 실제 피해 지름까지 도달. 반경을 바꿔도 함께 바뀜
	const float DiameterScale = FMath::Max(ExplosionRadius * 2.f / 100.f, 0.01f);
	Wave->SetActorScale3D(FVector(DiameterScale, DiameterScale, 1.f));
	Wave->SetLifeSpan(0.35f);
}
