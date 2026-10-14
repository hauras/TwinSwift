#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "SInputConfig.generated.h"

class UInputAction;

// 입력 액션 하나와 그 입력 태그
USTRUCT(BlueprintType)
struct FSInputActionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<const UInputAction> InputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};

/**
 * 능력용 입력 목록 (IA_Attack → InputTag.Attack ...)
 * 이동·카메라 같은 기본 입력은 컨트롤러에 직접 연결하고, 능력 입력만 여기서 태그로 바꿈
 */
UCLASS()
class TWINSWIFT_API USInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, meta = (TitleProperty = "InputTag"))
	TArray<FSInputActionConfig> AbilityInputActions;
};
