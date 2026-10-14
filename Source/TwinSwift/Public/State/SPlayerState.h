#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "SPlayerState.generated.h"

class UAbilitySystemComponent;
class USAbilitySystemComponent;
class USAttributeSet;

/**
 * GAS 소유자. ASC·AttributeSet을 캐릭터가 아니라 PlayerState에 둠 (Lyra 방식)
 * → 죽거나 다시 스폰돼도 능력·스탯이 유지됨
 */
UCLASS()
class TWINSWIFT_API ASPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ASPlayerState();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	USAbilitySystemComponent* GetSAbilitySystemComponent() const { return AbilitySystemComponent; }

	USAttributeSet* GetAttributeSet() const { return AttributeSet; }

private:
	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<USAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<USAttributeSet> AttributeSet;
};
