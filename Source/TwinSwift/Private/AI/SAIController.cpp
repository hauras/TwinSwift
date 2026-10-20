

#include "AI/SAIController.h"

#include "Character/BossCharacter.h"

void ASAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	const ABossCharacter* Boss = Cast<ABossCharacter>(InPawn);
	if (Boss && Boss->GetBehaviorTree())
	{
		RunBehaviorTree(Boss->GetBehaviorTree());
	}
}
