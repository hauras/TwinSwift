#pragma once

#include "CoreMinimal.h"
#include "Ability/SGameplayAbility.h"
#include "SGA_Dodge.generated.h"

class UAnimMontage;

/**
 * 회피 (전사 구르기, 마법사 순간이동을 블루프린트 설정값만 바꿔 같은 클래스로)
 * - 입력 방향으로 DodgeDistance만큼 이동. DodgeDuration > 0이면 그 시간 동안 밀고(구르기), 0이면 바로 옮김(순간이동)
 * - 시작부터 InvincibleTime 동안 무적 (State.Invincible → AttributeSet이 데미지를 무시)
 * - 다른 능력(공격)이 돌고 있으면 끊고 회피
 * - 쿨다운: 기본 1초, Cooldown.Dodge (부모 USGameplayAbility의 공통 쿨다운)
 */
UCLASS()
class TWINSWIFT_API USGA_Dodge : public USGameplayAbility
{
	GENERATED_BODY()

public:
	USGA_Dodge();

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
	// 이동 거리(cm)
	UPROPERTY(EditDefaultsOnly, Category = "Dodge", meta = (ClampMin = "0"))
	float DodgeDistance = 450.f;

	// 이동에 걸리는 시간(초). 0이면 순간이동
	UPROPERTY(EditDefaultsOnly, Category = "Dodge", meta = (ClampMin = "0"))
	float DodgeDuration = 0.4f;

	// 시작부터 무적인 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "Dodge", meta = (ClampMin = "0"))
	float InvincibleTime = 0.3f;

	// 회피 몽타주 (비우면 몽타주 없이 이동만). 몽타주가 끝나기 시작하면(BlendOut) 능력 종료
	UPROPERTY(EditDefaultsOnly, Category = "Dodge|Animation")
	TObjectPtr<UAnimMontage> DodgeMontage;

	// 몽타주 재생 속도 / 시작 지점(초). 긴 애니에서 구르는 부분만 빠르게 쓰려고
	UPROPERTY(EditDefaultsOnly, Category = "Dodge|Animation", meta = (ClampMin = "0.1"))
	float MontagePlayRate = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Dodge|Animation", meta = (ClampMin = "0"))
	float MontageStartTime = 0.f;

private:
	// 입력 방향 (입력이 없으면 캐릭터 앞)
	FVector GetDodgeDirection() const;

	// 무적 태그 붙이기 / 떼기 (붙인 경우에만 한 번 뗌 → Loose 태그가 쌓이지 않음)
	void StartInvincible();
	void StopInvincible();

	UFUNCTION()
	void OnMoveFinished();

	UFUNCTION()
	void OnMontageEnded();

	// 몽타주와 이동이 모두 끝나면 능력 종료
	void TryEndDodge();

	bool bInvincible = false;
	bool bMoveFinished = false;
	bool bMontageFinished = false;

	FTimerHandle InvincibleTimer;
};
