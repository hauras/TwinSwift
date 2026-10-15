#include "UI/STwinHUDWidget.h"

#include "AbilitySystemComponent.h"
#include "Ability/SAbilitySystemComponent.h"
#include "Ability/SAttributeSet.h"
#include "Character/PlayerCharacter.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "State/SPlayerState.h"
#include "Ability/SGameplayAbility.h"
#include "Controller/SPlayerController.h"
#include "Input/SInputConfig.h"
#include "InputMappingContext.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"

void USTwinHUDWidget::NativeDestruct()
{
	Unbind();

	Super::NativeDestruct();
}

void USTwinHUDWidget::BindToCharacter(APlayerCharacter* InCharacter)
{
	Unbind();

	ASPlayerState* SPlayerState = InCharacter ? InCharacter->GetPlayerState<ASPlayerState>() : nullptr;
	if (!SPlayerState)
	{
		return;
	}

	// 캐릭터마다 자기 ASC의 체력·최대 체력이 바뀔 때만 알림을 받음 (매 프레임 읽지 않음)
	for (const ETwinSlot CharSlot : { ETwinSlot::Warrior, ETwinSlot::Mage })
	{
		FSlotBinding& Binding = CharSlot == ETwinSlot::Warrior ? WarriorBinding : MageBinding;
		UAbilitySystemComponent* ASC = SPlayerState->GetSAbilitySystemComponent(CharSlot);
		if (!ASC)
		{
			continue;
		}

		Binding.ASC = ASC;
		Binding.HealthHandle = ASC->GetGameplayAttributeValueChangeDelegate(USAttributeSet::GetHealthAttribute())
			.AddUObject(this, &ThisClass::OnHealthChanged, CharSlot);
		Binding.MaxHealthHandle = ASC->GetGameplayAttributeValueChangeDelegate(USAttributeSet::GetMaxHealthAttribute())
			.AddUObject(this, &ThisClass::OnHealthChanged, CharSlot);

		RefreshHealth(CharSlot);
	}

	BoundCharacter = InCharacter;
	ActiveSlotChangedHandle = InCharacter->OnActiveSlotChanged.AddUObject(this, &ThisClass::OnActiveSlotChanged);
	SlotDownHandle = InCharacter->OnSlotDown.AddUObject(this, &ThisClass::OnSlotDown);
	DefeatedHandle = InCharacter->OnDefeated.AddUObject(this, &ThisClass::OnDefeated);
	PerfectEvadeHandle = InCharacter->OnPerfectEvade.AddUObject(this, &ThisClass::OnPerfectEvade);
	OnPerfectEvade(false);
	// UI 전용 아이콘 머티리얼은 한 번 로드하고 GC 참조를 보관한다.
	for (const TCHAR* Name : { TEXT("WarriorAttack"), TEXT("DashSlash"), TEXT("GroundSlam"), TEXT("Roll"),
		TEXT("MagicBolt"), TEXT("FrostBolt"), TEXT("Thunder"), TEXT("Blink") })
	{
		const FString Path = FString::Printf(TEXT("/Game/UI/Icons/M_UI_%s.M_UI_%s"), Name, Name);
		SkillMaterials.Add(FName(Name), LoadObject<UMaterialInterface>(nullptr, *Path));
	}
	OnActiveSlotChanged(InCharacter->GetActiveSlot());
	if (GetWorld())
		GetWorld()->GetTimerManager().SetTimer(SkillRefreshTimer, this, &ThisClass::RefreshSkillHUD, 0.1f, true);
}

void USTwinHUDWidget::Unbind()
{
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(SkillRefreshTimer);
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(SwapCooldownTimer);
	for (FSlotBinding* Binding : { &WarriorBinding, &MageBinding })
	{
		if (UAbilitySystemComponent* ASC = Binding->ASC.Get())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(USAttributeSet::GetHealthAttribute()).Remove(Binding->HealthHandle);
			ASC->GetGameplayAttributeValueChangeDelegate(USAttributeSet::GetMaxHealthAttribute()).Remove(Binding->MaxHealthHandle);
		}
		Binding->ASC.Reset();
		Binding->HealthHandle.Reset();
		Binding->MaxHealthHandle.Reset();
	}

	if (APlayerCharacter* Character = BoundCharacter.Get())
	{
		Character->OnActiveSlotChanged.Remove(ActiveSlotChangedHandle);
		Character->OnSlotDown.Remove(SlotDownHandle);
		Character->OnDefeated.Remove(DefeatedHandle);
		Character->OnPerfectEvade.Remove(PerfectEvadeHandle);
	}
	BoundCharacter.Reset();
	ActiveSlotChangedHandle.Reset();
	SlotDownHandle.Reset();
	DefeatedHandle.Reset();
	PerfectEvadeHandle.Reset();
}

