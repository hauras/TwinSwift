#include "UI/SUpgradeSelectWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Upgrade/SUpgradeData.h"

namespace
{
	// 카드에 보일 대상 글자
	FText GetTargetText(EUpgradeTarget Target)
	{
		switch (Target)
		{
		case EUpgradeTarget::Warrior:
			return FText::FromString(TEXT("전사"));
		case EUpgradeTarget::Mage:
			return FText::FromString(TEXT("마법사"));
		default:
			return FText::FromString(TEXT("공통"));
		}
	}

	// 대상 색 (글자·색띠): 탄 색 규칙과 같게 (전사 주황·마법사 파랑), 공통은 회색
	FLinearColor GetTargetColor(EUpgradeTarget Target)
	{
		switch (Target)
		{
		case EUpgradeTarget::Warrior:
			return FLinearColor(1.f, 0.55f, 0.15f);
		case EUpgradeTarget::Mage:
			return FLinearColor(0.3f, 0.62f, 1.f);
		default:
			return FLinearColor(0.7f, 0.68f, 0.64f);
		}
	}
}

void USUpgradeSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 키보드 1/2/3: 포커스를 받을 수 있어야 NativeOnKeyDown이 불림 (실제 포커스는 ShowChoices에서)
	SetIsFocusable(true);

	// 버튼 클릭은 인자가 없어 버튼마다 다른 함수로 연결
	UButton* Button0 = Cast<UButton>(GetWidgetFromName(TEXT("Card0Button")));
	UButton* Button1 = Cast<UButton>(GetWidgetFromName(TEXT("Card1Button")));
	UButton* Button2 = Cast<UButton>(GetWidgetFromName(TEXT("Card2Button")));

	if (Button0)
	{
		Button0->OnClicked.AddDynamic(this, &ThisClass::OnCard0Clicked);
	}
	if (Button1)
	{
		Button1->OnClicked.AddDynamic(this, &ThisClass::OnCard1Clicked);
	}
	if (Button2)
	{
		Button2->OnClicked.AddDynamic(this, &ThisClass::OnCard2Clicked);
	}
}

void USUpgradeSelectWidget::ShowChoices(const TArray<USUpgradeData*>& InChoices)
{
	Choices = InChoices;
	bChosen = false;

	for (int32 Index = 0; Index < 3; ++Index)
	{
		// 이 칸의 위젯들 (Card0Button, Card0Title ...)
		const FString Prefix = FString::Printf(TEXT("Card%d"), Index);
		UButton* Button = Cast<UButton>(GetWidgetFromName(FName(Prefix + TEXT("Button"))));
		UTextBlock* Title = Cast<UTextBlock>(GetWidgetFromName(FName(Prefix + TEXT("Title"))));
		UTextBlock* Description = Cast<UTextBlock>(GetWidgetFromName(FName(Prefix + TEXT("Description"))));
		UTextBlock* Target = Cast<UTextBlock>(GetWidgetFromName(FName(Prefix + TEXT("Target"))));
		UBorder* Band = Cast<UBorder>(GetWidgetFromName(FName(Prefix + TEXT("Band"))));

		// 넘겨받은 강화가 3개보다 적으면 남는 칸은 숨김 (자리까지 없애 가운데로 모임)
		const USUpgradeData* Upgrade = InChoices.IsValidIndex(Index) ? InChoices[Index] : nullptr;
		if (!Upgrade)
		{
			if (Button)
			{
				Button->SetVisibility(ESlateVisibility::Collapsed);
			}
			continue;
		}

		if (Title)
		{
			Title->SetText(Upgrade->DisplayName);
		}
		if (Description)
		{
			Description->SetText(Upgrade->Description);
		}
		if (Target)
		{
			Target->SetText(GetTargetText(Upgrade->Target));
			Target->SetColorAndOpacity(FSlateColor(GetTargetColor(Upgrade->Target)));
		}
		if (Band)
		{
			Band->SetBrushColor(GetTargetColor(Upgrade->Target));
		}

		// 지난번에 숨긴 칸일 수 있음. 버튼은 클릭을 받아야 해서 HitTestInvisible이 아니라 Visible
		if (Button)
		{
			Button->SetVisibility(ESlateVisibility::Visible);
		}
	}

	// 키보드 1/2/3을 받게
	SetKeyboardFocus();
}

FReply USUpgradeSelectWidget::NativeOnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	// 1/2/3은 여기서 처리하고 멈춤 (뒤의 게임 입력으로 넘어가지 않게). 나머지 키는 부모에게
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::One)
	{
		Choose(0);
		return FReply::Handled();
	}
	if (Key == EKeys::Two)
	{
		Choose(1);
		return FReply::Handled();
	}
	if (Key == EKeys::Three)
	{
		Choose(2);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(MyGeometry, InKeyEvent);
}

void USUpgradeSelectWidget::OnCard0Clicked()
{
	Choose(0);
}

void USUpgradeSelectWidget::OnCard1Clicked()
{
	Choose(1);
}

void USUpgradeSelectWidget::OnCard2Clicked()
{
	Choose(2);
}

void USUpgradeSelectWidget::Choose(int32 Index)
{
	// 클릭과 키가 같은 프레임에 들어와도 한 번만. 숨긴 칸 번호(키 3인데 카드 2장)는 무시
	if (bChosen || !Choices.IsValidIndex(Index)) return;

	bChosen = true;
	OnUpgradeChosen.Broadcast(Choices[Index]);
}
