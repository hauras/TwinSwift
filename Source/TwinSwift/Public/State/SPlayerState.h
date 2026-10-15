#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "Character/TwinSlot.h"
#include "SPlayerState.generated.h"

class UAbilitySystemComponent;
class USAbilitySystemComponent;
class USAttributeSet;

/**
 * GAS 소유자. ASC·AttributeSet을 캐릭터가 아니라 PlayerState에 둠 (Lyra 방식)
 * → 다시 스폰돼도 능력·스탯이 유지됨
 * 태그 캐릭터라 ASC·능력치를 캐릭터(전사·마법사)마다 하나씩 가짐 → 체력·쿨다운·상태 태그가 따로
 */
UCLASS()
class TWINSWIFT_API ASPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ASPlayerState();

	// 지금 나와 있는 캐릭터의 ASC (GAS가 "이 PlayerState의 ASC"를 찾을 때)
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// 캐릭터별 ASC·능력치
	USAbilitySystemComponent* GetSAbilitySystemComponent(ETwinSlot Slot) const;
	USAttributeSet* GetAttributeSet(ETwinSlot Slot) const;

protected:
	// 컴포넌트 준비가 끝난 뒤: 능력치를 만들어 각 ASC에 하나씩 등록
	virtual void PostInitializeComponents() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<USAbilitySystemComponent> WarriorASC;

	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<USAbilitySystemComponent> MageASC;

	UPROPERTY()
	TObjectPtr<USAttributeSet> WarriorAttributeSet;

	UPROPERTY()
	TObjectPtr<USAttributeSet> MageAttributeSet;
};
