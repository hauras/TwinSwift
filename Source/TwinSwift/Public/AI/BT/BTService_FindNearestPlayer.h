
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "BTService_FindNearestPlayer.generated.h"

/**
 * 
 */
UCLASS()
class TWINSWIFT_API UBTService_FindNearestPlayer : public UBTService_BlackboardBase
{
	GENERATED_BODY()
public:
	UBTService_FindNearestPlayer();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetToSelector;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector DistanceToTargetSelector;
};
