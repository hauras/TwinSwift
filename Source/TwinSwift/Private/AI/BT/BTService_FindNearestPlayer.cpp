#include "AI/BT/BTService_FindNearestPlayer.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

UBTService_FindNearestPlayer::UBTService_FindNearestPlayer()
{
	NodeName = TEXT("Find Nearest Player");

	// 매 틱이 아니라 0.4초(±0.1) 간격으로 검사
	Interval = 0.4f;
	RandomDeviation = 0.1f;
}

void UBTService_FindNearestPlayer::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	const AAIController* AIController = OwnerComp.GetAIOwner();
	const APawn* OwningPawn = AIController ? AIController->GetPawn() : nullptr;
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!OwningPawn || !GameState || !BlackboardComp)
	{
		return;
	}

	AActor* ClosestActor = nullptr;
	float ClosestDistance = TNumericLimits<float>::Max();

	// 월드 전체 액터 검색(GetAllActorsOfClass) 대신 플레이어 목록만 순회 → 플레이어 수만큼만 비교
	// 플레이어 사망 검사는 플레이어 사망 처리를 만들 때 추가 (단계 9-4)
	for (const APlayerState* PlayerState : GameState->PlayerArray)
	{
		APawn* PlayerPawn = PlayerState ? PlayerState->GetPawn() : nullptr;
		if (!IsValid(PlayerPawn))
		{
			continue;
		}

		const float Distance = OwningPawn->GetDistanceTo(PlayerPawn);
		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			ClosestActor = PlayerPawn;
		}
	}

	if (!ClosestActor)
	{
		return;
	}

	BlackboardComp->SetValueAsObject(TargetToSelector.SelectedKeyName, ClosestActor);
	BlackboardComp->SetValueAsFloat(DistanceToTargetSelector.SelectedKeyName, ClosestDistance);
}