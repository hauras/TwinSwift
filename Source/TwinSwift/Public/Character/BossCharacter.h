
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
class UCameraShakeBase;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UNiagaraSystem;
struct FAbilityEndedData;

// 갑옷(대지의 분노) 알림 종류
enum class EBossArmorEvent : uint8
{
	// 갑옷을 두름 (충전 시작)
	Raised,
	// 표식 폭발에 한 번 맞음
	Hit,
	// 다 맞아서 깨짐 → 큰 경직
	Broken,
	// (갑옷과 별개) 반격 가능 공격을 완벽 회피로 받음 → 큰 경직. HUD가 같은 안내 글자를 써서 여기에 둠
	Countered,
	// 못 깨고 강타가 나감 (또는 충전이 끊김)
	Expired,
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnBossDied, ABossCharacter*);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnBossArmorChanged, EBossArmorEvent);
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

	// 표식 폭발(산산조각·균열 폭발)이 이 보스에게 들어감 (AttributeSet이 부름). 갑옷 중이면 깨기 횟수 1
	void NotifyMarkBurst();

	// 반격 가능 공격(충격파 등)을 플레이어가 완벽 회피로 받음 (탄막 매니저가 부름) → CounterGroggyDuration 동안 큰 경직
	void NotifyCountered();

	// 갑옷을 깨려면 남은 표식 폭발 수 / 필요한 수 (HUD)
	int32 GetArmorHitsLeft() const { return ArmorHitsLeft; }
	int32 GetArmorHitsRequired() const { return ArmorHitsRequired; }

	FOnBossDied OnBossDied;

	FOnBossArmorChanged OnArmorChanged;
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

	// ---------- 2페이즈: 대지의 분노 ----------
	// 체력이 이 비율 이하가 되면 한 번: 하던 공격을 끊고 갑옷을 두른 채 RageAbilityTag 능력(충전 → 경기장 전체 강타)
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase2", meta = (ClampMin = "0", ClampMax = "1"))
	float PhaseTwoHealthRatio = 0.4f;

	// 충전 → 강타 능력 (StartupAbilities에 있어야 함, 패턴 선택기 목록에는 넣지 않음). 비우면 2페이즈 없음
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase2", meta = (Categories = "Ability.Boss"))
	FGameplayTag RageAbilityTag;

	// 갑옷을 깨는 데 필요한 표식 폭발 수
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase2", meta = (ClampMin = "1"))
	int32 ArmorHitsRequired = 1;

	// 표식 폭발을 한 번으로 세는 최소 간격(초). 균열 연쇄처럼 한 번에 여러 번 들어와도 1번
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase2", meta = (ClampMin = "0"))
	float ArmorHitInterval = 0.5f;

	// 갑옷이 깨진 뒤 큰 경직 시간(초). 이 동안 받는 피해 증가 (SDamageExecution)
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase2", meta = (ClampMin = "0.1"))
	float GroggyDuration = 4.f;

	// 큰 경직 몽타주. 비우면 사망 몽타주를 쓰고 GroggyPoseTime에서 멈춤 (쓰러지기 직전 자세)
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase2")
	TObjectPtr<UAnimMontage> GroggyMontage;

	// 큰 경직 몽타주를 멈출 시간(초, 몽타주 시간). 0이면 멈추지 않음
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase2", meta = (ClampMin = "0"))
	float GroggyPoseTime = 0.8f;

	// 큰 경직이 끝날 때 원래 자세로 돌아오는 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase2", meta = (ClampMin = "0"))
	float GroggyRecoverBlendTime = 0.5f;

	// 대지의 분노가 끝난 뒤(깨든 못 깨든) 걷기 속도 배율
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase2", meta = (ClampMin = "0.1"))
	float PhaseTwoSpeedScale = 1.3f;

	// 갑옷 겉껍질 (메시 위에 한 겹 더 그리는 오버레이 재질, M_GolemArmor). 비우면 겉모습 변화 없음
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase2")
	TObjectPtr<UMaterialInterface> ArmorOverlayMaterial;

	// 오버레이 재질의 밝기 파라미터 이름. 남은 갑옷 비율(1 → 0)을 넣음 → 맞을수록 흐려짐
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase2")
	FName ArmorGlowParameter = TEXT("Glow");

	// 갑옷이 깨질 때 가슴 높이 이펙트 / 크기 / 높이(발밑 기준 cm)
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase2")
	TObjectPtr<UNiagaraSystem> ArmorBreakEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase2", meta = (ClampMin = "0.1"))
	float ArmorBreakEffectScale = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase2")
	float ArmorBreakEffectHeight = 250.f;

	// 갑옷이 깨질 때 화면 흔들림 (기본 USCameraShake_BossImpact)
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase2")
	TSubclassOf<UCameraShakeBase> ArmorBreakShake;

	// ---------- 반격 기회 ----------
	// 반격당했을 때 큰 경직 시간(초). 이 동안 받는 피해 증가 (SDamageExecution, 갑옷 파괴와 같은 배율). 0이면 반격 없음
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Counter", meta = (ClampMin = "0"))
	float CounterGroggyDuration = 2.5f;
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

	// ---------- 2페이즈 ----------
	// 대지의 분노 시작 (체력 알림 다음 틱): 공격 취소 → 갑옷 → 충전 능력. 능력 발동에 실패하면 갑옷 없이 2페이즈로
	void StartEarthRage();

	// 갑옷 끝: 깨졌으면(bBroken) 이펙트 + 큰 경직, 아니면 갑옷만 벗음. 어느 쪽이든 2페이즈(속도 증가)
	void EndArmor(bool bBroken);

	// 갑옷 겉모습 켜기·끄기 / 밝기를 남은 횟수에 맞춤
	void SetArmorVisible(bool bVisible);
	void RefreshArmorGlow();

	// 행동 트리·이동만 멈춤 / 전투 중이면 행동 트리를 처음부터 다시 실행 (대지의 분노·큰 경직 동안 걸어 다니지 않게)
	void StopAI();
	void ResumeAI();

	// 큰 경직: State.Groggy(공격 취소) + 행동 트리 정지 + 몽타주. Duration 뒤 EndGroggy
	void BeginGroggy(float Duration);
	void HoldGroggyPose();
	void EndGroggy();

	// 큰 경직 타이머·태그 정리 (사망·전투 중지). bStopMontage면 몽타주도 멈춤 (사망은 같은 몽타주를 이어 가서 안 멈춤)
	void ClearGroggy(bool bStopMontage);

	// 능력이 끝날 때마다 GAS가 부름 → 대지의 분노가 끝났는데 갑옷이 남아 있으면 못 깬 것
	void OnAbilityEnded(const FAbilityEndedData& Data);

	// 큰 경직에 쓰는 몽타주 (GroggyMontage, 없으면 DeathMontage)
	UAnimMontage* GetGroggyMontage() const;

	// 대지의 분노를 시작했는지 (한 번만)
	bool bRageTriggered = false;

	// 속도 증가를 넣었는지 (한 번만)
	bool bPhaseTwoApplied = false;

	// 큰 경직 중인지 (State.Groggy를 이 코드가 붙였는지)
	bool bGroggyActive = false;

	// 전투 중인지 (StartCombat ~ StopCombat). 큰 경직이 끝날 때 전투 중일 때만 행동 트리를 다시 움직임
	bool bInCombat = false;

	int32 ArmorHitsLeft = 0;

	// 마지막으로 센 표식 폭발 시각 (월드 시간)
	double LastArmorHitTime = 0.0;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ArmorOverlayInstance;

	FTimerHandle RageStartTimer;
	FTimerHandle GroggyPoseTimer;
	FTimerHandle GroggyEndTimer;
	FDelegateHandle AbilityEndedHandle;
};
