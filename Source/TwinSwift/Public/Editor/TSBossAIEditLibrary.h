#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TSBossAIEditLibrary.generated.h"

class UBehaviorTree;
class UBlackboardData;
class ANavMeshBoundsVolume;

UCLASS()
class TWINSWIFT_API UTSBossAIEditLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	// BB의 두 키와 Selector + 서비스 + MoveTo를 생성. 기존 트리는 덮어쓰지 않음
	UFUNCTION(BlueprintCallable, Category = "Editor|BossAI")
	static FString ConfigureTree(UBehaviorTree* Tree, UBlackboardData* Blackboard);

	// 볼륨에 실제 박스 브러시를 만들어 내비게이션 영역으로 사용
	UFUNCTION(BlueprintCallable, Category = "Editor|BossAI")
	static bool BuildNavBox(ANavMeshBoundsVolume* Volume, FVector FullSize);
};
