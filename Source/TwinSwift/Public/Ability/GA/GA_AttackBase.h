#pragma once

#include "CoreMinimal.h"
#include "Ability/SGameplayAbility.h"
#include "GA_AttackBase.generated.h"

class UAnimMontage;
class UGameplayEffect;

/**
 * 근접 공격 하나: 커서 방향으로 몸을 돌리고 → 몽타주 재생 → 칼날 판정(Event.Montage.Hit)마다 데미지
 * 조각 4에서 콤보(섹션 이동)·입력 버퍼·캔슬을 추가
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

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// 공격력 × 이 값 = 데미지
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float DamageMultiplier = 1.f;

private:
	// 1차 교훈: 몽타주가 끝나기 시작(BlendOut)할 때 능력을 끝내야 다음 입력이 바로 먹음
	UFUNCTION()
	void OnMontageBlendOut();

	// 다른 몽타주에 끊기거나 취소됨 → 그 경로에서도 반드시 EndAbility
	UFUNCTION()
	void OnMontageCancelled();

	// 칼날 판정(ANS_WeaponTrace)이 적을 찾을 때마다 옴
	UFUNCTION()
	void OnHitEvent(FGameplayEventData Payload);

	// GetAimPoint() 방향으로 몸을 돌림
	void FaceAimPoint() const;
};
