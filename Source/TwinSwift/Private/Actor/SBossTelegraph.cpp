#include "Actor/SBossTelegraph.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Pool/SActorPool.h"
#include "TimerManager.h"

namespace
{
	// 재질 파라미터 이름 (M_BossTelegraph와 같아야 함)
	const FName RadiusParam(TEXT("Radius"));
	const FName ForwardXParam(TEXT("ForwardX"));
	const FName ForwardYParam(TEXT("ForwardY"));
	const FName CosHalfAngleParam(TEXT("CosHalfAngle"));
	const FName ProgressParam(TEXT("Progress"));
	const FName FlashParam(TEXT("Flash"));

	// 평면 메시 한 변 길이(cm). 엔진 기본 Plane = 100
	constexpr float PlaneSize = 100.f;
}

ASBossTelegraph::ASBossTelegraph()
{
	PrimaryActorTick.bCanEverTick = false;

	Plane = CreateDefaultSubobject<UStaticMeshComponent>("Plane");
	SetRootComponent(Plane);

	// 표시만: 충돌·그림자 없음
	Plane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Plane->SetCastShadow(false);
	Plane->SetGenerateOverlapEvents(false);
}

void ASBossTelegraph::Setup(const FVector& Center, const FRotator& Facing, float Radius, float FanAngle)
{
	// 판은 좌우로만 돌림 (기울면 바닥에서 뜸)
	SetActorLocationAndRotation(
		Center + FVector(0.f, 0.f, HeightOffset),
		FRotator(0.f, Facing.Yaw, 0.f),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);

	// 판을 지름만큼 늘림 (부채꼴도 원 크기로 덮고 재질이 잘라냄)
	const float Scale = FMath::Max(Radius * 2.f / PlaneSize, 0.01f);
	SetActorScale3D(FVector(Scale, Scale, 1.f));

	if (UMaterialInstanceDynamic* Material = GetMaterial())
	{
		const FVector Forward = Facing.Vector().GetSafeNormal2D();
		Material->SetScalarParameterValue(RadiusParam, Radius);
		Material->SetScalarParameterValue(ForwardXParam, Forward.X);
		Material->SetScalarParameterValue(ForwardYParam, Forward.Y);
		// 360도면 cos(180°) = -1 → 모든 방향 통과 = 원
		Material->SetScalarParameterValue(CosHalfAngleParam, FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(FanAngle, 1.f, 360.f) * 0.5f)));
		Material->SetScalarParameterValue(ProgressParam, 0.f);
		Material->SetScalarParameterValue(FlashParam, 0.f);
	}
}

void ASBossTelegraph::SetProgress(float Progress)
{
	if (UMaterialInstanceDynamic* Material = GetMaterial())
	{
		Material->SetScalarParameterValue(ProgressParam, FMath::Clamp(Progress, 0.f, 1.f));
	}
}

void ASBossTelegraph::FlashAndRelease()
{
	if (UMaterialInstanceDynamic* Material = GetMaterial())
	{
		Material->SetScalarParameterValue(ProgressParam, 1.f);
		Material->SetScalarParameterValue(FlashParam, 1.f);
	}

	// 능력이 끝나도 번쩍은 보여야 해서 예고 액터가 스스로 반납
	GetWorldTimerManager().SetTimer(FlashTimer, this, &ThisClass::Release, FlashDuration, false);
}

void ASBossTelegraph::Release()
{
	// 이미 반납됨 (풀에 두 번 들어가면 두 공격이 같은 판을 씀)
	if (!bInUse)
	{
		return;
	}
	bInUse = false;
	GetWorldTimerManager().ClearTimer(FlashTimer);

	if (USActorPool* Pool = GetWorld()->GetSubsystem<USActorPool>())
	{
		Pool->Release(this);
	}
	else
	{
		Destroy();
	}
}

void ASBossTelegraph::OnAcquiredFromPool()
{
	bInUse = true;
	SetActorHiddenInGame(false);
}

void ASBossTelegraph::OnReleasedToPool()
{
	GetWorldTimerManager().ClearTimer(FlashTimer);
	SetActorHiddenInGame(true);
}

UMaterialInstanceDynamic* ASBossTelegraph::GetMaterial()
{
	if (!MaterialInstance && Plane)
	{
		MaterialInstance = Plane->CreateAndSetMaterialInstanceDynamic(0);
	}
	return MaterialInstance;
}
