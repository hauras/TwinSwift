#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TSWeaponTrailLibrary.generated.h"

class UMaterialInterface;

/**
 * 에디터 전용 Niagara 도구 (Python에서 unreal.TSWeaponTrailLibrary로 부름)
 * Python으로는 Niagara 모듈 안의 숫자를 바꿀 수 없어서 C++로 둠
 */
UCLASS()
class TWINSWIFT_API UTSWeaponTrailLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    // 복사한 시스템에서 지정한 에미터만 남기고 긴 입자 수명을 제한
    UFUNCTION(BlueprintCallable, Category="Editor|WeaponTrail")
    static FString PrepareOrbEffect(UObject* SystemAsset, const TArray<FString>& KeepNames, float LifetimeCap);

    // 리본 수명·생성 빈도 배율 조절 (빠른 공격에서도 궤적이 촘촘하고 오래 남게)
    // 배율 1, 1이면 아무것도 안 바꾸고 이펙트의 숫자 값 목록만 돌려줌
    UFUNCTION(BlueprintCallable, Category="Editor|WeaponTrail")
    static FString TuneTrailTiming(UObject* SystemAsset, float LifetimeScale, float SpawnScale);

    // 이펙트 안의 색(모듈 숫자의 색 값 + 색 커브)을 색상환에서 TargetHue(0~360도)로 돌림. 밝기·채도는 그대로
    // 채도가 거의 없는 흰색·회색은 건드리지 않음. bApply = false면 바꾸지 않고 색 목록만 돌려줌
    // 예: 빨간 번개 → TargetHue 215 (파랑)
    UFUNCTION(BlueprintCallable, Category="Editor|WeaponTrail")
    static FString TintSystemHue(UObject* SystemAsset, float TargetHue, bool bApply);

    // 에미터의 입자 공간(로컬/월드) 목록을 돌려주고, bApply면 이름에 NameFilters 중 하나가 들어간 에미터를 로컬 공간으로
    // 로컬 = 입자가 이펙트를 따라 움직임 (날아가는 투사체에 붙인 반달), 월드 = 생긴 자리에 남음 (꼬리 불꽃)
    UFUNCTION(BlueprintCallable, Category="Editor|WeaponTrail")
    static FString SetEmittersLocalSpace(UObject* SystemAsset, const TArray<FString>& NameFilters, bool bApply);

    // 렌더러(스프라이트·리본·데칼·메시 덮어쓰기)가 쓰는 재질 목록을 돌려주고, bApply면 From[i]를 To[i]로 바꿈
    // 색이 재질에 박혀 있어 TintSystemHue로 안 바뀌는 부분(예: 주황 돌·바닥 자국)을 파란 복사본으로 갈아 끼울 때
    UFUNCTION(BlueprintCallable, Category="Editor|WeaponTrail")
    static FString ReplaceSystemMaterials(
        UObject* SystemAsset,
        const TArray<UMaterialInterface*>& From,
        const TArray<UMaterialInterface*>& To,
        bool bApply);
};
