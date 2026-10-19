#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pool/SPoolable.h"
#include "DamageTextActor.generated.h"

class UTextRenderComponent;

// 데미지 숫자 종류: 색·크기·위치·화면 흔들림이 달라짐 (데미지 스펙의 Damage.* 태그로 정함)
enum class EDamageNumberType : uint8
{
	Normal,
	// 산산조각 추가 피해: 하늘색
	Shatter,
	// 균열 폭발: 주황색
	CrackBurst,
};

/**
 * 데미지 숫자: 떠오르면서 1초 뒤 사라짐
 * 위젯 없이 TextRender로 만들어 에셋이 필요 없음. 꾸미기는 UI 단계에서
 * 맞을 때마다 생겨서 오브젝트 풀(USActorPool)로 다시 씀: 끝나면 지우지 않고 반납
 */
UCLASS()
class TWINSWIFT_API ADamageTextActor : public AActor, public ISPoolable
{
	GENERATED_BODY()

public:
	ADamageTextActor();

	virtual void Tick(float DeltaSeconds) override;

	void SetDamage(float Damage, EDamageNumberType Type = EDamageNumberType::Normal);

	// ISPoolable: 꺼낼 때 보이고 시간 0부터 / 넣을 때 숨기고 멈춤
	virtual void OnAcquiredFromPool() override;
	virtual void OnReleasedToPool() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Damage Text")
	TObjectPtr<UTextRenderComponent> Text;

	// 1초에 올라가는 거리
	UPROPERTY(EditDefaultsOnly, Category = "Damage Text")
	float RiseSpeed = 120.f;

	// 떠 있는 시간(초). 지나면 풀에 반납
	UPROPERTY(EditDefaultsOnly, Category = "Damage Text", meta = (ClampMin = "0.1"))
	float LifeTime = 1.f;

	// 보인 뒤 지난 시간(초)
	float Age = 0.f;
};
