#include "Ability/SAbilitySystemComponent.h"

#include "Ability/SGameplayAbility.h"

void USAbilitySystemComponent::AddStartupAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupAbilities)
{
	// 능력 지급은 서버만. 클라는 복제로 받음
	if (!IsOwnerActorAuthoritative() || bStartupAbilitiesGiven)
	{
		return;
	}

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : StartupAbilities)
	{
		if (!AbilityClass)
		{
			continue;
		}

		FGameplayAbilitySpec AbilitySpec(AbilityClass, 1);

		// 능력에 정해 둔 입력 태그를 스펙에 붙임 → 입력이 오면 이 태그로 찾음
		if (const USGameplayAbility* SAbility = Cast<USGameplayAbility>(AbilitySpec.Ability))
		{
			if (SAbility->InputTag.IsValid())
			{
				AbilitySpec.GetDynamicSpecSourceTags().AddTag(SAbility->InputTag);
			}
		}

		GiveAbility(AbilitySpec);
	}

	bStartupAbilitiesGiven = true;
}

void USAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	ABILITYLIST_SCOPE_LOCK();
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (!AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			continue;
		}

		AbilitySpecInputPressed(AbilitySpec);

		if (AbilitySpec.IsActive())
		{
			// 이미 실행 중인 능력(예: 콤보)에 "또 눌렀다"를 알림 → WaitInputPress로 받음
			const TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
			if (Instances.Num() > 0 && Instances.Last())
			{
				const FPredictionKey PredictionKey = Instances.Last()->GetCurrentActivationInfoRef().GetActivationPredictionKey();
				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, AbilitySpec.Handle, PredictionKey);
			}
		}
		else
		{
			TryActivateAbility(AbilitySpec.Handle);
		}
	}
}

void USAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	ABILITYLIST_SCOPE_LOCK();
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (!AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			continue;
		}

		AbilitySpecInputReleased(AbilitySpec);

		if (AbilitySpec.IsActive())
		{
			// 인스턴스가 없는 능력이면 Last()가 크래시 → 있을 때만 이벤트 전달 (1차 버그)
			const TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
			if (Instances.Num() > 0 && Instances.Last())
			{
				const FPredictionKey PredictionKey = Instances.Last()->GetCurrentActivationInfoRef().GetActivationPredictionKey();
				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, AbilitySpec.Handle, PredictionKey);
			}
		}
	}
}
