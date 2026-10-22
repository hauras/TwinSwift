
#include "AnimNotify/ANS_ActionWindow.h"

#include "AbilitySystemGlobals.h"
#include "Character/PlayerCharacter.h"
#include "State/SPlayerState.h"
#include "Ability/SAbilitySystemComponent.h"

void UANS_ActionWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
                                    const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	UAbilitySystemComponent* ASC = FindOwnerASC(MeshComp);
	if (ASC && !WindowTags.IsEmpty())
	{
		ASC->AddLooseGameplayTags(WindowTags);
	}
}

void UANS_ActionWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                  const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	UAbilitySystemComponent* ASC = FindOwnerASC(MeshComp);
	if (ASC && !WindowTags.IsEmpty())
	{
		ASC->RemoveLooseGameplayTags(WindowTags);
	}
}

FString UANS_ActionWindow::GetNotifyName_Implementation() const
{
	// 몽타주 타임라인에 보일 이름: "Window: Combo Dodge"
	if (WindowTags.IsEmpty())
	{
		return TEXT("Action Window");
	}

	FString Result = TEXT("Window:");
	for (const FGameplayTag& Tag : WindowTags)
	{
		// "Window.Combo" → 끝에서부터 첫 점에서 잘라 오른쪽 "Combo"만
		FString ShortName;
		Tag.GetTagName().ToString().Split(TEXT("."), nullptr, &ShortName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		Result += TEXT(" ") + ShortName;
	}
	return Result;
}

UAbilitySystemComponent* UANS_ActionWindow::FindOwnerASC(USkeletalMeshComponent* MeshComp)
{
	if (!MeshComp) return nullptr;

	AActor* Owner = MeshComp->GetOwner();

	if (APlayerCharacter* PlayerCharacter  = Cast<APlayerCharacter>(Owner))
	{
		ASPlayerState* PlayerState = PlayerCharacter->GetPlayerState<ASPlayerState>();
		if (!PlayerState) return nullptr;
		
		for (const ETwinSlot CharSlot : { ETwinSlot::Warrior, ETwinSlot::Mage })
		{
			if (PlayerCharacter->GetSlotMesh(CharSlot) == MeshComp)
			{
				return PlayerState->GetSAbilitySystemComponent(CharSlot);
			}
		}
		return nullptr;
	}
	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner);
}
