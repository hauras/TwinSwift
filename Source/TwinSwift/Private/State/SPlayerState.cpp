#include "State/SPlayerState.h"

#include "Ability/SAbilitySystemComponent.h"
#include "Ability/SAttributeSet.h"
#include "SGameplayTags.h"
#include "Character/PlayerCharacter.h"

ASPlayerState::ASPlayerState()
{
	WarriorASC = CreateDefaultSubobject<USAbilitySystemComponent>("WarriorASC");
	MageASC = CreateDefaultSubobject<USAbilitySystemComponent>("MageASC");
}

void ASPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// 능력치를 생성자에서 만들면 두 ASC가 둘 다 가져감 (ASC는 시작할 때 주인이 가진 AttributeSet을 전부 등록)
	// → ASC 시작이 끝난 지금 만들어서 각자 하나씩 직접 등록
	WarriorAttributeSet = NewObject<USAttributeSet>(this, TEXT("WarriorAttributeSet"));
	WarriorASC->AddAttributeSetSubobject(WarriorAttributeSet.Get());

	MageAttributeSet = NewObject<USAttributeSet>(this, TEXT("MageAttributeSet"));
	MageASC->AddAttributeSetSubobject(MageAttributeSet.Get());

	// 어느 캐릭터의 ASC인지 태그로 (한 번만 → 쌓이지 않음). 탄 색 막기처럼 "맞은 쪽이 누구인가"를 태그로 판단
	WarriorASC->AddLooseGameplayTag(SGameplayTags::Character_Warrior);
	MageASC->AddLooseGameplayTag(SGameplayTags::Character_Mage);
}

UAbilitySystemComponent* ASPlayerState::GetAbilitySystemComponent() const
{
	// 캐릭터가 아직 없으면(스폰 전) 전사
	const APlayerCharacter* PlayerCharacter = GetPawn<APlayerCharacter>();
	const ETwinSlot ActiveSlot = PlayerCharacter ? PlayerCharacter->GetActiveSlot() : ETwinSlot::Warrior;
	return GetSAbilitySystemComponent(ActiveSlot);
}

USAbilitySystemComponent* ASPlayerState::GetSAbilitySystemComponent(ETwinSlot Slot) const
{
	return Slot == ETwinSlot::Warrior ? WarriorASC.Get() : MageASC.Get();
}

USAttributeSet* ASPlayerState::GetAttributeSet(ETwinSlot Slot) const
{
	return Slot == ETwinSlot::Warrior ? WarriorAttributeSet.Get() : MageAttributeSet.Get();
}