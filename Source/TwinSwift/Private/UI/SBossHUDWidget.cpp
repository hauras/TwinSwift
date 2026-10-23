#include "UI/SBossHUDWidget.h"

#include "AbilitySystemComponent.h"
#include "Ability/SAttributeSet.h"
#include "Character/BossCharacter.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "TimerManager.h"

void USBossHUDWidget::BindToEncounter(ASBossEncounter* InEncounter)
{
	Unbind();

	if (!InEncounter)
	{
		return;
	}
	Encounter = InEncounter;
	StateChangedHandle = InEncounter->OnEncounterStateChanged.AddUObject(this, &ThisClass::OnEncounterStateChanged);

	// 보스 이름·체력 (체력·최대 체력이 바뀔 때만 알림 → 매 프레임 읽지 않음)
	if (ABossCharacter* EncounterBoss = InEncounter->GetBoss())
	{
		BossNameText->SetText(EncounterBoss->GetDisplayName());

		Boss = EncounterBoss;
		ArmorChangedHandle = EncounterBoss->OnArmorChanged.AddUObject(this, &ThisClass::OnBossArmorChanged);

		if (UAbilitySystemComponent* ASC = EncounterBoss->GetAbilitySystemComponent())
		{
			BossASC = ASC;
			HealthChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(USAttributeSet::GetHealthAttribute())
				.AddUObject(this, &ThisClass::OnBossHealthChanged);
			MaxHealthChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(USAttributeSet::GetMaxHealthAttribute())
				.AddUObject(this, &ThisClass::OnBossHealthChanged);
		}
	}

	RefreshHealth();
	HideArmorText();

	// 지금 상태에 맞게 보이기 (대기 중이면 전부 숨김)
	OnEncounterStateChanged(InEncounter->GetState());
}

void USBossHUDWidget::NativeDestruct()
{
	Unbind();

	Super::NativeDestruct();
}

void USBossHUDWidget::Unbind()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(IntroTitleTimer);
		GetWorld()->GetTimerManager().ClearTimer(ArmorTextTimer);
	}

	if (ABossCharacter* OldBoss = Boss.Get())
	{
		OldBoss->OnArmorChanged.Remove(ArmorChangedHandle);
	}

	if (ASBossEncounter* OldEncounter = Encounter.Get())
	{
		OldEncounter->OnEncounterStateChanged.Remove(StateChangedHandle);
	}
	if (UAbilitySystemComponent* ASC = BossASC.Get())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(USAttributeSet::GetHealthAttribute()).Remove(HealthChangedHandle);
		ASC->GetGameplayAttributeValueChangeDelegate(USAttributeSet::GetMaxHealthAttribute()).Remove(MaxHealthChangedHandle);
	}

	Encounter.Reset();
	Boss.Reset();
	BossASC.Reset();
	ArmorChangedHandle.Reset();
	StateChangedHandle.Reset();
	HealthChangedHandle.Reset();
	MaxHealthChangedHandle.Reset();
}

void USBossHUDWidget::OnEncounterStateChanged(EBossEncounterState NewState)
{
	// 이름·체력바: 전투부터 (등장신 중에는 이름 카드가 대신)
	const bool bShowBoss = NewState != EBossEncounterState::Waiting && NewState != EBossEncounterState::Intro;
	BossPanel->SetVisibility(bShowBoss ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	// 이름 카드: 등장이 시작되면 착지 직후에 나왔다 사라짐. 건너뛰어 전투가 되면 바로 숨김
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(IntroTitleTimer);
	}
	if (NewState == EBossEncounterState::Intro && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(IntroTitleTimer, this, &ThisClass::ShowIntroTitle, FMath::Max(IntroTitleDelay, 0.01f), false);
	}
	else
	{
		HideIntroTitle();
	}

	// 승리·패배면 갑옷 안내도 치움
	if (NewState == EBossEncounterState::Victory || NewState == EBossEncounterState::Defeat)
	{
		HideArmorText();
	}

	// 결과 글자: 승리·패배일 때만
	switch (NewState)
	{
	case EBossEncounterState::Victory:
	{
		const float Seconds = Encounter.IsValid() ? Encounter->GetFightSeconds() : 0.f;
		ResultText->SetText(FText::FromString(FString::Printf(TEXT("클리어 %.1f초 · R: 다시 시작"), Seconds)));
		ResultText->SetVisibility(ESlateVisibility::HitTestInvisible);
		break;
	}
	case EBossEncounterState::Defeat:
		ResultText->SetText(FText::FromString(TEXT("패배 · R: 다시 시작")));
		ResultText->SetVisibility(ESlateVisibility::HitTestInvisible);
		break;
	default:
		ResultText->SetVisibility(ESlateVisibility::Collapsed);
		break;
	}
}

void USBossHUDWidget::ShowIntroTitle()
{
	if (!IntroTitleText)
	{
		return;
	}

	if (const ASBossEncounter* CurrentEncounter = Encounter.Get())
	{
		if (const ABossCharacter* EncounterBoss = CurrentEncounter->GetBoss())
		{
			IntroTitleText->SetText(EncounterBoss->GetDisplayName());
		}
	}
	IntroTitleText->SetVisibility(ESlateVisibility::HitTestInvisible);

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(IntroTitleTimer, this, &ThisClass::HideIntroTitle, IntroTitleDuration, false);
	}
}

void USBossHUDWidget::HideIntroTitle()
{
	if (IntroTitleText)
	{
		IntroTitleText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void USBossHUDWidget::OnBossArmorChanged(EBossArmorEvent Event)
{
	const ABossCharacter* CurrentBoss = Boss.Get();
	if (!ArmorText || !CurrentBoss)
	{
		return;
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ArmorTextTimer);
	}

	switch (Event)
	{
	case EBossArmorEvent::Raised:
	case EBossArmorEvent::Hit:
	{
		// 깬 횟수 / 필요한 횟수
		const int32 Required = CurrentBoss->GetArmorHitsRequired();
		const int32 Done = Required - CurrentBoss->GetArmorHitsLeft();
		ArmorText->SetText(FText::FromString(FString::Printf(TEXT("대지의 분노 · 표식 폭발로 갑옷 깨기 %d / %d"), Done, Required)));
		ArmorText->SetVisibility(ESlateVisibility::HitTestInvisible);
		break;
	}
	case EBossArmorEvent::Broken:
		ArmorText->SetText(FText::FromString(TEXT("갑옷 파괴!")));
		ArmorText->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimer(ArmorTextTimer, this, &ThisClass::HideArmorText, ArmorBrokenTextDuration, false);
		}
		break;
	case EBossArmorEvent::Expired:
		HideArmorText();
		break;
	}
}

void USBossHUDWidget::HideArmorText()
{
	if (ArmorText)
	{
		ArmorText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void USBossHUDWidget::OnBossHealthChanged(const FOnAttributeChangeData& Data)
{
	RefreshHealth();
}

void USBossHUDWidget::RefreshHealth()
{
	const UAbilitySystemComponent* ASC = BossASC.Get();
	if (!ASC)
	{
		return;
	}

	const float Health = ASC->GetNumericAttribute(USAttributeSet::GetHealthAttribute());
	const float MaxHealth = ASC->GetNumericAttribute(USAttributeSet::GetMaxHealthAttribute());
	BossHealthBar->SetPercent(MaxHealth > 0.f ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f);
}