void USTwinHUDWidget::OnHealthChanged(const FOnAttributeChangeData& Data, ETwinSlot CharSlot)
{
	RefreshHealth(CharSlot);
}

void USTwinHUDWidget::RefreshHealth(ETwinSlot CharSlot)
{
	const FSlotBinding& Binding = CharSlot == ETwinSlot::Warrior ? WarriorBinding : MageBinding;
	const UAbilitySystemComponent* ASC = Binding.ASC.Get();
	if (!ASC)
	{
		return;
	}

	const float Health = ASC->GetNumericAttribute(USAttributeSet::GetHealthAttribute());
	const float MaxHealth = ASC->GetNumericAttribute(USAttributeSet::GetMaxHealthAttribute());

	if (UProgressBar* Bar = GetHealthBar(CharSlot))
	{
		Bar->SetPercent(MaxHealth > 0.f ? Health / MaxHealth : 0.f);
	}

	if (UTextBlock* Text = GetHealthText(CharSlot))
	{
		Text->SetText(FText::FromString(FString::Printf(
			TEXT("%d / %d"),
			FMath::RoundToInt(Health),
			FMath::RoundToInt(MaxHealth))));
	}
}

void USTwinHUDWidget::OnActiveSlotChanged(ETwinSlot NewSlot)
{
	// 역할별 위치는 고정하고 현재 캐릭터를 큰 카드, 대기 캐릭터를 작은 카드로 표시한다.
	for (const ETwinSlot CharSlot : { ETwinSlot::Warrior, ETwinSlot::Mage })
	{
		if (UWidget* Card = GetCardWidget(CharSlot))
		{
			const bool bActive = CharSlot == NewSlot;
			if (auto* CanvasSlot = Cast<UCanvasPanelSlot>(Card->Slot))
				CanvasSlot->SetPosition(bActive ? FVector2D(20.f, 20.f) : FVector2D(738.f, 20.f));
			Card->SetRenderScale(FVector2D(bActive ? 1.f : BenchScale));
			Card->SetRenderOpacity(bActive ? 1.f : BenchOpacity);
		}
	}

	if (auto* Text = Cast<UTextBlock>(GetWidgetFromName(TEXT("ActiveName"))))
	{
		Text->SetText(FText::FromString(NewSlot == ETwinSlot::Warrior ? TEXT("전사") : TEXT("마법사")));
		Text->SetColorAndOpacity(FSlateColor(NewSlot == ETwinSlot::Warrior ?
			FLinearColor(1.f, 0.52f, 0.12f) : FLinearColor(0.2f, 0.68f, 1.f)));
	}
	RefreshSkillHUD();
	RefreshDownStates();
	StartSwapCooldownDisplay();
	OnActiveSlotChangedBP(NewSlot);
}

// ---------- 교체 규칙 표시 ----------

void USTwinHUDWidget::StartSwapCooldownDisplay()
{
	UpdateSwapCooldownDisplay();

	// 쿨다운이 도는 동안만 짧게 갱신 (평소에는 타이머 없음)
	const APlayerCharacter* Character = BoundCharacter.Get();
	if (Character && Character->GetSwapCooldownRemaining() > 0.f && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(SwapCooldownTimer, this, &ThisClass::UpdateSwapCooldownDisplay, 0.05f, true);
	}
}

void USTwinHUDWidget::UpdateSwapCooldownDisplay()
{
	const APlayerCharacter* Character = BoundCharacter.Get();
	const float Remaining = Character ? Character->GetSwapCooldownRemaining() : 0.f;
	const float Duration = Character ? FMath::Max(Character->GetSwapCooldownDuration(), 0.01f) : 1.f;

	// 쿨다운은 "들어올 쪽" = 대기 카드에만 의미가 있음
	for (const ETwinSlot CharSlot : { ETwinSlot::Warrior, ETwinSlot::Mage })
	{
		const bool bShow = Character
			&& Remaining > 0.f
			&& CharSlot != Character->GetActiveSlot()
			&& !Character->IsSlotDown(CharSlot);
		const ESlateVisibility CoverVisibility = bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;

		if (UProgressBar* Cover = CharSlot == ETwinSlot::Warrior ? WarriorSwapCooldown.Get() : MageSwapCooldown.Get())
		{
			Cover->SetVisibility(CoverVisibility);
			Cover->SetPercent(Remaining / Duration);
		}

		if (UTextBlock* Text = CharSlot == ETwinSlot::Warrior ? WarriorSwapCooldownText.Get() : MageSwapCooldownText.Get())
		{
			Text->SetVisibility(CoverVisibility);
			Text->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), Remaining)));
		}
	}

	if (Remaining <= 0.f && GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(SwapCooldownTimer);
	}
}

