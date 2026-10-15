#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TSWeaponTrailLibrary.generated.h"
class UMaterialInterface;
UCLASS()
class TWINSWIFT_API UTSWeaponTrailLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Editor|WeaponTrail")
    static FString ConfigureBladeTrail(UObject* SystemAsset);

    // 리본 수명·생성 빈도 배율 조절 (빠른 공격에서도 궤적이 촘촘하고 오래 남게)
    // 배율 1, 1이면 아무것도 안 바꾸고 이펙트의 숫자 값 목록만 돌려줌
    UFUNCTION(BlueprintCallable, Category="Editor|WeaponTrail")
    static FString TuneTrailTiming(UObject* SystemAsset, float LifetimeScale, float SpawnScale);

    UFUNCTION(BlueprintCallable, Category="Editor|WeaponTrail")
    static FString PrepareRibbon(UObject* SystemAsset, UObject* EmitterAsset, UMaterialInterface* Material);
};
