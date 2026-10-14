#include "State/SPlayerState.h"

#include "Ability/SAbilitySystemComponent.h"
#include "Ability/SAttributeSet.h"

ASPlayerState::ASPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<USAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);

	// Mixed: 내 GE는 나에게 전부, 다른 사람에게는 태그·큐만 복제 (플레이어용 권장값)
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<USAttributeSet>("AttributeSet");

	// PlayerState 기본 복제 주기(1초에 1번 수준)는 GAS에 너무 느림
	SetNetUpdateFrequency(100.f);
}

UAbilitySystemComponent* ASPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
