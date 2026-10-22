#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "SAbilitySystemComponent.generated.h"

/**
 * 입력 태그로 능력을 발동하는 ASC
 * 능력마다 InputTag(USGameplayAbility)를 정해 두면, 그 키를 눌렀을 때 해당 능력이 발동됨
 */
UCLASS()
class TWINSWIFT_API USAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	// 시작 능력 지급. 두 번 불려도 중복 지급하지 않음
	void AddStartupAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupAbilities);

	bool HasGivenStartupAbilities() const { return bStartupAbilitiesGiven; }

	// 컨트롤러가 키 입력을 태그로 바꿔 전달
	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	// 이 입력 태그의 능력이 지금 발동 가능한지 (쿨다운·차단 태그). 발동은 하지 않음
	// "상대 캐릭터 스킬 = 교체하며 시전"에서 교체하기 전에 확인 → 쿨다운인데 교체만 되는 일 방지
	bool CanActivateAbilityByInputTag(const FGameplayTag& InputTag) const;

private:
	bool bStartupAbilitiesGiven = false;
};
