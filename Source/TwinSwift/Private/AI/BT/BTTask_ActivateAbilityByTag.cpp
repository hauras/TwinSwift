#include "AI/BT/BTTask_ActivateAbilityByTag.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Character/BossCharacter.h"

UBTTask_ActivateAbilityByTag::UBTTask_ActivateAbilityByTag()
{
	NodeName = TEXT("Activate Ability By Tag");

	// TickTask를 받으려면 켜야 함 (기본은 꺼짐)
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_ActivateAbilityByTag::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* AIController = OwnerComp.GetAIOwner();
	ABossCharacter* Boss = AIController ? Cast<ABossCharacter>(AIController->GetPawn()) : nullptr;
	if (!Boss || !Boss->TryActivateAttack(AbilityTag))
	{
		return EBTNodeResult::Failed;
	}

	// 공격 중이면 끝날 때까지 이 노드에 머묾 (바로 끝나는 능력이면 기다릴 필요 없음)
	return Boss->IsAttacking() ? EBTNodeResult::InProgress : EBTNodeResult::Succeeded;
}

void UBTTask_ActivateAbilityByTag::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	const AAIController* AIController = OwnerComp.GetAIOwner();
	const ABossCharacter* Boss = AIController ? Cast<ABossCharacter>(AIController->GetPawn()) : nullptr;

	// 공격이 끝남 (State.Attacking이 사라짐). 보스가 없어져도 멈추지 않게 끝냄
	if (!Boss || !Boss->IsAttacking())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UBTTask_ActivateAbilityByTag::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* AIController = OwnerComp.GetAIOwner();
	const ABossCharacter* Boss = AIController ? Cast<ABossCharacter>(AIController->GetPawn()) : nullptr;
	if (UAbilitySystemComponent* ASC = Boss ? Boss->GetAbilitySystemComponent() : nullptr)
	{
		// 이 노드가 발동한 공격만 취소 (태그가 같은 능력)
		const FGameplayTagContainer AbilityTags(AbilityTag);
		ASC->CancelAbilities(&AbilityTags);
	}

	return EBTNodeResult::Aborted;
}

FString UBTTask_ActivateAbilityByTag::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %s"), *Super::GetStaticDescription(), *AbilityTag.ToString());
}