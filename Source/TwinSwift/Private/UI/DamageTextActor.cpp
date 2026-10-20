#include "UI/DamageTextActor.h"

#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Pool/SActorPool.h"
#include "UObject/ConstructorHelpers.h"

ADamageTextActor::ADamageTextActor()
{
	// 수명은 Tick에서 직접 셈 (InitialLifeSpan으로 지우면 풀에 반납할 수 없음)
	PrimaryActorTick.bCanEverTick = true;

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

void ADamageTextActor::SetDamage(float Damage, EDamageNumberType Type)
{
	Text->SetText(FText::AsNumber(FMath::RoundToInt(Damage)));

	// 표식 피해는 글자 알림 없이 색과 크기로만 구분 (산산조각 하늘색, 균열 폭발 주황색, 내가 맞음 빨간색)
	switch (Type)
	{
	case EDamageNumberType::Shatter:
		Text->SetTextRenderColor(FColor(35, 180, 255));
		Text->SetWorldSize(72.f);
		break;
	case EDamageNumberType::CrackBurst:
		Text->SetTextRenderColor(FColor(255, 130, 30));
		Text->SetWorldSize(68.f);
		break;
	case EDamageNumberType::PlayerHurt:
		Text->SetTextRenderColor(FColor(255, 45, 45));
		Text->SetWorldSize(64.f);
		break;
	default:
		Text->SetTextRenderColor(FColor::White);
		Text->SetWorldSize(60.f);
		break;
	}
}

void ADamageTextActor::OnAcquiredFromPool()
{
	Age = 0.f;
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);
}

void ADamageTextActor::OnReleasedToPool()
{
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
}

void ADamageTextActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 다 떴으면 반납 (풀이 꺼져 있으면 풀이 지움)
	Age += DeltaSeconds;
	if (Age >= LifeTime)
	{
		if (USActorPool* Pool = GetWorld()->GetSubsystem<USActorPool>())
		{
			Pool->Release(this);
		}
		else
		{
			Destroy();
		}
		return;
	}

	AddActorWorldOffset(FVector(0.f, 0.f, RiseSpeed * DeltaSeconds));

	// 항상 카메라를 보게 (TextRender는 기본으로 +X를 향함)
	if (const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector ToCamera = CameraManager->GetCameraLocation() - GetActorLocation();
		SetActorRotation(ToCamera.Rotation());
	}
}
