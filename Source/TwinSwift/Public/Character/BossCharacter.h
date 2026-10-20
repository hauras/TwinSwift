
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Character/EnemyCharacter.h"
#include "BossCharacter.generated.h"

class UGameplayAbility;
class UGameplayEffect;
class UBehaviorTree;

UCLASS()
class TWINSWIFT_API ABossCharacter : public AEnemyCharacter
{
	GENERATED_BODY()
public:
	ABossCharacter();

	bool TryActivateAttack(const FGameplayTag& AbilityTag);

	bool IsAttacking() const;

	UBehaviorTree* GetBehaviorTree() const { return BehaviorTree; }
protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|GAS")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|GAS")
	TSubclassOf<UGameplayEffect> DefaultAttributesEffect;

	// 이 보스의 행동 트리 (골렘: BT_Golem)
	UPROPERTY(EditDefaultsOnly, Category = "Boss|AI")
	TObjectPtr<UBehaviorTree> BehaviorTree;
	
private:
	void GiveStartupAbilitiesAndAttributes();

	// 큰 경직(State.Groggy)이 붙으면 진행 중인 보스 공격(Ability.Boss.*) 취소
	// 작은 경직(State.Stun)은 공격을 끊지 않음 = 슈퍼아머 (산산조각을 반복해 보스를 묶어 두지 못하게)
	void OnGroggyTagChanged(const FGameplayTag Tag, int32 NewCount);

	FDelegateHandle GroggyTagHandle;
};
