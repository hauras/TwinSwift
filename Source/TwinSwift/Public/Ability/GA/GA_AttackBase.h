#pragma once

#include "CoreMinimal.h"
#include "Ability/SGameplayAbility.h"
#include "GA_AttackBase.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitInputPress;

// 콤보 한 타: 몽타주 + 데미지 배율
USTRUCT(BlueprintType)
struct FSComboStep
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditDefaultsOnly)
	float DamageMultiplier = 1.f;

	// 이 타의 재생 속도 (판정·칼 궤적 노티파이도 같이 빨라짐)
	UPROPERTY(EditDefaultsOnly)
	float PlayRate = 1.f;
};

/**
 * 근접 공격 (콤보)
 * - 누르면 커서 방향으로 돌고 현재 타의 몽타주를 재생
 * - 타가 나가는 도중 다시 누르면, 몽타주의 Window.Combo 창이 열리는 순간 다음 타로 (창이 없으면 그 타가 끝나는 순간)
 *   후딜(자세 되돌리기)을 끊고 잇기 때문에 연타가 빨라짐. 창 전(준비·타격)에는 끊지 않음
 * - 타가 끝난 뒤 ComboResetTime 안에 누르면 다음 타부터, 지나면 1타부터
 * - 칼날 판정(Event.Montage.Hit)마다 그 타의 배율로 데미지
 */
UCLASS()
class TWINSWIFT_API UGA_AttackBase : public USGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_AttackBase();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 순서대로 1타, 2타, 3타... 각 타는 같은 준비 자세로 시작·끝나는 애니라 이어 붙여도 튀지 않음
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TArray<FSComboStep> ComboSteps;

	// 타가 끝난 뒤 이 시간(초) 안에 다시 누르면 다음 타, 지나면 1타부터
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float ComboResetTime = 1.f;

	// 다음 타 예약이 유효한 시간(실제 초). 입력 버퍼(회피·교체·스킬 0.3초)와 같은 규칙
	// 준비 동작 초반에 누른 입력이 한참 뒤 창이 열릴 때 저절로 나가지 않게
	UPROPERTY(EditDefaultsOnly, Category = "Attack", meta = (ClampMin = "0"))
	float ComboInputBufferTime = 0.3f;

private:
	// CurrentStep의 몽타주 재생 + 다음 입력 대기
	void PlayCurrentStep();

	// 다음 타 입력 대기 등록 (이전 대기는 끝냄). 타가 바뀔 때와, 입력을 받았는데 같은 타에 머물 때 다시
	void WaitComboInput();

	// 1차 교훈: 몽타주가 끝나기 시작(BlendOut)할 때 판단 → 예약된 다음 타가 있으면 이어서, 없으면 능력 종료
	UFUNCTION()
	void OnStepBlendOut();

	// 다른 몽타주에 끊기거나 취소됨 → 그 경로에서도 반드시 EndAbility
	UFUNCTION()
	void OnStepCancelled();

	// 칼날 판정(ANS_WeaponTrace)이 적을 찾을 때마다 옴
	UFUNCTION()
	void OnHitEvent(FGameplayEventData Payload);

	// 공격 중에 또 눌렀음 → 다음 타 예약 + 창이 이미 열려 있으면 바로 다음 타
	UFUNCTION()
	void OnComboInputPressed(float TimeWaited);

	// 몽타주의 Window.Combo 창이 열림 (ANS_ActionWindow) → 예약돼 있으면 바로 다음 타
	UFUNCTION()
	void OnComboWindowOpened();

	// 입력 있음 + 창 열림 + 다음 타 있음이면 다음 타로. 입력·창 중 어느 쪽이 먼저 와도 되게 두 곳에서 부름
	// 창이 없는 몽타주는 예전처럼 블렌드 아웃에서 이어짐 (OnStepBlendOut)
	void TryAdvanceCombo();

	// 이전 타 몽타주가 늦게 보낸 콜백인지 (현재 타 몽타주가 아직 정상 재생 중이면 그렇다)
	bool IsStaleMontageCallback() const;

	// 지금 재생 중인 타 (ComboSteps 인덱스)
	int32 CurrentStep = 0;

	// 다음 타가 예약됐는지와 예약한 시각(실제 시간)
	bool bNextStepQueued = false;
	double NextStepQueuedTime = 0.0;

	// 예약이 있고 아직 만료되지 않았는지. 만료됐으면 예약을 지움
	bool HasValidQueue();

	// 마지막으로 끝난 타와 그 시각 (ComboResetTime 판정용)
	int32 LastFinishedStep = INDEX_NONE;
	double LastFinishedTime = -1000.0;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitInputPress> InputTask;
};
