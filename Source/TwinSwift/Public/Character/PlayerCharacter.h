#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterBase.h"
#include "Character/TwinSlot.h"
#include "GameplayEffectTypes.h"
#include "PlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class USCameraConfig;
class UStaticMeshComponent;
class USAbilitySystemComponent;
class UNiagaraSystem;
struct FOnAttributeChangeData;

// 교체됐을 때 (새로 나온 캐릭터)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnActiveSlotChanged, ETwinSlot);

// 캐릭터 하나가 쓰러졌을 때 (쓰러진 캐릭터)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSlotDown, ETwinSlot);

// 둘 다 쓰러졌을 때 (패배)
DECLARE_MULTICAST_DELEGATE(FOnDefeated);

// 완벽 회피 슬로가 시작(true)·끝(false)될 때
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPerfectEvade, bool);

// 지금 캐릭터가 피해를 받았을 때 (피해량)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerDamaged, float);

/**
 * 플레이어 캐릭터. 액터 하나에 몸(메시) 둘이 상주하는 태그 캐릭터 (전사·마법사)
 * 캡슐·이동·카메라는 둘이 같이 쓰고, 교체하면 보이는 몸과 "지금 ASC"만 바뀜
 * ASC·능력치는 캐릭터마다 하나씩 PlayerState에 있음
 */
UCLASS()
class TWINSWIFT_API APlayerCharacter : public ACharacterBase
{
	GENERATED_BODY()

public:
	APlayerCharacter();

	// 컨트롤러가 이 캐릭터를 잡았을 때 → GAS 초기화 + 시작 능력·스탯 지급
	virtual void PossessedBy(AController* NewController) override;

	// 카메라 설정 에셋 내용을 스프링암·카메라에 적용 (내 화면에서만 의미 있음)
	void ApplyCameraConfig(const USCameraConfig* Config);

	// 걷기 켜기/끄기
	void SetWalking(bool bNewWalking);

	UCameraComponent* GetCamera() const { return Camera; }

	UStaticMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	// ---------- 태그 교체 ----------

	// 다른 캐릭터로 교체 (전사 ↔ 마법사)
	void SwapCharacter();

	// 지정한 캐릭터로 교체. 교체할 수 없으면(CanSwapTo) 아무것도 안 함. 공격 중이면 공격을 끊고 교체
	void SwapTo(ETwinSlot NewSlot);

	// 지금 이 캐릭터로 교체할 수 있는지 (이미 나와 있음·쿨다운·쓰러짐·패배면 불가)
	bool CanSwapTo(ETwinSlot NewSlot) const;

	// 교체 쿨다운 남은 시간(초). 0이면 교체 가능
	float GetSwapCooldownRemaining() const;

	float GetSwapCooldownDuration() const { return SwapCooldown; }

	// 어시스트는 교체 쿨다운과 별개이며 두 캐릭터가 공유
	float GetAssistCooldownRemaining() const;

	float GetAssistCooldownDuration() const { return AssistCooldown; }

	bool HasActiveAssist() const;


	// 체력 0으로 쓰러졌는지
	bool IsSlotDown(ETwinSlot Slot) const;

	// 둘 다 쓰러졌는지
	bool IsDefeated() const { return bDefeated; }

	ETwinSlot GetActiveSlot() const { return ActiveSlot; }

	// 지금 나와 있는 몸
	USkeletalMeshComponent* GetActiveMesh() const;

	// 캐릭터별 몸 (전사 = GetMesh, 마법사 = MageMesh)
	USkeletalMeshComponent* GetSlotMesh(ETwinSlot Slot) const;

	// 교체가 실제로 일어났을 때 알림 (HUD가 카드 강조를 바꿈)
	FOnActiveSlotChanged OnActiveSlotChanged;

	// 쓰러짐·패배 알림 (HUD가 카드·패배 표시)
	FOnSlotDown OnSlotDown;
	FOnDefeated OnDefeated;

	// ---------- 완벽 회피 (타이밍 교체) ----------

