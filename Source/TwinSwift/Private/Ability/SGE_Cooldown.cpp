#include "Ability/SGE_Cooldown.h"

#include "SGameplayTags.h"

USGE_Cooldown::USGE_Cooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat Duration;
	Duration.DataTag = SGameplayTags::SetByCaller_Duration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(Duration);
}
