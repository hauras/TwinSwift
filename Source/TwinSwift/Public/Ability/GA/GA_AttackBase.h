#pragma once

#include "CoreMinimal.h"
#include "Ability/SGameplayAbility.h"
#include "GA_AttackBase.generated.h"

class UAnimMontage;
class UGameplayEffect;
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
};

/**
 * 근접 공격 (콤보)
 * - 누르면 커서 방향으로 돌고 현재 타의 몽타주를 재생
 * - 타가 나가는 도중 다시 누르면, 그 타가 끝나는 순간 다음 타 몽타주로 이어짐 (입력 버퍼)
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

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

private:
	// CurrentStep의 몽타주 재생 + 다음 입력 대기
	void PlayCurrentStep();

	// 1차 교훈: 몽타주가 끝나기 시작(BlendOut)할 때 판단 → 예약된 다음 타가 있으면 이어서, 없으면 능력 종료
	UFUNCTION()
	void OnStepBlendOut();

	// 다른 몽타주에 끊기거나 취소됨 → 그 경로에서도 반드시 EndAbility
	UFUNCTION()
	void OnStepCancelled();

	// 칼날 판정(ANS_WeaponTrace)이 적을 찾을 때마다 옴
	UFUNCTION()
	void OnHitEvent(FGameplayEventData Payload);

	// 공격 중에 또 눌렀음 → 다음 타 예약
	UFUNCTION()
	void OnComboInputPressed(float TimeWaited);

	// 이전 타 몽타주가 늦게 보낸 콜백인지 (현재 타 몽타주가 아직 정상 재생 중이면 그렇다)
	bool IsStaleMontageCallback() const;

	// GetAimPoint() 방향으로 몸을 돌림
	void FaceAimPoint() const;

	// 지금 재생 중인 타 (ComboSteps 인덱스)
	int32 CurrentStep = 0;

	// 다음 타가 예약됐는지
	bool bNextStepQueued = false;

	// 마지막으로 끝난 타와 그 시각 (ComboResetTime 판정용)
	int32 LastFinishedStep = INDEX_NONE;
	double LastFinishedTime = -1000.0;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitInputPress> InputTask;
};
