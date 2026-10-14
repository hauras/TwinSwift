#include "State/SPlayerState.h"

#include "Ability/SAbilitySystemComponent.h"
#include "Ability/SAttributeSet.h"

ASPlayerState::ASPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<USAbilitySystemComponent>("AbilitySystemComponent");
	AttributeSet = CreateDefaultSubobject<USAttributeSet>("AttributeSet");
}

UAbilitySystemComponent* ASPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
