#include "Character/EnemyCharacter.h"

#include "Ability/SAbilitySystemComponent.h"
#include "Ability/SAttributeSet.h"

AEnemyCharacter::AEnemyCharacter()
{
	AbilitySystemComponent = CreateDefaultSubobject<USAbilitySystemComponent>("AbilitySystemComponent");

	AttributeSet = CreateDefaultSubobject<USAttributeSet>("AttributeSet");
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	InitAbilityActorInfo();
}

void AEnemyCharacter::InitAbilityActorInfo()
{
	// 주인과 몸이 둘 다 자기 자신
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
}
