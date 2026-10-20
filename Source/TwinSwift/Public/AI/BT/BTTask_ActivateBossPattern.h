#pragma once

#include "CoreMinimal.h"
#include "AI/BT/BTTask_ActivateAbilityByTag.h"
#include "BTTask_ActivateBossPattern.generated.h"

/**
 * BT 작업: 보스의 패턴 선택기(USBossPatternComponent)가 고른 공격을 발동하고 끝날 때까지 기다림
 * 발동·대기·중단 처리는 부모(UBTTask_ActivateAbilityByTag) 그대로, "어떤 태그"만 선택기에 물음
 * 고를 게 없으면(전부 쿨다운·거리 밖) Failed → Selector가 다음 노드(Move To)로
 */
UCLASS(HideCategories = (Ability))
class TWINSWIFT_API UBTTask_ActivateBossPattern : public UBTTask_ActivateAbilityByTag
{
	GENERATED_BODY()

public:
	UBTTask_ActivateBossPattern();

protected:
	virtual FGameplayTag ChooseAbilityTag(UBehaviorTreeComponent& OwnerComp, ABossCharacter& Boss) const override;

	virtual FString GetStaticDescription() const override;

	// 표적을 적어 둔 블랙보드 키. 거리는 고르는 순간 직접 잼 (서비스가 0.4초마다 적는 거리보다 정확)
	UPROPERTY(EditAnywhere, Category = "Pattern")
	FName TargetKeyName = TEXT("TargetActor");
};
