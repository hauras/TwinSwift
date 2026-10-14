#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SAbilityFunctionLibrary.generated.h"

UCLASS()
class TWINSWIFT_API USAbilityFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 같은 편인지. 팀 판정은 이 함수 하나로만 (1차에서 여기저기 따로 판정해서 어긋났음)
	// 지금 규칙: 플레이어끼리, 적끼리는 같은 편
	UFUNCTION(BlueprintPure, Category = "Combat")
	static bool IsFriend(const AActor* A, const AActor* B);
};
