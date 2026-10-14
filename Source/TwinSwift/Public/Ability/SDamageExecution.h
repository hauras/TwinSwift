#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "SDamageExecution.generated.h"

/**
 * 데미지 계산: 공격자 AttackPower × 스킬 배율(SetByCaller.DamageMultiplier) → 대상 IncomingDamage
 * 체력은 AttributeSet의 PostGameplayEffectExecute에서 깎음
 */
UCLASS()
class TWINSWIFT_API USDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	USDamageExecution();

	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
