#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "BTTask_ActivateAbilityByTag.generated.h"

/**
 * BT 작업: 태그로 보스 공격 능력을 발동하고, 공격이 끝날 때까지 기다림
 * 능력 클래스를 모르고 태그만 알아서 보스 3마리가 이 작업 하나를 같이 씀
 * 발동 실패(경직·쿨다운) → 바로 Failed, 성공 → State.Attacking이 사라지면 Succeeded
 */
UCLASS()
class TWINSWIFT_API UBTTask_ActivateAbilityByTag : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ActivateAbilityByTag();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// 공격이 끝났는지 매 틱 확인
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// BT가 이 노드를 중단하면(페이즈 전환 분기 등) 공격도 취소 → 노드를 떠났는데 공격 몽타주가 남아 미끄러지는 일이 없음
	// 평소 분기 전환으로는 중단되지 않게 BT를 짬 (거리 데코레이터 Observer Aborts = None → 시작한 공격은 끝까지)
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// BT 에디터 노드에 "Ability.Boss.Golem.Slam"처럼 태그를 보여 줌
	virtual FString GetStaticDescription() const override;

	// 발동할 능력의 태그 (능력의 Asset Tags와 같아야 함)
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (Categories = "Ability"))
	FGameplayTag AbilityTag;
};