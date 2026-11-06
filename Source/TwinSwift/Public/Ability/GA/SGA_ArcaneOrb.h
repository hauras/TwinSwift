#pragma once

#include "CoreMinimal.h"
#include "Ability/SGameplayAbility.h"
#include "SGA_ArcaneOrb.generated.h"

class ASArcaneOrb;
class UAnimMontage;

/**
 * 마법사 마력 구체 (우클릭)
 * - 첫 입력: 조준점(최대 PlacementRange)의 바닥 위에 구체 설치. 손동작 신호(Event.Montage.Hit)에 맞춰 생김
 * - 구체가 있을 때 다시 입력: 폭발 동작 후 터뜨림 (쿨다운 무시)
 * - 설치된 구체는 능력과 별개로 남음 (교체해도 유지). 충전·자동 폭발은 ASArcaneOrb가 맡음
 * - 쿨다운: 설치하는 순간 시작 (기본 6초, Cooldown.Mage.ArcaneOrb)
 */
UCLASS()
class TWINSWIFT_API USGA_ArcaneOrb : public USGameplayAbility
{
	GENERATED_BODY()

public:
	USGA_ArcaneOrb();

	// 구체가 있으면 폭발 입력이라 쿨다운을 보지 않음
	virtual bool CheckCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

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
	// 설치할 구체 (BP에서 메시·이펙트·피해 수치를 정함)
	UPROPERTY(EditDefaultsOnly, Category = "Orb")
	TSubclassOf<ASArcaneOrb> OrbClass;

	// 설치 거리(cm). 조준점이 더 멀면 이 거리까지 당김
	UPROPERTY(EditDefaultsOnly, Category = "Orb", meta = (ClampMin = "0"))
	float PlacementRange = 800.f;

	UPROPERTY(EditDefaultsOnly, Category = "Orb|Animation")
	TObjectPtr<UAnimMontage> InstallMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Orb|Animation")
	TObjectPtr<UAnimMontage> DetonateMontage;

	// 몽타주 시작 지점(초). 긴 애니에서 손동작 부분만 씀
	UPROPERTY(EditDefaultsOnly, Category = "Orb|Animation", meta = (ClampMin = "0"))
	float InstallStartTime = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Orb|Animation", meta = (ClampMin = "0"))
	float DetonateStartTime = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Orb|Animation", meta = (ClampMin = "0.1"))
	float CastPlayRate = 1.4f;

	// 발동 후 짧은 복귀 동작을 보여 준 뒤 종료
	UPROPERTY(EditDefaultsOnly, Category = "Orb|Animation", meta = (ClampMin = "0"))
	float CastRecoveryTime = 0.18f;

private:
	// 내가 설치해서 아직 안 터진 구체가 있는지
	bool HasActiveOrb() const;

	// 조준점 쪽 바닥(평평한 곳) 위 설치 위치. 못 찾으면 false
	bool FindPlacement(FTransform& OutTransform) const;

	// 손동작 신호 순간: 설치면 구체 생성 + 쿨다운, 폭발이면 구체 터뜨림 (한 번만)
	void ReleaseOrb();

	UFUNCTION()
	void OnCastEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnCastEnded();

	UFUNCTION()
	void OnCastCancelled();

	// 설치된 구체 (가리키기만 함. 터지면 비어 있음)
	TWeakObjectPtr<ASArcaneOrb> ActiveOrb;

	// 이번 시전에서 터뜨릴 구체 (시전 시작 때 고정)
	TWeakObjectPtr<ASArcaneOrb> PendingDetonation;

	// 이번 시전에서 설치할 위치 (시전 시작 때 정함)
	FTransform PendingPlacement;

	// 이번 시전이 폭발인지
	bool bDetonating = false;

	// 이번 시전에서 설치·폭발을 이미 했는지
	bool bReleased = false;

	FTimerHandle RecoveryTimer;
};
