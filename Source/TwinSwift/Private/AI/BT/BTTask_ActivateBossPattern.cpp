#include "AI/BT/BTTask_ActivateBossPattern.h"

#include "AI/SBossPatternComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Character/BossCharacter.h"

UBTTask_ActivateBossPattern::UBTTask_ActivateBossPattern()
{
	NodeName = TEXT("Activate Boss Pattern");
}

FGameplayTag UBTTask_ActivateBossPattern::ChooseAbilityTag(UBehaviorTreeComponent& OwnerComp, ABossCharacter& Boss) const
{
	USBossPatternComponent* Patterns = Boss.GetPatternComponent();
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	const AActor* Target = Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(TargetKeyName)) : nullptr;
	if (!Patterns || !Target)
	{
		return FGameplayTag();
	}

	// 바닥 평면 거리 (보스 공격 판정과 같은 기준)
	const float Distance = FVector::Dist2D(Boss.GetActorLocation(), Target->GetActorLocation());
	return Patterns->SelectPattern(Distance);
}

FString UBTTask_ActivateBossPattern::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: 패턴 선택기 (표적 %s)"), *NodeName, *TargetKeyName.ToString());
}
