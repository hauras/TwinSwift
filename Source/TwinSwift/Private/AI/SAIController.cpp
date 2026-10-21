
#include "AI/SAIController.h"

void ASAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 행동 트리는 빙의할 때 시작하지 않음 → ABossCharacter::StartCombat (전투 진행 액터 ASBossEncounter가 부름)
}