	// 완벽 회피 판정 열기: PerfectWindowTime 동안 이 ASC에 State.PerfectWindow (교체·회피 직후)
	void OpenPerfectWindow(UAbilitySystemComponent* TargetASC);

	// 판정 중에 공격을 받으면 AttributeSet이 부름 → 데미지는 이미 무시됨, 여기서 슬로 시작
	void TriggerPerfectEvade();

	// 피해를 받으면 AttributeSet이 부름 (무적으로 버린 피해는 안 부름) → 컨트롤러(빨간 숫자·흔들림) + OnPlayerDamaged(HUD 빨간 번쩍)
	void NotifyDamaged(float Damage);

	float GetPerfectWindowTime() const { return PerfectWindowTime; }

	// 완벽 회피 슬로 시작·끝 알림 (HUD가 글자 표시)
	FOnPerfectEvade OnPerfectEvade;

	// 피해 알림 (HUD가 화면 가장자리 빨간 번쩍)
	FOnPlayerDamaged OnPlayerDamaged;

protected:
	// 에디터에서 BP를 고칠 때도 불림 → 마법사 몸 위치를 전사 몸에 맞춤
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

	virtual void InitAbilityActorInfo() override;

	// 슬로 중에 PIE를 끄면 세상 속도가 느린 채로 남지 않게 되돌림
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 완벽 회피 슬로 중에는 세상 배율의 역수 (플레이어만 정상 속도)
	virtual float GetNormalTimeDilation() const override;

	// 교체가 성공한 위치에 한 번 재생. 비우면 해당 방향의 연출을 끌 수 있다.
	UPROPERTY(EditDefaultsOnly, Category = "Twin|VFX")
	TSoftObjectPtr<UNiagaraSystem> SwapToMageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Twin|VFX")
	TSoftObjectPtr<UNiagaraSystem> SwapToWarriorEffect;

	// 대기 중인 캐릭터에게 붙는 회복 GE. 교체하면 나간 쪽으로 옮겨 감
	UPROPERTY(EditDefaultsOnly, Category = "Twin")
	TSubclassOf<UGameplayEffect> BenchRegenEffect;

	// 교체 후 다시 교체할 수 있을 때까지(초). 강제 교체(쓰러짐)는 무시
	UPROPERTY(EditDefaultsOnly, Category = "Twin", meta = (ClampMin = "0"))
	float SwapCooldown = 2.f;

	// 어시스트 쿨다운(초). 교체는 막지 않고 분신만 쉼
	UPROPERTY(EditDefaultsOnly, Category = "Twin", meta = (ClampMin = "0"))
	float AssistCooldown = 6.f;

	// 교체·회피 직후 완벽 회피 판정 시간(초). 교체는 이 시간 동안 무적이기도 함
	UPROPERTY(EditDefaultsOnly, Category = "Twin|Perfect Evade", meta = (ClampMin = "0"))
	float PerfectWindowTime = 0.25f;

	// 완벽 회피 슬로: 세상 속도 배율, 실제 시간(초). 플레이어는 정상 속도로 움직임
	UPROPERTY(EditDefaultsOnly, Category = "Twin|Perfect Evade", meta = (ClampMin = "0.05", ClampMax = "1"))
	float PerfectSlowDilation = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Twin|Perfect Evade", meta = (ClampMin = "0"))
	float PerfectSlowDuration = 0.5f;

	// 전사 시작 능력·스탯
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	FTwinSlotConfig WarriorConfig;

	// 마법사 시작 능력·스탯
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	FTwinSlotConfig MageConfig;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float RunSpeed = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float WalkSpeed = 200.f;

private:
	void ApplyWalkSpeed(bool bNewWalking);

	void PlaySwapEffect(ETwinSlot NewSlot);

	// 첫 교체 순간 동기 로딩이 생기지 않도록 BeginPlay에서 읽고 참조를 유지한다.
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> LoadedSwapToMageEffect;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> LoadedSwapToWarriorEffect;

