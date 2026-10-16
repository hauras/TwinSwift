#pragma once

#include "CoreMinimal.h"
#include "Ability/GA/SGA_CastAbility.h"
#include "Engine/TimerHandle.h"
#include "SGA_ThunderStrike.generated.h"

class UNiagaraSystem;

/**
 * 낙뢰 (키 4)
 * - 손을 내려치는 신호에 커서 위치(최대 MaxRange)에 경고 원 → StrikeDelay 뒤 번개, 반경 Radius 안의 적을 DamageMultiplier 배율로 타격
 * - 캐릭터는 RecoveryTime 뒤 움직일 수 있고, 번개가 떨어지면 능력 종료 (부모 USGA_CastAbility)
 * - [균열]이 붙은 적을 맞히면 연쇄 폭발: 단계 8
 */
UCLASS()
class TWINSWIFT_API USGA_ThunderStrike : public USGA_CastAbility
{
	GENERATED_BODY()

public:
	USGA_ThunderStrike();

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 최대 사거리(cm). 커서가 더 멀면 그 방향으로 여기까지만
	UPROPERTY(EditDefaultsOnly, Category = "Thunder", meta = (ClampMin = "0"))
	float MaxRange = 1000.f;

	// 타격 반경(cm)
	UPROPERTY(EditDefaultsOnly, Category = "Thunder", meta = (ClampMin = "0"))
	float Radius = 250.f;

	// 경고 원을 띄우고 번개가 떨어질 때까지(초). 적이 피할 시간
	UPROPERTY(EditDefaultsOnly, Category = "Thunder", meta = (ClampMin = "0"))
	float StrikeDelay = 0.5f;

	// 타격 순간 떨어지는 번개
	UPROPERTY(EditDefaultsOnly, Category = "Thunder")
	TObjectPtr<UNiagaraSystem> StrikeEffect;

	// 경고 원 이펙트. 비우면 디버그 원으로 그림
	UPROPERTY(EditDefaultsOnly, Category = "Thunder")
	TObjectPtr<UNiagaraSystem> WarningEffect;

	// 손을 내려치는 신호: 커서 위치에 경고 원 + StrikeDelay 타이머
	virtual void OnCast(const FVector& Origin, const FVector& Direction) override;

	// 번개가 떨어졌으면 true
	virtual bool IsCastFinished() const override;

private:
	// StrikeDelay 뒤에 불림: 번개 이펙트 + 범위 타격 + 종료 시도
	void Strike();

	// 번개가 떨어질 위치 (바닥)
	FVector StrikeLocation = FVector::ZeroVector;

	// 경고 → 번개까지의 타이머 (반복 안 함)
	FTimerHandle StrikeTimer;

	// 번개가 이미 떨어졌는지 (IsCastFinished가 씀)
	bool bStruck = false;
};
