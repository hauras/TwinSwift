#include "Ability/SDamageExecution.h"

#include "AbilitySystemComponent.h"
#include "SGameplayTags.h"
#include "Ability/SAttributeSet.h"
#include "Character/CharacterBase.h"

namespace
{
	// 갑옷(State.Armored) 중 받는 피해 배율 / 큰 경직(State.Groggy) 중 받는 피해 배율
	constexpr float ArmoredDamageScale = 0.2f;
	constexpr float GroggyDamageScale = 1.5f;

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

	float Damage = AttackPower * Multiplier;

	// 맞는 쪽 상태: 갑옷(대지의 분노 충전 중)이면 크게 줄고, 큰 경직(갑옷이 깨진 뒤)이면 더 들어감
	// → 갑옷은 평타로 버티지 말고 표식 폭발로 깨라는 신호, 깨면 보상
	if (EvaluateParams.TargetTags)
	{
		if (EvaluateParams.TargetTags->HasTag(SGameplayTags::State_Armored))
		{
			Damage *= ArmoredDamageScale;
		}
		else if (EvaluateParams.TargetTags->HasTag(SGameplayTags::State_Groggy))
		{
			Damage *= GroggyDamageScale;
		}
	}

	// 맞는 캐릭터마다 다른 규칙 (거울 보스: 같은 몸 방어). 피해 계산은 보스 종류를 모름
	const UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	if (const ACharacterBase* TargetCharacter = TargetASC ? Cast<ACharacterBase>(TargetASC->GetAvatarActor()) : nullptr)
	{
		Damage *= TargetCharacter->GetIncomingDamageScale(ExecutionParams.GetSourceAbilitySystemComponent());
	}

	if (Damage > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			USAttributeSet::GetIncomingDamageAttribute(),
			EGameplayModOp::Additive,
			Damage));
	}
}
