#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SGameplayAbility.generated.h"

/**
 * 모든 능력의 부모
 * - 입력 태그로 발동 (InputTag)
 * - 쿨다운: 능력마다 쿨다운 GE 에셋을 만들지 않고, 시간(CooldownDuration)과 태그(CooldownTags)만 정하면
 *   공통 쿨다운 GE(USGE_Cooldown)에 넣어 적용 (GetCooldownTags·ApplyCooldown 재정의)
 */
UCLASS(Abstract)
class TWINSWIFT_API USGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	USGameplayAbility();

	// 이 능력을 발동하는 입력 (예: InputTag.Attack)
	UPROPERTY(EditDefaultsOnly, Category = "Input", meta = (Categories = "InputTag"))
	FGameplayTag InputTag;

	virtual const FGameplayTagContainer* GetCooldownTags() const override;

	virtual void ApplyCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	// 쿨다운(초). 0이면 쿨다운 없음 (또는 Cooldown Gameplay Effect Class를 직접 넣은 경우 그쪽을 씀)
	UPROPERTY(EditDefaultsOnly, Category = "Cooldown", meta = (ClampMin = "0"))
	float CooldownDuration = 0.f;

	// 쿨다운 동안 붙는 태그 (예: Cooldown.Dodge). 이 태그가 있으면 다시 발동 못 함. HUD가 남은 시간을 읽을 때도 씀
	UPROPERTY(EditDefaultsOnly, Category = "Cooldown", meta = (Categories = "Cooldown"))
	FGameplayTagContainer CooldownTags;
};
