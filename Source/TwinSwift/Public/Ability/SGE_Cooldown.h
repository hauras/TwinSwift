#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "SGE_Cooldown.generated.h"

/**
 * 공통 "시간 태그" GE: 지속 시간은 SetByCaller.Duration, 붙는 태그는 능력이 적용할 때 넣음 (DynamicGrantedTags)
 * - 쿨다운: USGameplayAbility::ApplyCooldown → 능력마다 쿨다운 GE 에셋을 만들지 않아도 됨
 * - 표식: 내려찍기의 균열(Mark.Crack) 등 일정 시간 붙는 태그
 */
UCLASS()
class TWINSWIFT_API USGE_Cooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	USGE_Cooldown();
};
