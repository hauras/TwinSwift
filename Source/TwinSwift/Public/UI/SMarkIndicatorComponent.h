#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "GameplayTagContainer.h"
#include "SMarkIndicatorComponent.generated.h"

class UAbilitySystemComponent;
class UMaterialInterface;

/**
 * 적 머리 위 표식 아이콘 ([균열], [빙결])
 * - 주인의 ASC에 "표식 태그가 붙었다/떨어졌다" 알림을 등록 → 그 순간에만 아이콘을 켜고 끔 (매 프레임 확인하지 않음)
 * - 아이콘 아래 막대 = 표식 남은 시간. 남은 시간은 아이콘이 보이는 동안만 위젯(USMarkIconWidget)이 갱신
 * - 화면 공간 위젯이라 카메라와 거리가 달라도 같은 크기
 */
UCLASS(ClassGroup = (UI), meta = (BlueprintSpawnableComponent))
class TWINSWIFT_API USMarkIndicatorComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	USMarkIndicatorComponent();

	// 표식 태그가 붙은 효과의 남은 시간 비율(0~1)과 남은 초. 없으면 false
	bool GetMarkTimeRemaining(const FGameplayTag& MarkTag, float& OutFraction, float& OutSeconds) const;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// [균열] 전용 아이콘 UI 머티리얼
	UPROPERTY(EditDefaultsOnly, Category = "Mark")
	TObjectPtr<UMaterialInterface> CrackIcon;

	// [빙결] 전용 아이콘 UI 머티리얼
	UPROPERTY(EditDefaultsOnly, Category = "Mark")
	TObjectPtr<UMaterialInterface> FrostIcon;

private:
	// 표식 태그 개수가 0 ↔ 1 이상으로 바뀔 때 ASC가 부름
	void OnMarkTagChanged(const FGameplayTag Tag, int32 NewCount);

	// 지금 붙은 표식대로 아이콘 켜고 끄기. 표식이 하나도 없으면 컴포넌트 자체를 숨김
	void RefreshIcons();

	TWeakObjectPtr<UAbilitySystemComponent> OwnerASC;

	FDelegateHandle CrackHandle;
	FDelegateHandle FrostHandle;
};
