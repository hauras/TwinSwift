#include "Projectile/SProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Ability/SAbilityFunctionLibrary.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Pool/SActorPool.h"
#include "TimerManager.h"

namespace
{
	// 명중 후 사라질 때까지(초). 이미 나간 꼬리 입자가 꺼질 시간
	constexpr float FadeOutTime = 0.22f;
}

ASProjectile::ASProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	// 적(Pawn)은 겹침으로 감지, 벽(World)은 막힘 → 벽에서 멈추고 적은 관통 감지
	Collision = CreateDefaultSubobject<USphereComponent>("Collision");
	SetRootComponent(Collision);
	Collision->InitSphereRadius(12.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>("Movement");
	Movement->SetUpdatedComponent(Collision);
	Movement->InitialSpeed = 1800.f;
	Movement->MaxSpeed = 1800.f;
	Movement->ProjectileGravityScale = 0.f;
	Movement->bRotationFollowsVelocity = true;
	// 빠르게 날아도 얇은 적을 건너뛰지 않게 프레임을 쪼개서 이동
	Movement->bForceSubStepping = true;
	// 생성되자마자 날아가지 않음: Launch에서 켬 (풀에서 꺼낸 투사체를 다시 쏠 때도 같은 길)
	Movement->bAutoActivate = false;

	FlightEffect = CreateDefaultSubobject<UNiagaraComponent>("FlightEffect");
	FlightEffect->SetupAttachment(Collision);
}

void ASProjectile::InitializeProjectile(
	UAbilitySystemComponent* InSourceASC,
	const TArray<FGameplayEffectSpecHandle>& InSpecs)
{
	SourceASC = InSourceASC;
	Specs = InSpecs;
}

void ASProjectile::SetHomingTarget(AActor* Target)
{
	if (!Target || !Target->GetRootComponent())
	{
		return;
	}

	HomingTarget = Target;
	Movement->bIsHomingProjectile = true;
	Movement->HomingTargetComponent = Target->GetRootComponent();
	Movement->HomingAccelerationMagnitude = HomingAcceleration;
}

void ASProjectile::BeginPlay()
{
	Super::BeginPlay();

	// 이벤트 연결은 처음 한 번만 (풀에서 다시 써도 그대로)
	Collision->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnOverlap);
	Collision->OnComponentHit.AddDynamic(this, &ThisClass::OnBlocked);

	// 풀 없이 직접 만든 투사체는 예전처럼 바로 발사
	if (!bManagedByPool)
	{
		Launch();
	}
}

void ASProjectile::Launch()
{
	bLaunched = true;
	bResolved = false;
	SetActorHiddenInGame(false);

	// 명중 때 숨긴 구체 메시(블루프린트에 붙은 것)를 다시 보이게
	TArray<UStaticMeshComponent*> VisualMeshes;
	GetComponents<UStaticMeshComponent>(VisualMeshes);
	for (UStaticMeshComponent* VisualMesh : VisualMeshes)
	{
		VisualMesh->SetHiddenInGame(false);
	}

	// 쏜 캐릭터 몸에서 바로 터지지 않게
	Collision->IgnoreActorWhenMoving(GetOwner(), true);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// 앞 방향(발사 회전)으로 초기 속도
	Movement->SetUpdatedComponent(Collision);
	Movement->Velocity = GetActorForwardVector() * Movement->InitialSpeed;
	Movement->Activate(true);

	FlightEffect->Activate(true);
	if (FlightEffectRepeatInterval > 0.f && FlightEffect->GetAsset())
	{
		GetWorldTimerManager().SetTimer(FlightRepeatTimer, this, &ThisClass::SpawnFlightEffectCopy, FlightEffectRepeatInterval, true);
	}

	// 아무것도 안 맞으면 MaxLifeTime 뒤 끝
	GetWorldTimerManager().SetTimer(LifeTimer, this, &ThisClass::ReturnToPool, MaxLifeTime, false);
}

void ASProjectile::ResetForPool()
{
	bLaunched = false;
	bResolved = false;

	FTimerManager& Timers = GetWorldTimerManager();
	Timers.ClearTimer(LifeTimer);
	Timers.ClearTimer(FadeTimer);
	Timers.ClearTimer(FlightRepeatTimer);

	SetActorHiddenInGame(true);
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Collision->ClearMoveIgnoreActors();

	Movement->StopMovementImmediately();
	Movement->Deactivate();
	Movement->bIsHomingProjectile = false;
	Movement->HomingTargetComponent = nullptr;

	// 비행 이펙트는 바로 끄고, 반복으로 붙였던 복사본은 지움 (다음 발에 남지 않게)
	FlightEffect->DeactivateImmediate();
	TArray<UNiagaraComponent*> Effects;
	GetComponents<UNiagaraComponent>(Effects);
	for (UNiagaraComponent* Effect : Effects)
	{
		if (Effect != FlightEffect)
		{
			Effect->DestroyComponent();
		}
	}

	// 지난 발의 상태 비우기: 하나라도 남으면 다음 발이 물려받음
	SourceASC.Reset();
	Specs.Reset();
	PiercedActors.Reset();
	HomingTarget.Reset();
}

void ASProjectile::OnCreatedByPool()
{
	// BeginPlay가 바로 Launch하지 않게 (쏘는 쪽이 스펙을 넣은 뒤 Launch)
	bManagedByPool = true;
}

