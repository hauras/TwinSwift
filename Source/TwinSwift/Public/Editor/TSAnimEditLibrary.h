#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TSAnimEditLibrary.generated.h"

class UBlendSpace;

/**
 * 에디터 전용 애니메이션 도구 (Python에서 unreal.TSAnimEditLibrary로 부름)
 * Python으로 블렌드스페이스 샘플을 넣으면 블렌드 계산용 데이터가 다시 만들어지지 않아서 C++로 둠
 */
UCLASS()
class TWINSWIFT_API UTSAnimEditLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 샘플 확인 + 블렌드 계산용 데이터(구간·삼각형) 다시 만들기. 에디터에서 샘플을 옮겼을 때와 같은 처리
	// 이걸 안 하면 샘플은 보이는데 게임에서 블렌드가 안 됨 (예: 걸어도 서 있기 자세)
	UFUNCTION(BlueprintCallable, Category = "Editor|Animation")
	static bool RebuildBlendSpace(UBlendSpace* BlendSpace);
};
