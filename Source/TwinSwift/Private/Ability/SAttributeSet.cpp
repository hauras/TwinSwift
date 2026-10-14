#include "Ability/SAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Controller/SPlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

USAttributeSet::USAttributeSet()
{
	// 시작 값. 캐릭터마다 다르게 하려면 PlayerCharacter의 DefaultAttributesEffect(GE)로 덮어씀
	InitMaxHealth(100.f);
	InitHealth(100.f);
	InitAttackPower(10.f);
}

void USAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
}

void USAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float Damage = GetIncomingDamage();
		SetIncomingDamage(0.f);

		if (Damage > 0.f)
		{
			SetHealth(FMath::Clamp(GetHealth() - Damage, 0.f, GetMaxHealth()));

			ShowDamageNumber(Data, Damage);

			// 사망·피격 반응은 이후 조각에서
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// 회복 GE 등으로 직접 바뀐 경우에도 범위 유지
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}
}

void USAttributeSet::ShowDamageNumber(const FGameplayEffectModCallbackData& Data, float Damage) const
{
	// 때린 플레이어 찾기: 능력이 만든 GE는 Causer = 캐릭터, Instigator = PlayerState
	const FGameplayEffectContextHandle& Context = Data.EffectSpec.GetContext();

	ASPlayerController* SourceController = nullptr;
	if (const APawn* CauserPawn = Cast<APawn>(Context.GetEffectCauser()))
	{
		SourceController = Cast<ASPlayerController>(CauserPawn->GetController());
	}
	if (!SourceController)
	{
		if (const APlayerState* InstigatorState = Cast<APlayerState>(Context.GetInstigator()))
		{
			SourceController = Cast<ASPlayerController>(InstigatorState->GetPlayerController());
		}
	}

	AActor* TargetAvatar = Data.Target.GetAvatarActor();
	if (SourceController && TargetAvatar)
	{
		SourceController->ShowDamageNumber(Damage, TargetAvatar);
	}
}
