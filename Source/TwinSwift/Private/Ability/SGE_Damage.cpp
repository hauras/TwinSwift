#include "Ability/SGE_Damage.h"

#include "Ability/SDamageExecution.h"

USGE_Damage::USGE_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition DamageExecution;
	DamageExecution.CalculationClass = USDamageExecution::StaticClass();
	Executions.Add(DamageExecution);
}
