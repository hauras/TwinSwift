

#include "Input/SInputBufferComponent.h"
#include "SGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Character/PlayerCharacter.h"
#include "Controller/SPlayerController.h"
#include "GameFramework/PlayerController.h"

void USInputBufferComponent::HandleInput(const FGameplayTag& InputTag)
{
	if (IsAllowedNow(InputTag))
	{
		// 칸 하나 = 마지막 입력만: 바로 실행되는 입력이 와도 이전 예약은 버림 (회피 예약 → 평타 → 창이 열릴 때 회피가 나가지 않게)
		Clear();
		Execute(InputTag);
		return;
	}

	// 칸이 하나라 새 입력이 이전 저장을 덮어씀 (가장 마지막에 누른 것이 남음)
	// 시간은 실제 시간: 완벽 회피 슬로(세상 0.3배) 중에도 플레이어는 정상 속도라 0.3초가 1초로 늘지 않게
	BufferedInput = InputTag;
	BufferedTime = GetWorld()->GetRealTimeSeconds();
}

void USInputBufferComponent::Clear()
{
	BufferedInput = FGameplayTag();
	BufferedTime = 0.0;
}

void USInputBufferComponent::BindToASC(UAbilitySystemComponent* ASC)
{
	// 이미 이 ASC를 듣고 있으면 그대로 (같은 알림을 두 번 받지 않게)
	if (BoundASC.Get() == ASC)
	{
		return;
	}

	// 이전 캐릭터 ASC의 알림은 끊음 (교체하면 다른 ASC)
	UnbindASC();
	if (!ASC)
	{
		return;
	}
	BoundASC = ASC;

	// 들을 태그: 창 3개(열리면 실행 시도), 공격 중(끝나면 실행 시도), 경직·사망(붙으면 버림)
	const FGameplayTag WatchedTags[] = {
		SGameplayTags::Window_Dodge,
		SGameplayTags::Window_Swap,
		SGameplayTags::Window_Skill,
		SGameplayTags::State_Attacking,
		SGameplayTags::State_Stun,
		SGameplayTags::State_Dead,
	};
	for (const FGameplayTag& Tag : WatchedTags)
	{
		// 0 ↔ 1 이상으로 바뀔 때만 알림 (NewOrRemoved). 핸들은 끊을 때 씀
		const FDelegateHandle Handle = ASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &ThisClass::OnTagChanged);
		TagHandles.Emplace(Tag, Handle);
	}
}

void USInputBufferComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindASC();

	Super::EndPlay(EndPlayReason);
}

FGameplayTag USInputBufferComponent::GetRequiredWindow(const FGameplayTag& InputTag) const
{
	if (InputTag== SGameplayTags::InputTag_Attack)
	{
		return FGameplayTag();
	}
	else if (InputTag== SGameplayTags::InputTag_Dodge)
	{
		return SGameplayTags::Window_Dodge;
	}
	else if (InputTag== SGameplayTags::InputTag_Swap)
	{
		return SGameplayTags::Window_Swap;
	}
	const bool bWarriorSkill = InputTag == SGameplayTags::InputTag_Warrior_Skill1 || InputTag == SGameplayTags::InputTag_Warrior_Skill2;
	const bool bMageSkill = InputTag == SGameplayTags::InputTag_Mage_Skill1 || InputTag == SGameplayTags::InputTag_Mage_Skill2;

	if (!bWarriorSkill && !bMageSkill)
	{
		return FGameplayTag();
	}
	
	// 지금 나와 있는 캐릭터 (이 컴포넌트의 주인 = 컨트롤러 → 그 컨트롤러의 폰)
	const APlayerController* Controller = Cast<APlayerController>(GetOwner());
	const APlayerCharacter* PlayerCharacter = Controller ? Controller->GetPawn<APlayerCharacter>() : nullptr;
	if (!PlayerCharacter)
	{
		return SGameplayTags::Window_Skill;
	}

	// 지금 캐릭터의 스킬이면 스킬 창, 상대 캐릭터의 스킬이면 교체가 먼저라 교체 창
	const bool bWarriorNow = PlayerCharacter->GetActiveSlot() == ETwinSlot::Warrior;
	const bool bOwnSkill = (bWarriorSkill && bWarriorNow) || (bMageSkill && !bWarriorNow);
	return bOwnSkill ? SGameplayTags::Window_Skill : SGameplayTags::Window_Swap;
}

bool USInputBufferComponent::IsAllowedNow(const FGameplayTag& InputTag) const
{
	const FGameplayTag Required = GetRequiredWindow(InputTag);
	if (!Required.IsValid())
	{
		return true;
	}
	const UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC)
	{
		return true;
	}
	if (!ASC->HasMatchingGameplayTag(SGameplayTags::State_Attacking))
	{
		return true;
	}
	return ASC->HasMatchingGameplayTag(Required);
}

void USInputBufferComponent::TryFlush()
{
	if (!BufferedInput.IsValid())
	{
		return;
	}
	if (GetWorld()->GetRealTimeSeconds() - BufferedTime > BufferDuration)
	{
		Clear();
		return;
	}
	if (!IsAllowedNow(BufferedInput))
	{
		return;
	}
	// 먼저 비우고 실행: 실행하면 능력이 발동해 태그가 바뀌고 TryFlush가 다시 불릴 수 있음 → 같은 입력이 두 번 나가지 않게
	const FGameplayTag Input = BufferedInput;
	Clear();
	Execute(Input);
}

void USInputBufferComponent::Execute(const FGameplayTag& InputTag)
{
	// 실행은 컨트롤러가 함 (교체·스킬 키 → 캐릭터·ASC를 아는 쪽)
	if (ASPlayerController* Controller = Cast<ASPlayerController>(GetOwner()))
	{
		Controller->ExecuteAbilityInput(InputTag);
	}
}

void USInputBufferComponent::OnTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// 경직·사망이 붙음 → 저장된 입력을 버림 (풀린 뒤 예전 입력이 갑자기 나가지 않게)
	if (Tag == SGameplayTags::State_Stun || Tag == SGameplayTags::State_Dead)
	{
		if (NewCount > 0)
		{
			Clear();
		}
		return;
	}

	// 공격이 끝남(공격 중 태그가 떨어짐) 또는 창이 열림(창 태그가 붙음) → 지금 실행할 수 있는지 다시 확인
	const bool bAttackEnded = Tag == SGameplayTags::State_Attacking && NewCount == 0;
	const bool bWindowOpened = Tag != SGameplayTags::State_Attacking && NewCount > 0;
	if (bAttackEnded || bWindowOpened)
	{
		TryFlush();
	}
}

void USInputBufferComponent::UnbindASC()
{
	// 구독할 때와 같은 태그·같은 알림 종류로 찾아서 핸들로 끊음
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		for (const TPair<FGameplayTag, FDelegateHandle>& Pair : TagHandles)
		{
			ASC->RegisterGameplayTagEvent(Pair.Key, EGameplayTagEventType::NewOrRemoved).Remove(Pair.Value);
		}
	}
	TagHandles.Reset();
	BoundASC.Reset();
}
