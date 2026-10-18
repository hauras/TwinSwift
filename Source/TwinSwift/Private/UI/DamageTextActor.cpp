#include "UI/DamageTextActor.h"

#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

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

	// 조명과 노출에 색이 씻기지 않도록 글자 색을 그대로 출력
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> NumberMaterial(
		TEXT("/Game/UI/Materials/M_DamageText_Unlit.M_DamageText_Unlit"));
	if (NumberMaterial.Succeeded())
	{
		Text->SetTextMaterial(NumberMaterial.Object);
	}
}

void ADamageTextActor::SetDamage(float Damage, bool bShatter)
{
	Text->SetText(FText::AsNumber(FMath::RoundToInt(Damage)));
	// 산산조각은 글자 알림 없이 색과 크기로만 구분
	Text->SetTextRenderColor(bShatter ? FColor(35, 180, 255) : FColor::White);
	Text->SetWorldSize(bShatter ? 72.f : 60.f);
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
