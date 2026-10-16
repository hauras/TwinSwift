#include "Projectile/SProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Ability/SAbilityFunctionLibrary.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

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

void ASProjectile::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(MaxLifeTime);

	// 쏜 캐릭터 몸에서 바로 터지지 않게
	Collision->IgnoreActorWhenMoving(GetOwner(), true);
	Collision->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnOverlap);
	Collision->OnComponentHit.AddDynamic(this, &ThisClass::OnBlocked);
}

void ASProjectile::OnOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == GetOwner() || USAbilityFunctionLibrary::IsFriend(GetOwner(), OtherActor))
	{
		return;
	}

	Resolve(OtherActor, bFromSweep ? FVector(SweepResult.ImpactPoint) : GetActorLocation());
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

	UAbilitySystemComponent* TargetASC = Target ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target) : nullptr;
	if (TargetASC && SourceASC.IsValid())
	{
		for (const FGameplayEffectSpecHandle& Spec : Specs)
		{
			if (Spec.IsValid())
			{
				SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
			}
		}
	}

	if (ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactEffect, Point);
	}

	// 새 입자만 멈추고, 이미 나간 꼬리는 잠깐 남았다가 사라짐
	FlightEffect->Deactivate();
	SetLifeSpan(FadeOutTime);
}
