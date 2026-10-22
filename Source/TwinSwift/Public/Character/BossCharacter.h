
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Character/EnemyCharacter.h"
#include "GameplayEffectTypes.h"
#include "BossCharacter.generated.h"

class UGameplayAbility;
class UGameplayEffect;
class UBehaviorTree;
class USBossPatternComponent;
class UAnimMontage;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnBossDied, ABossCharacter*);
UCLASS()
class TWINSWIFT_API ABossCharacter : public AEnemyCharacter
{
	GENERATED_BODY()
public:
	ABossCharacter();

	bool TryActivateAttack(const FGameplayTag& AbilityTag);

	bool IsAttacking() const;

	UBehaviorTree* GetBehaviorTree() const { return BehaviorTree; }

	// 패턴 선택기 (BT의 Activate Boss Pattern이 물어봄)
	USBossPatternComponent* GetPatternComponent() const { return PatternComponent; }

	bool IsDead() const;

	// 전투 시작: 행동 트리 실행 (ASBossEncounter가 부름. 그 전에는 서 있기만 함). 컨트롤러·BT가 없으면 false
	bool StartCombat();

	// 전투 멈춤: 행동 트리·진행 중인 공격·이동 정지 (플레이어 패배, 사망)
	void StopCombat();

	// HUD에 보일 이름 (골렘: "바위 골렘")
	const FText& GetDisplayName() const { return DisplayName; }

	// 등장: IntroAbilityTag 능력 발동 (골렘: 높은 곳·가장자리에서 Landing으로 뛰어내림). Landing = 바닥 위 한 점. 능력이 없으면 false
	bool PlayIntro(const FVector& Landing);

	// 실제 등장 능력이 아직 실행 중인지 확인
	bool IsIntroPlaying() const;

	// 등장 건너뛰기: 등장 능력을 끊고 Landing(바닥)에 바로 세움
	void SkipIntro(const FVector& Landing);

	// 도약 공격이 노릴 위치를 한 번만 정해 둠 (등장신 착지 등). 공격이 발동할 때 가져가고 비워짐
	bool ConsumeLeapTargetOverride(FVector& OutLocation);

	FOnBossDied OnBossDied;
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

	// 패턴 목록과 고르는 규칙 (BP에서 Patterns 입력)
	UPROPERTY(VisibleAnywhere, Category = "Boss|AI")
	TObjectPtr<USBossPatternComponent> PatternComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Death")
	TObjectPtr<UAnimMontage> DeathMontage;

	// 보스 이름 (BP에서)
	UPROPERTY(EditDefaultsOnly, Category = "Boss")
	FText DisplayName;

	// 등장 능력의 태그 (StartupAbilities에 그 능력이 있어야 함. 패턴 선택기 목록에는 넣지 않음)
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Intro", meta = (Categories = "Ability.Boss"))
	FGameplayTag IntroAbilityTag;
private:
	// ConsumeLeapTargetOverride가 가져갈 위치 (없으면 표적을 노림)
	TOptional<FVector> LeapTargetOverride;

	void GiveStartupAbilitiesAndAttributes();

	// 큰 경직(State.Groggy)이 붙으면 진행 중인 보스 공격(Ability.Boss.*) 취소
	// 작은 경직(State.Stun)은 공격을 끊지 않음 = 슈퍼아머 (산산조각을 반복해 보스를 묶어 두지 못하게)
	void OnGroggyTagChanged(const FGameplayTag Tag, int32 NewCount);

	// 체력이 바뀌면 GAS가 부름 → 0 이하면 Die (플레이어 OnSlotHealthChanged와 같은 방식)
	void OnHealthChanged(const FOnAttributeChangeData& Data);

	// 한 번만: State.Dead, 공격 취소, AI 정지, 충돌 정리, 몽타주, 알림
	void Die();

	FDelegateHandle HealthChangedHandle;
	FDelegateHandle GroggyTagHandle;
};
