
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "SAIController.generated.h"

/**
 * 
 */
UCLASS()
class TWINSWIFT_API ASAIController : public AAIController
{
	GENERATED_BODY()
protected:
	virtual void OnPossess(APawn* InPawn) override;
};
