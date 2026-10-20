#include "Ability/SAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Controller/SPlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "SGameplayTags.h"
#include "Character/PlayerCharacter.h"

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

	// 값은 이 GE를 실행한 ASC(Data.Target)에 씀
	// SetHealth 같은 자동 setter는 주인 액터(PlayerState)의 "지금 ASC"에 써서, 캐릭터가 둘이면 다른 캐릭터 값을 덮어씀
	UAbilitySystemComponent& TargetASC = Data.Target;

	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float Damage = GetIncomingDamage();
		TargetASC.SetNumericAttributeBase(GetIncomingDamageAttribute(), 0.f);

		// 무적(회피 중 등)이면 데미지를 버림. 그중 완벽 회피 판정 중이었으면 슬로 연출
		if (TargetASC.HasMatchingGameplayTag(SGameplayTags::State_Invincible))
		{
			if (TargetASC.HasMatchingGameplayTag(SGameplayTags::State_PerfectWindow))
			{
				if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(TargetASC.GetAvatarActor()))
				{
					PlayerCharacter->TriggerPerfectEvade();
				}
			}
			return;
		}

		if (Damage > 0.f)
		{
			TargetASC.SetNumericAttributeBase(GetHealthAttribute(), FMath::Clamp(GetHealth() - Damage, 0.f, GetMaxHealth()));

			ShowDamageNumber(Data, Damage);

			// 플레이어가 맞았으면 맞은 쪽에서 알림 (때린 쪽이 보스면 위 숫자가 안 뜸)
			if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(TargetASC.GetAvatarActor()))
			{
				PlayerCharacter->NotifyDamaged(Damage);
			}

			// 사망은 이후 단계에서
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// 회복 GE 등으로 직접 바뀐 경우에도 범위 유지
		TargetASC.SetNumericAttributeBase(GetHealthAttribute(), FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
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
		// 데미지 스펙에 붙은 Damage.* 태그로 숫자 종류를 정함
		const FGameplayTagContainer& DamageTags = Data.EffectSpec.GetDynamicAssetTags();
		EDamageNumberType Type = EDamageNumberType::Normal;
		if (DamageTags.HasTagExact(SGameplayTags::Damage_Shatter))
		{
			Type = EDamageNumberType::Shatter;
		}
		else if (DamageTags.HasTagExact(SGameplayTags::Damage_CrackBurst))
		{
			Type = EDamageNumberType::CrackBurst;
		}
		SourceController->ShowDamageNumber(Damage, TargetAvatar, Type);
	}
}
