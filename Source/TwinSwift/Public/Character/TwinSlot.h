#pragma once

#include "CoreMinimal.h"
#include "TwinSlot.generated.h"

class UGameplayAbility;
class UGameplayEffect;

// 태그 캐릭터의 두 자리
UENUM(BlueprintType)
enum class ETwinSlot : uint8
{
	Warrior,
	Mage
};

// 캐릭터 하나(전사 또는 마법사)의 시작 능력·스탯
USTRUCT(BlueprintType)
struct FTwinSlotConfig
{
	GENERATED_BODY()

	// 시작할 때 받는 능력 (전사: GA_Warrior_Attack)
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;

	// 시작 스탯 GE (비워 두면 AttributeSet 생성자의 기본값)
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGameplayEffect> DefaultAttributesEffect;
};
