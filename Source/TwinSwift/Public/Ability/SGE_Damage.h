#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "SGE_Damage.generated.h"

/**
 * 기본 데미지 GE: 즉시 적용 + USDamageExecution 계산
 * 에셋을 따로 만들지 않아도 되게 C++에 둠. 배율은 능력이 SetByCaller로 넘김
 */
UCLASS()
class TWINSWIFT_API USGE_Damage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	USGE_Damage();
};
