#include "UI/DamageTextActor.h"

#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"

ADamageTextActor::ADamageTextActor()
{
	PrimaryActorTick.bCanEverTick = true;
	InitialLifeSpan = 1.f;

	Text = CreateDefaultSubobject<UTextRenderComponent>("Text");
	SetRootComponent(Text);
	Text->SetHorizontalAlignment(EHTA_Center);
	Text->SetVerticalAlignment(EVRTA_TextCenter);
	Text->SetWorldSize(60.f);
	Text->SetTextRenderColor(FColor::White);
	Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ADamageTextActor::SetDamage(float Damage)
{
	Text->SetText(FText::AsNumber(FMath::RoundToInt(Damage)));
}

void ADamageTextActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AddActorWorldOffset(FVector(0.f, 0.f, RiseSpeed * DeltaSeconds));

	// 항상 카메라를 보게 (TextRender는 기본으로 +X를 향함)
	if (const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector ToCamera = CameraManager->GetCameraLocation() - GetActorLocation();
		SetActorRotation(ToCamera.Rotation());
	}
}
