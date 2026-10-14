#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "CharacterBase.generated.h"

class UAbilitySystemComponent;
class USAttributeSet;

/**
 * 플레이어·적 공통 부모
 * ASC는 플레이어면 PlayerState, 적이면 캐릭터 자신이 가짐. 여기서는 포인터만 들고 있음
 */
UCLASS(Abstract)
class TWINSWIFT_API ACharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACharacterBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	USAttributeSet* GetAttributeSet() const { return AttributeSet; }

protected:
	// ASC에 "주인(Owner)과 몸(Avatar)"을 알려 줌. 자식이 구현
	virtual void InitAbilityActorInfo() {}

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<USAttributeSet> AttributeSet;
};
