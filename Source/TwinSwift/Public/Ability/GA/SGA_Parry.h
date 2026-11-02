#pragma once

#include "CoreMinimal.h"
#include "Ability/SGameplayAbility.h"
#include "SGA_Parry.generated.h"

class UAnimMontage;
class UCameraShakeBase;
class UNiagaraSystem;

/**
 * 전사 반격 자세 (우클릭)
 * - 누르면 자세 몽타주 + ParryWindow초 동안 State.Parrying → 그 사이 맞으면 피해를 버리고(SAttributeSet) Event.Parry.Success
 * - 성공: 반격 몽타주. 칼이 지나가는 순간(몽타주의 Event.Montage.Hit) 막을 때 CounterRange 안이던 상대에게 배율 피해, 보스면 큰 경직(NotifyCountered). 반격 동안 무적
 * - 막는 순간 연출: 이펙트 + 양쪽 잠깐 멈춤 + 화면 흔들림 + (플레이어) "반격!" 글자
 * - 실패(시간 끝): RecoveryTime 동안 빈틈 뒤 종료 (헛자세는 손해)
 * - 거울 보스도 같은 능력: 보스 자세 중에 플레이어가 때리면 보스가 받아침
 * - 쿨다운: 기본 6초, Cooldown.Warrior.Parry
 */
UCLASS()
class TWINSWIFT_API USGA_Parry : public USGameplayAbility
{
	GENERATED_BODY()

public:
	USGA_Parry();

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
	// 받아칠 수 있는 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "Parry", meta = (ClampMin = "0.05"))
	float ParryWindow = 0.8f;

	// 헛자세 뒤 빈틈(초). 이 동안은 다른 행동을 못 하고 그대로 맞음
	UPROPERTY(EditDefaultsOnly, Category = "Parry", meta = (ClampMin = "0"))
	float RecoveryTime = 0.4f;

	// 반격 베기 배율 (때린 상대에게)
	UPROPERTY(EditDefaultsOnly, Category = "Parry", meta = (ClampMin = "0"))
	float CounterMultiplier = 2.5f;

	// 반격 베기가 닿는 거리(cm). 멀리서 쏜 탄을 막았으면 피해·경직 없이 막기만
	UPROPERTY(EditDefaultsOnly, Category = "Parry", meta = (ClampMin = "0"))
	float CounterRange = 400.f;

	// 막은 순간 이펙트 (몸과 때린 쪽 사이, 가슴 높이)
	UPROPERTY(EditDefaultsOnly, Category = "Parry|Feedback")
	TObjectPtr<UNiagaraSystem> ParryEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Parry|Feedback", meta = (ClampMin = "0"))
	float ParryEffectScale = 2.f;

	// 막은 순간 멈추는 시간(실제 초). 나와 반격 거리 안의 상대 둘 다. 0이면 안 멈춤
	UPROPERTY(EditDefaultsOnly, Category = "Parry|Feedback", meta = (ClampMin = "0"))
	float ParryHitStop = 0.12f;

	// 막은 순간 화면 흔들림
	UPROPERTY(EditDefaultsOnly, Category = "Parry|Feedback")
	TSubclassOf<UCameraShakeBase> ParryShake;

	// 자세 몽타주 (칼을 몸 앞에 세움). 능력이 끝나면 멈춤
	// 재생 속도는 코드가 맞춤: 자세 + 빈틈 시간 동안 이어지게 (애니가 0.5초뿐이라 그대로 틀면 자세가 안 보임)
	UPROPERTY(EditDefaultsOnly, Category = "Parry|Animation")
	TObjectPtr<UAnimMontage> StanceMontage;

	// 자세 몽타주 시작 지점(초). 긴 애니에서 일부만 쓸 때 (막기 애니 A_Knight_Block은 0.5초짜리라 처음부터)
	UPROPERTY(EditDefaultsOnly, Category = "Parry|Animation", meta = (ClampMin = "0"))
	float StanceStartTime = 0.f;

	// 반격 베기 몽타주. 끝나기 시작하면(BlendOut) 능력 종료
	UPROPERTY(EditDefaultsOnly, Category = "Parry|Animation")
	TObjectPtr<UAnimMontage> CounterMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Parry|Animation", meta = (ClampMin = "0.1"))
	float CounterPlayRate = 1.3f;

private:
	// 자세 중에 맞음 (SAttributeSet이 보낸 Event.Parry.Success)
	UFUNCTION()
	void OnParried(FGameplayEventData Payload);

	// 받아칠 시간이 끝남 → 빈틈 뒤 종료
	void OnWindowEnded();

	UFUNCTION()
	void OnCounterMontageEnded();

	// 자세·무적 태그 붙이기 / 떼기 (붙인 경우에만 한 번 뗌 → Loose 태그가 쌓이지 않음)
	void SetParrying(bool bEnable);
	void SetInvincible(bool bEnable);

	// 막은 순간 연출: 이펙트·멈춤·흔들림·글자 (멀리서 쏜 탄을 막아도 나옴)
	void PlayParryFeedback(AActor* Attacker);

	// 칼이 지나가는 순간 (반격 몽타주의 Event.Montage.Hit) → CounterAttack
	UFUNCTION()
	void OnCounterHit(FGameplayEventData Payload);

	// 반격 대상에게 피해 + 보스면 큰 경직
	void CounterAttack(AActor* Target);

	// 막을 때 반격 거리 안이던 상대 (칼이 지나갈 때 때림. 멀리서 쏜 탄을 막았으면 비어 있음)
	TWeakObjectPtr<AActor> CounterTarget;

	bool bParrying = false;
	bool bInvincible = false;
	bool bCountered = false;

	FTimerHandle WindowTimer;
	FTimerHandle RecoveryTimer;
};
