#pragma once

#include "CoreMinimal.h"
#include "Ability/SGameplayAbility.h"
#include "SGA_Knockback.generated.h"

class UAnimMontage;

/**
 * 밀려남 (전사·마법사 공통, 맞은 캐릭터의 ASC에서 실행)
 * 서버가 Event.Knockback 이벤트를 보내면 켜짐 (지금은 탄막 매니저: 색을 못 맞춘 충격파)
 * EventMagnitude = 밀려나는 거리(cm), Instigator = 민 쪽 → 그 반대쪽으로 루트 모션으로 밂
 * 밀리는 동안 경직 태그 → 이동 입력·회피·공격이 막힘. 탈출 수단은 교체
 */
UCLASS()
class TWINSWIFT_API USGA_Knockback : public USGameplayAbility
{
	GENERATED_BODY()

public:
	USGA_Knockback();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 밀리는 시간(초). 거리는 이벤트로 받음 → 속도 = 거리 / 이 값
	UPROPERTY(EditDefaultsOnly, Category = "Knockback", meta = (ClampMin = "0.05"))
	float KnockbackDuration = 0.25f;

	// 맞아 밀리는 모션 (없어도 밀림). 틀어만 두고 끝을 기다리지 않음
	UPROPERTY(EditDefaultsOnly, Category = "Knockback")
	TObjectPtr<UAnimMontage> KnockbackMontage;

private:
	// 미는 작업이 끝남 → 능력 끝
	UFUNCTION()
	void OnKnockbackFinished();
};