	// 두 캐릭터 모두: 시작 스탯 GE 적용 + 시작 능력 지급 (한 번만)
	void GiveStartupAbilitiesAndAttributes();

	// 캐릭터 하나의 ASC에 시작 스탯 GE 적용 + 시작 능력 지급
	void GiveSlotStartup(USAbilitySystemComponent* SASC, const FTwinSlotConfig& Config);

	// 나와 있는 몸만 보이고 움직이게, "지금 ASC·능력치"를 그 캐릭터 것으로
	void ApplyActiveSlot();

	// 대기 회복 GE를 지금 대기 중인 캐릭터에게 옮김 (쓰러진 캐릭터는 회복 안 함)
	void UpdateBenchRegen();

	// 실제 교체 (검사 없이): 상태 바꾸고 몸·ASC·회복·이펙트·알림
	void PerformSwap(ETwinSlot NewSlot);

	// 나가는 캐릭터의 분신 생성 (쿨다운이 끝났고 AssistClass가 있을 때만). SwapTo가 교체 전에 부름
	void SpawnAssist();

	// 두 캐릭터의 체력이 바뀌면 GAS가 부름 → 0이면 쓰러짐 처리
	void OnSlotHealthChanged(const FOnAttributeChangeData& Data, ETwinSlot Slot);

	// 쓰러짐: State.Dead 태그, 능력 취소, 나와 있던 캐릭터면 강제 교체, 둘 다면 패배
	void HandleSlotDown(ETwinSlot Slot);

	static ETwinSlot GetOtherSlot(ETwinSlot Slot) { return Slot == ETwinSlot::Warrior ? ETwinSlot::Mage : ETwinSlot::Warrior; }

	// 완벽 회피 판정 닫기 / 교체 무적 끝 / 슬로 끝
	void ClosePerfectWindow();
	void EndSwapInvincible();
	void EndPerfectSlow();

	// 판정·무적을 붙인 ASC (붙인 만큼만 정확히 떼려고 보관 → Loose 태그가 쌓이지 않음)
	TWeakObjectPtr<UAbilitySystemComponent> PerfectWindowASC;
	TWeakObjectPtr<UAbilitySystemComponent> SwapInvincibleASC;

	FTimerHandle PerfectWindowTimer;
	FTimerHandle SwapInvincibleTimer;
	FTimerHandle PerfectSlowTimer;
	bool bPerfectSlow = false;

	// 마지막으로 교체한 시각 (게임 시간, 초). 처음에는 쿨다운 없이 교체 가능하게 아주 옛날
	double LastSwapTime = -1000.0;

	// 마지막으로 어시스트가 나간 시각 (게임 시간, 초). 처음 교체에도 어시스트가 나가게 아주 옛날
	double LastAssistTime = -1000.0;

	bool bDefeated = false;

	// 체력 변경 알림 연결 (두 번 연결되지 않게 보관)
	FDelegateHandle WarriorHealthHandle;
	FDelegateHandle MageHealthHandle;

	// 지금 붙어 있는 대기 회복 (어느 ASC에 붙었는지 + 떼어 낼 때 쓰는 손잡이)
	FActiveGameplayEffectHandle BenchRegenHandle;
	TWeakObjectPtr<UAbilitySystemComponent> BenchRegenASC;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	// 무기 (전사 대검). 메시는 BP에서 지정, 위치·각도·크기는 스켈레톤의 WeaponSocket
	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	// 마법사 몸. 전사 몸은 ACharacter 기본 메시(GetMesh)
	UPROPERTY(VisibleAnywhere, Category = "Twin")
	TObjectPtr<USkeletalMeshComponent> MageMesh;

	// 지금 나와 있는 캐릭터 (게임 중에 바뀌는 상태라 레벨에 놓인 캐릭터에서만 보임)
	UPROPERTY(VisibleInstanceOnly, Category = "Twin")
	ETwinSlot ActiveSlot = ETwinSlot::Warrior;
};
