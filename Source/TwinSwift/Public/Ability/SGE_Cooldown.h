#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "SGE_Cooldown.generated.h"

/**
 * 공통 쿨다운 GE: 지속 시간은 SetByCaller.Duration, 붙는 태그는 능력이 적용할 때 넣음
 * USGameplayAbility::ApplyCooldown이 씀 → 능력마다 쿨다운 GE 에셋을 만들지 않아도 됨
 */
UCLASS()
class TWINSWIFT_API USGE_Cooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	USGE_Cooldown();
};
