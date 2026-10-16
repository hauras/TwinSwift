#pragma once

#include "CoreMinimal.h"
#include "Ability/SGameplayAbility.h"
#include "SGA_DashSlash.generated.h"

class UAnimMontage;

/**
 * 전사 돌진 베기 (키 1)
 * - 커서 쪽으로 몸을 돌리고 DashDistance만큼 DashDuration 동안 밀고 나감 (회피와 같은 루트모션 힘)
 * - 미는 동안 지나간 길에 있는 적을 한 번씩 DamageMultiplier 배율로 타격
 * - 콤보 중에 쓰면 콤보를 끊고 돌진 (CancelAbilitiesWithTag)
 * - 몽타주가 끝나기 시작(BlendOut)하고 이동도 끝나면 능력 종료
 * - 쿨다운: 기본 6초, Cooldown.Warrior.DashSlash
 */
UCLASS()
class TWINSWIFT_API USGA_DashSlash : public USGameplayAbility
{
	GENERATED_BODY()

public:
	USGA_DashSlash();

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
	// 돌진 거리(cm)
	UPROPERTY(EditDefaultsOnly, Category = "Dash", meta = (ClampMin = "0"))
	float DashDistance = 600.f;

	// 돌진에 걸리는 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "Dash", meta = (ClampMin = "0.05"))
	float DashDuration = 0.6f;

	// 데미지 배율 (공격력 × 배율)
	UPROPERTY(EditDefaultsOnly, Category = "Dash", meta = (ClampMin = "0"))
	float DamageMultiplier = 1.8f;

	// 판정 너비: 캡슐 반지름에 더하는 값(cm). 클수록 옆에 있는 적도 맞음
	UPROPERTY(EditDefaultsOnly, Category = "Dash", meta = (ClampMin = "0"))
	float HitPadding = 60.f;

	// 돌진 몽타주 (비우면 몽타주 없이 이동만)
	UPROPERTY(EditDefaultsOnly, Category = "Dash|Animation")
	TObjectPtr<UAnimMontage> DashMontage;

	// 몽타주 재생 속도 / 시작 지점(초). 긴 애니에서 미끄러지는 부분부터 빠르게 쓰려고
	UPROPERTY(EditDefaultsOnly, Category = "Dash|Animation", meta = (ClampMin = "0.1"))
	float MontagePlayRate = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Dash|Animation", meta = (ClampMin = "0"))
	float MontageStartTime = 0.f;

private:
	// 지난 검사 위치 → 지금 위치를 캡슐로 쓸어서 새로 닿은 적을 타격
	void SweepPath();

	UFUNCTION()
	void OnMoveFinished();

	UFUNCTION()
	void OnMontageEnded();

	// 몽타주와 이동이 모두 끝나면 능력 종료
	void TryEndDash();

	// 이번 돌진에서 이미 맞은 적 (한 번씩만)
	TSet<TWeakObjectPtr<AActor>> HitActors;

	// 지난번 검사한 위치
	FVector LastSweepLocation = FVector::ZeroVector;

	bool bMoveFinished = false;
	bool bMontageFinished = false;

	FTimerHandle SweepTimer;
};
