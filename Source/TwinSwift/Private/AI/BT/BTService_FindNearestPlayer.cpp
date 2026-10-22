#include "AI/BT/BTService_FindNearestPlayer.h"

#include "AIController.h"
#include "Ability/SAbilityFunctionLibrary.h"
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
	// 때릴 수 없는 플레이어(패배해 지금 캐릭터까지 쓰러짐 등)는 제외: 판정 규칙은 IsTargetable 하나
	for (const APlayerState* PlayerState : GameState->PlayerArray)
	{
		APawn* PlayerPawn = PlayerState ? PlayerState->GetPawn() : nullptr;
		if (!IsValid(PlayerPawn) || !USAbilityFunctionLibrary::IsTargetable(OwningPawn, PlayerPawn))
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

	// 표적이 없으면 이전 표적을 지움 (남겨 두면 쓰러진·사라진 플레이어를 계속 쫓음)
	// 거리는 0으로 지우면 "바로 앞"으로 읽히니 아주 먼 값으로
	if (!ClosestActor)
	{
		BlackboardComp->ClearValue(TargetToSelector.SelectedKeyName);
		BlackboardComp->SetValueAsFloat(DistanceToTargetSelector.SelectedKeyName, TNumericLimits<float>::Max());
		return;
	}

	BlackboardComp->SetValueAsObject(TargetToSelector.SelectedKeyName, ClosestActor);
	BlackboardComp->SetValueAsFloat(DistanceToTargetSelector.SelectedKeyName, ClosestDistance);
}