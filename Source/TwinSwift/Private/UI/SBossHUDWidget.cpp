#include "UI/SBossHUDWidget.h"

#include "AbilitySystemComponent.h"
#include "Ability/SAttributeSet.h"
#include "Character/BossCharacter.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

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
	if (ABossCharacter* Boss = InEncounter->GetBoss())
	{
		BossNameText->SetText(Boss->GetDisplayName());

		if (UAbilitySystemComponent* ASC = Boss->GetAbilitySystemComponent())
		{
			BossASC = ASC;
			HealthChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(USAttributeSet::GetHealthAttribute())
				.AddUObject(this, &ThisClass::OnBossHealthChanged);
			MaxHealthChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(USAttributeSet::GetMaxHealthAttribute())
				.AddUObject(this, &ThisClass::OnBossHealthChanged);
		}
	}

	RefreshHealth();

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
	BossASC.Reset();
	StateChangedHandle.Reset();
	HealthChangedHandle.Reset();
	MaxHealthChangedHandle.Reset();
}

void USBossHUDWidget::OnEncounterStateChanged(EBossEncounterState NewState)
{
	// 이름·체력바: 등장 ~ 결과까지 (등장신 중에 체력바가 올라오는 연출은 10-22에)
	const bool bShowBoss = NewState != EBossEncounterState::Waiting;
	BossPanel->SetVisibility(bShowBoss ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

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