void USTwinHUDWidget::RefreshDownStates()
{
	const APlayerCharacter* Character = BoundCharacter.Get();
	if (!Character)
	{
		return;
	}

	for (const ETwinSlot CharSlot : { ETwinSlot::Warrior, ETwinSlot::Mage })
	{
		const bool bDown = Character->IsSlotDown(CharSlot);

		if (UTextBlock* Text = CharSlot == ETwinSlot::Warrior ? WarriorDownText.Get() : MageDownText.Get())
		{
			Text->SetVisibility(bDown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}

		// 쓰러진 카드는 교체 강조(OnActiveSlotChanged)보다 더 흐리게
		if (bDown)
		{
			if (UWidget* Card = GetCardWidget(CharSlot))
			{
				Card->SetRenderOpacity(DownOpacity);
			}
		}
	}

	if (DefeatText)
	{
		DefeatText->SetVisibility(Character->IsDefeated() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void USTwinHUDWidget::OnSlotDown(ETwinSlot DownSlot)
{
	RefreshDownStates();
	UpdateSwapCooldownDisplay();
}

void USTwinHUDWidget::OnDefeated()
{
	RefreshDownStates();
}

void USTwinHUDWidget::OnPerfectEvade(bool bStarted)
{
	if (PerfectEvadeText)
	{
		PerfectEvadeText->SetVisibility(bStarted ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

UWidget* USTwinHUDWidget::GetCardWidget(ETwinSlot CharSlot) const
{
	return CharSlot == ETwinSlot::Warrior ? WarriorCard.Get() : MageCard.Get();
}

UProgressBar* USTwinHUDWidget::GetHealthBar(ETwinSlot CharSlot) const
{
	return CharSlot == ETwinSlot::Warrior ? WarriorHealthBar.Get() : MageHealthBar.Get();
}

UTextBlock* USTwinHUDWidget::GetHealthText(ETwinSlot CharSlot) const
{
	return CharSlot == ETwinSlot::Warrior ? WarriorHealthText.Get() : MageHealthText.Get();
}

// 이 파일 내용은 준비 스크립트가 HUD cpp에 추가한다.

namespace
{
    struct FHUDSkill
    {
        const TCHAR* Name;
        const TCHAR* Input;
        const TCHAR* Key;
        const TCHAR* Description;
        const TCHAR* Icon;
    };

    const FHUDSkill WarriorSkills[] = {
        { TEXT("대검 콤보"), TEXT("InputTag.Attack"), TEXT("LMB"), TEXT("2타 콤보 · 배율 1.0 / 1.6"), TEXT("WarriorAttack") },
        { TEXT("돌진 베기"), TEXT("InputTag.Warrior.Skill1"), TEXT("1"), TEXT("커서 방향 6m · 경로 타격 1.8 · 쿨 6초"), TEXT("DashSlash") },
        { TEXT("내려찍기"), TEXT("InputTag.Warrior.Skill2"), TEXT("2"), TEXT("커서 위치 착지 · 반경 3m · 배율 2.2 · 균열 · 쿨 10초"), TEXT("GroundSlam") },
        { TEXT("구르기"), TEXT("InputTag.Dodge"), TEXT("Space"), TEXT("구르며 0.3초 동안 무적"), TEXT("Roll") }
    };
    const FHUDSkill MageSkills[] = {
        { TEXT("마탄"), TEXT("InputTag.Attack"), TEXT("LMB"), TEXT("커서를 향해 마탄 발사"), TEXT("MagicBolt") },
        { TEXT("냉기탄"), TEXT("InputTag.Mage.Skill1"), TEXT("3"), TEXT("냉기 투사체 · 빙결 표식 부여"), TEXT("FrostBolt") },
        { TEXT("낙뢰"), TEXT("InputTag.Mage.Skill2"), TEXT("4"), TEXT("커서 위치 범위 공격 · 균열 표식 폭발"), TEXT("Thunder") },
        { TEXT("순간이동"), TEXT("InputTag.Dodge"), TEXT("Space"), TEXT("짧은 거리 순간이동"), TEXT("Blink") }
    };

    FGameplayTag HUDTag(const TCHAR* Name)
    {
        return FGameplayTag::RequestGameplayTag(FName(Name), false);
    }
}

void USTwinHUDWidget::SetHUDText(FName Name, const FString& Value)
{
    if (auto* Text = Cast<UTextBlock>(GetWidgetFromName(Name)))
        Text->SetText(FText::FromString(Value));
}

FString USTwinHUDWidget::GetInputKeyLabel(FGameplayTag Input, const FString& Fallback) const
{
    const auto* PC = Cast<ASPlayerController>(GetOwningPlayer());
    if (!PC || !PC->GetHUDInputConfig() || !PC->GetHUDMappingContext()) return Fallback;
    for (const auto& Entry : PC->GetHUDInputConfig()->AbilityInputActions)
    {
        if (Entry.InputTag != Input) continue;
        for (const auto& Mapping : PC->GetHUDMappingContext()->GetMappings())
        {
            if (Mapping.Action != Entry.InputAction) continue;
            const FName Key = Mapping.Key.GetFName();
            if (Key == TEXT("LeftMouseButton")) return TEXT("LMB");
            if (Key == TEXT("SpaceBar")) return TEXT("Space");
            if (Key == TEXT("One")) return TEXT("1");
            if (Key == TEXT("Two")) return TEXT("2");
            if (Key == TEXT("Three")) return TEXT("3");
            if (Key == TEXT("Four")) return TEXT("4");
            return Mapping.Key.GetDisplayName().ToString();
        }
    }
    return Fallback;
}

void USTwinHUDWidget::RefreshSkillHUD()
{
    const auto* Character = BoundCharacter.Get();
    if (!Character) return;
    const bool bWarrior = Character->GetActiveSlot() == ETwinSlot::Warrior;
    const FHUDSkill* Active = bWarrior ? WarriorSkills : MageSkills;
    const FHUDSkill* Bench = bWarrior ? MageSkills : WarriorSkills;
    UAbilitySystemComponent* ASC = bWarrior ? WarriorBinding.ASC.Get() : MageBinding.ASC.Get();
    if (!ASC) return;
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const FHUDSkill& Skill = Active[Index];
        const FString Prefix = FString::Printf(TEXT("Skill%d"), Index);
        const FGameplayTag Input = HUDTag(Skill.Input);
        SetHUDText(FName(Prefix + TEXT("Name")), Skill.Name);
        SetHUDText(FName(Prefix + TEXT("Key")), GetInputKeyLabel(Input, Skill.Key));
        const USGameplayAbility* Ability = nullptr;
        for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
        {
            const auto* Candidate = Cast<USGameplayAbility>(Spec.Ability);
            if (Candidate && Candidate->InputTag == Input) { Ability = Candidate; break; }
        }
        float Remaining = 0.f;
        float Duration = 0.f;
        if (Ability)
        if (const auto* Tags = Ability->GetCooldownTags(); Tags && !Tags->IsEmpty())
        {
            const auto Times = ASC->GetActiveEffectsTimeRemainingAndDuration(
                FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(*Tags));
            for (const auto& Time : Times)
                if (Time.Key > Remaining) { Remaining = Time.Key; Duration = Time.Value; }
        }
        SetHUDText(FName(Prefix + TEXT("State")), !Ability ? TEXT("미구현") :
            Remaining > 0.f ? FString::Printf(TEXT("%.1f"), Remaining) : TEXT(""));
        if (auto* Overlay = Cast<UProgressBar>(GetWidgetFromName(FName(Prefix + TEXT("Cooldown")))))
            Overlay->SetPercent(Duration > 0.f ? FMath::Clamp(Remaining / Duration, 0.f, 1.f) : 0.f);
        if (auto* Image = Cast<UImage>(GetWidgetFromName(FName(Prefix + TEXT("Icon")))))
        {
            if (const auto* Icon = SkillMaterials.Find(FName(Skill.Icon)); Icon && Icon->Get()) Image->SetBrushFromMaterial(Icon->Get());
            Image->SetRenderOpacity(Ability ? 1.f : 0.3f);
            Image->SetToolTipText(FText::FromString(FString(Skill.Description) +
                (Ability ? TEXT("") : TEXT("\n계획된 스킬입니다. 능력이 지급되면 활성화됩니다."))));
        }
    }
    SetHUDText(TEXT("BenchSkill1Key"), GetInputKeyLabel(HUDTag(Bench[1].Input), Bench[1].Key));
    SetHUDText(TEXT("BenchSkill2Key"), GetInputKeyLabel(HUDTag(Bench[2].Input), Bench[2].Key));
    SetHUDText(TEXT("PassiveText"), bWarrior ? TEXT("산산조각 · 빙결 적 타격 시 3.0 + 경직 [예정]") :
        TEXT("균열 → 낙뢰로 폭발 / 빙결 → 전사가 산산조각 [예정]"));
    SetHUDText(TEXT("AssistText"), bWarrior ? TEXT("교체 어시스트: 냉기 장판 [예정]") :
        TEXT("교체 어시스트: 회전 베기 1.2 [예정]"));
}

