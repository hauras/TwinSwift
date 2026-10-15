#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TSWeaponTrailLibrary.generated.h"

/**
 * 에디터 전용 Niagara 도구 (Python에서 unreal.TSWeaponTrailLibrary로 부름)
 * Python으로는 Niagara 모듈 안의 숫자를 바꿀 수 없어서 C++로 둠
 */
UCLASS()
class TWINSWIFT_API UTSWeaponTrailLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    // 리본 수명·생성 빈도 배율 조절 (빠른 공격에서도 궤적이 촘촘하고 오래 남게)
    // 배율 1, 1이면 아무것도 안 바꾸고 이펙트의 숫자 값 목록만 돌려줌
    UFUNCTION(BlueprintCallable, Category="Editor|WeaponTrail")
    static FString TuneTrailTiming(UObject* SystemAsset, float LifetimeScale, float SpawnScale);
};
