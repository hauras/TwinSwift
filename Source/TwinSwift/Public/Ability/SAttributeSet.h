#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "SAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * 체력·공격력
 * 마나는 없음: 이번 설계는 스킬을 쿨다운으로만 제한
 */
UCLASS()
class TWINSWIFT_API USAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	USAttributeSet();

	// 값이 바뀌기 직전: 체력이 0~최대 체력을 넘지 않게
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	// GE 적용 직후: 받은 피해를 체력에 반영
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(BlueprintReadOnly, Category = "Vital")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(USAttributeSet, Health);

	UPROPERTY(BlueprintReadOnly, Category = "Vital")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(USAttributeSet, MaxHealth);

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData AttackPower;
	ATTRIBUTE_ACCESSORS(USAttributeSet, AttackPower);

	// 메타 속성: 데미지 계산 결과를 잠깐 담는 칸. 쓰고 바로 0으로 돌림
	UPROPERTY(BlueprintReadOnly, Category = "Meta")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(USAttributeSet, IncomingDamage);

protected:
	// 때린 플레이어 화면에 데미지 숫자
	void ShowDamageNumber(const FGameplayEffectModCallbackData& Data, float Damage) const;

};
