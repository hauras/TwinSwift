#include "Ability/SDamageExecution.h"

#include "SGameplayTags.h"
#include "Ability/SAttributeSet.h"

namespace
{
	// 계산에 쓸 속성 목록 (공격자의 AttackPower를 GE를 만든 순간 값으로 캡처)
	struct FSDamageStatics
	{
		DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower);

		FSDamageStatics()
		{
			DEFINE_ATTRIBUTE_CAPTUREDEF(USAttributeSet, AttackPower, Source, true);
		}
	};

	const FSDamageStatics& DamageStatics()
	{
		static FSDamageStatics Statics;
		return Statics;
	}
}

USDamageExecution::USDamageExecution()
{
	RelevantAttributesToCapture.Add(DamageStatics().AttackPowerDef);
}

void USDamageExecution::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	FAggregatorEvaluateParameters EvaluateParams;
	EvaluateParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluateParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float AttackPower = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().AttackPowerDef, EvaluateParams, AttackPower);
	AttackPower = FMath::Max(AttackPower, 0.f);

	// 스킬마다 다른 배율. 안 넣었으면 1배
	const float Multiplier = Spec.GetSetByCallerMagnitude(SGameplayTags::SetByCaller_DamageMultiplier, false, 1.f);

	const float Damage = AttackPower * Multiplier;
	if (Damage > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			USAttributeSet::GetIncomingDamageAttribute(),
			EGameplayModOp::Additive,
			Damage));
	}
}
