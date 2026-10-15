#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SGameplayAbility.generated.h"

/**
 * 모든 능력의 부모
 * 단계 2: 입력 태그만. 단계 4에서 BlendOut 종료, 입력 버퍼, 캔슬 윈도우를 추가
 */
UCLASS(Abstract)
class TWINSWIFT_API USGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	USGameplayAbility();

	// 이 능력을 발동하는 입력 (예: InputTag.Attack)
	UPROPERTY(EditDefaultsOnly, Category = "Input", meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};
