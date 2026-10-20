
#include "Character/BossCharacter.h"

#include "SGameplayTags.h"
#include "Ability/SAbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AI/SAIController.h"

ABossCharacter::ABossCharacter()
{
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	
	GetCharacterMovement()->MaxWalkSpeed = 300.f;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 180.f, 0.f);

	AIControllerClass = ASAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ABossCharacter::BeginPlay()
{
	Super::BeginPlay();

	GiveStartupAbilitiesAndAttributes();

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		GroggyTagHandle = ASC->RegisterGameplayTagEvent(SGameplayTags::State_Groggy, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &ThisClass::OnGroggyTagChanged);
	}
}

void ABossCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->RegisterGameplayTagEvent(SGameplayTags::State_Groggy, EGameplayTagEventType::NewOrRemoved).Remove(GroggyTagHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void ABossCharacter::OnGroggyTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// 붙을 때만 (떨어질 때는 할 일 없음)
	if (NewCount <= 0)
	{
		return;
	}

	// 부모 태그 하나로 보스 공격 전부 → 보스 2·3도 코드 변경 없음
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		const FGameplayTagContainer BossAttackTags(SGameplayTags::Ability_Boss);
		ASC->CancelAbilities(&BossAttackTags);
	}
}

void ABossCharacter::GiveStartupAbilitiesAndAttributes()
{
	USAbilitySystemComponent* SASC = Cast<USAbilitySystemComponent>(GetAbilitySystemComponent());
	if (!SASC ||SASC->HasGivenStartupAbilities())
	{
		return;
	}

	if (DefaultAttributesEffect)
	{
		FGameplayEffectContextHandle Context = SASC->MakeEffectContext();
		Context.AddSourceObject(this);

		const FGameplayEffectSpecHandle Spec = SASC->MakeOutgoingSpec(DefaultAttributesEffect, 1.f, Context);
		if (Spec.IsValid())
		{
			SASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
	SASC->AddStartupAbilities(StartupAbilities);
}

bool ABossCharacter::TryActivateAttack(const FGameplayTag& AbilityTag)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !AbilityTag.IsValid())
	{
		return false;
	}
	
	if (ASC->HasMatchingGameplayTag(SGameplayTags::State_Stun)
		|| ASC->HasMatchingGameplayTag(SGameplayTags::State_Groggy)
		|| ASC->HasMatchingGameplayTag(SGameplayTags::State_Dead))
	{
		return false;
	}

	return ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AbilityTag));
}

bool ABossCharacter::IsAttacking() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return ASC && ASC->HasMatchingGameplayTag(SGameplayTags::State_Attacking);
}