void ASProjectile::OnReleasedToPool()
{
	ResetForPool();
}

void ASProjectile::ReturnToPool()
{
	// 명중과 수명 끝이 겹쳐도 한 번만
	if (!bLaunched)
	{
		return;
	}

	if (bManagedByPool)
	{
		if (USActorPool* Pool = GetWorld()->GetSubsystem<USActorPool>())
		{
			Pool->Release(this);
			return;
		}
	}
	Destroy();
}

void ASProjectile::SpawnFlightEffectCopy()
{
	// 비행 이펙트 컴포넌트와 같은 자리·회전·크기로 붙임 → 같이 날아가고, 다 재생되면 스스로 사라짐
	UNiagaraFunctionLibrary::SpawnSystemAttached(
		FlightEffect->GetAsset(),
		Collision,
		NAME_None,
		FlightEffect->GetRelativeLocation(),
		FlightEffect->GetRelativeRotation(),
		FlightEffect->GetRelativeScale3D(),
		EAttachLocation::KeepRelativeOffset,
		true,
		ENCPoolMethod::None);
}

void ASProjectile::OnOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	// 주인·아군·죽은 적은 통과 (벽은 OnBlocked가 따로 처리)
	if (OtherActor == GetOwner() || !USAbilityFunctionLibrary::IsTargetable(GetOwner(), OtherActor))
	{
		return;
	}

	// 유도탄은 자기 표적에만 맞음 (가는 길의 다른 적은 통과). 표적이 사라졌거나 죽었으면 아무 적이나
	if (HomingTarget.IsValid()
		&& OtherActor != HomingTarget.Get()
		&& USAbilityFunctionLibrary::IsTargetable(GetOwner(), HomingTarget.Get()))
	{
		return;
	}

	FVector Point = GetActorLocation();
	if (bFromSweep && !SweepResult.bStartPenetrating)
	{
		Point = SweepResult.ImpactPoint;
	}
	else if (OtherComp)
	{
		// 처음부터 겹친 경우에도 중심이 아니라 충돌 표면에서 터지게 함
		FVector SurfacePoint;
		if (OtherComp->GetClosestPointOnCollision(Point, SurfacePoint) > 0.f)
		{
			Point = SurfacePoint;
		}
	}
	// 몸 안에 파묻히지 않게 표면에서 조금 바깥으로
	const FVector Normal = (Point - OtherActor->GetActorLocation()).GetSafeNormal();
	Point += (Normal.IsNearlyZero() ? -GetActorForwardVector() : Normal) * 15.f;
	if (bPierce)
	{
		Pierce(OtherActor, Point);
		return;
	}
	Resolve(OtherActor, Point);
}

void ASProjectile::Pierce(AActor* Target, const FVector& Point)
{
	// 피해는 권한(서버)에서만. 한 적의 컴포넌트가 여럿 겹쳐도 한 번만
	if (bResolved || !HasAuthority() || PiercedActors.Contains(Target))
	{
		return;
	}
	PiercedActors.Add(Target);

	ApplySpecsTo(Target);
	if (ImpactEffect)
	{
		USAbilityFunctionLibrary::SpawnOneShotEffect(this, ImpactEffect, Point);
	}
}

void ASProjectile::ApplySpecsTo(AActor* Target) const
{
	UAbilitySystemComponent* TargetASC = Target ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target) : nullptr;
	if (!TargetASC || !SourceASC.IsValid())
	{
		return;
	}
	for (const FGameplayEffectSpecHandle& Spec : Specs)
	{
		if (Spec.IsValid())
		{
			SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
		}
	}
}

void ASProjectile::OnBlocked(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (OtherActor == GetOwner())
	{
		return;
	}

	// 벽: 데미지 없이 이펙트만
	Resolve(nullptr, Hit.ImpactPoint);
}

void ASProjectile::Resolve(AActor* Target, const FVector& Point)
{
	// 1차 교훈: 피해는 권한(서버)에서만. 겹침·막힘이 같은 프레임에 둘 다 와도 한 번만
	if (bResolved || !HasAuthority())
	{
		return;
	}
	bResolved = true;

	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Movement->StopMovementImmediately();
	Movement->Deactivate();

	ApplySpecsTo(Target);

	if (ImpactEffect)
	{
		USAbilityFunctionLibrary::SpawnOneShotEffect(this, ImpactEffect, Point);
	}

	// 구체 메시가 명중 섬광을 가리지 않게 즉시 숨김. 꼬리만 마저 사라짐
	TArray<UStaticMeshComponent*> VisualMeshes;
	GetComponents<UStaticMeshComponent>(VisualMeshes);
	for (UStaticMeshComponent* VisualMesh : VisualMeshes)
	{
		VisualMesh->SetHiddenInGame(true);
	}

	// 새 입자만 멈추고, 이미 나간 꼬리는 잠깐 남았다가 끝 (풀에 반납 또는 지움)
	GetWorldTimerManager().ClearTimer(FlightRepeatTimer);
	GetWorldTimerManager().ClearTimer(LifeTimer);
	FlightEffect->Deactivate();
	GetWorldTimerManager().SetTimer(FadeTimer, this, &ThisClass::ReturnToPool, FadeOutTime, false);
}
