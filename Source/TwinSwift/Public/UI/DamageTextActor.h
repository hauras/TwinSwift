#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DamageTextActor.generated.h"

class UTextRenderComponent;

/**
 * 데미지 숫자: 떠오르면서 1초 뒤 사라짐
 * 위젯 없이 TextRender로 만들어 에셋이 필요 없음. 꾸미기는 UI 단계에서
 */
UCLASS()
class TWINSWIFT_API ADamageTextActor : public AActor
{
	GENERATED_BODY()

public:
	ADamageTextActor();

	virtual void Tick(float DeltaSeconds) override;

	void SetDamage(float Damage);

private:
	UPROPERTY(VisibleAnywhere, Category = "Damage Text")
	TObjectPtr<UTextRenderComponent> Text;

	// 1초에 올라가는 거리
	UPROPERTY(EditDefaultsOnly, Category = "Damage Text")
	float RiseSpeed = 120.f;
};
