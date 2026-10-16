#include "UI/SMarkIconWidget.h"

#include "GameplayTagContainer.h"
#include "SGameplayTags.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Materials/MaterialInterface.h"
#include "UI/SMarkIndicatorComponent.h"

namespace
{
	// 아이콘 크기(px), 남은 시간 막대 높이(px), 칸 사이 간격(px)
	constexpr float IconSize = 64.f;
	constexpr float BarHeight = 5.f;
	constexpr float SlotGap = 4.f;

	// 남은 시간이 이보다 적으면 깜빡임(초), 깜빡이는 빠르기(초당)
	constexpr float BlinkStartSeconds = 1.5f;
	constexpr float BlinkSpeed = 8.f;

	// 막대 색: [균열] 주황, [빙결] 하늘색
	const FLinearColor CrackColor(1.f, 0.45f, 0.1f, 1.f);
	const FLinearColor FrostColor(0.35f, 0.8f, 1.f, 1.f);
}

void USMarkIconWidget::Setup(USMarkIndicatorComponent* InOwner, UMaterialInterface* CrackIcon, UMaterialInterface* FrostIcon)
{
	OwnerComponent = InOwner;
	CrackMaterial = CrackIcon;
	FrostMaterial = FrostIcon;
	ApplyIcons();
}

void USMarkIconWidget::SetMarks(bool bCrack, bool bFrost)
{
	bShowCrack = bCrack;
	bShowFrost = bFrost;
	if (CrackSlot.Root)
	{
		CrackSlot.Root->SetVisibility(bCrack ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (FrostSlot.Root)
	{
		FrostSlot.Root->SetVisibility(bFrost ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void USMarkIconWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UWidget* Root = GetWidgetFromName(TEXT("CrackRoot")))
	{
		CrackSlot.Root = Root;
		CrackSlot.Icon = Cast<UImage>(GetWidgetFromName(TEXT("CrackIcon")));
		CrackSlot.TimeBar = Cast<UProgressBar>(GetWidgetFromName(TEXT("CrackTimeBar")));
	}
	if (UWidget* Root = GetWidgetFromName(TEXT("FrostRoot")))
	{
		FrostSlot.Root = Root;
		FrostSlot.Icon = Cast<UImage>(GetWidgetFromName(TEXT("FrostIcon")));
		FrostSlot.TimeBar = Cast<UProgressBar>(GetWidgetFromName(TEXT("FrostTimeBar")));
	}
	ApplyIcons();
	SetMarks(bShowCrack, bShowFrost);
}

TSharedRef<SWidget> USMarkIconWidget::RebuildWidget()
{
	// WBP가 있으면 디자이너의 배치를 사용하고, 없으면 기본 트리를 만듦
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), "MarkRow");
		WidgetTree->RootWidget = Row;

		CrackSlot = MakeSlot(CrackColor);
		FrostSlot = MakeSlot(FrostColor);
		for (const FMarkSlot* MarkSlot : { &CrackSlot, &FrostSlot })
		{
			UHorizontalBoxSlot* RowSlot = Row->AddChildToHorizontalBox(MarkSlot->Root);
			RowSlot->SetPadding(FMargin(SlotGap * 0.5f, 0.f));
			RowSlot->SetVerticalAlignment(VAlign_Center);
		}
		ApplyIcons();
		SetMarks(bShowCrack, bShowFrost);
	}
	if (WidgetTree && WidgetTree->RootWidget)
	{
		if (UWidget* Root = GetWidgetFromName(TEXT("CrackRoot")))
		{
			CrackSlot.Root = Root;
			CrackSlot.Icon = Cast<UImage>(GetWidgetFromName(TEXT("CrackIcon")));
			CrackSlot.TimeBar = Cast<UProgressBar>(GetWidgetFromName(TEXT("CrackTimeBar")));
		}
		if (UWidget* Root = GetWidgetFromName(TEXT("FrostRoot")))
		{
			FrostSlot.Root = Root;
			FrostSlot.Icon = Cast<UImage>(GetWidgetFromName(TEXT("FrostIcon")));
			FrostSlot.TimeBar = Cast<UProgressBar>(GetWidgetFromName(TEXT("FrostTimeBar")));
		}
		ApplyIcons();
		SetMarks(bShowCrack, bShowFrost);
	}
	return Super::RebuildWidget();
}

USMarkIconWidget::FMarkSlot USMarkIconWidget::MakeSlot(const FLinearColor& BarColor)
{
	FMarkSlot MarkSlot;

	// 칸 = 세로 상자 (위: 아이콘, 아래: 남은 시간 막대). 폭은 아이콘 크기로 고정
	USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Box->SetWidthOverride(IconSize);
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Box->AddChild(Column);

	MarkSlot.Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	MarkSlot.Icon->SetDesiredSizeOverride(FVector2D(IconSize, IconSize));
	Column->AddChildToVerticalBox(MarkSlot.Icon)->SetHorizontalAlignment(HAlign_Center);

	USizeBox* BarBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	BarBox->SetHeightOverride(BarHeight);
	MarkSlot.TimeBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
	MarkSlot.TimeBar->SetFillColorAndOpacity(BarColor);
	MarkSlot.TimeBar->SetPercent(1.f);
	BarBox->AddChild(MarkSlot.TimeBar);
	UVerticalBoxSlot* BarSlot = Column->AddChildToVerticalBox(BarBox);
	BarSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));

	MarkSlot.Root = Box;
	return MarkSlot;
}

void USMarkIconWidget::ApplyIcons()
{
	if (CrackSlot.Icon && CrackMaterial)
	{
		CrackSlot.Icon->SetBrushFromMaterial(CrackMaterial);
		CrackSlot.Icon->SetDesiredSizeOverride(FVector2D(IconSize, IconSize));
	}
	if (FrostSlot.Icon && FrostMaterial)
	{
		FrostSlot.Icon->SetBrushFromMaterial(FrostMaterial);
		FrostSlot.Icon->SetDesiredSizeOverride(FVector2D(IconSize, IconSize));
	}
}

void USMarkIconWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	BlinkTime += InDeltaTime;
	UpdateSlot(CrackSlot, SGameplayTags::Mark_Crack, BlinkTime);
	UpdateSlot(FrostSlot, SGameplayTags::Mark_Frost, BlinkTime);
}

void USMarkIconWidget::UpdateSlot(const FMarkSlot& MarkSlot, const FGameplayTag& MarkTag, float Time)
{
	const USMarkIndicatorComponent* Owner = OwnerComponent.Get();
	if (!Owner || !MarkSlot.Root || MarkSlot.Root->GetVisibility() == ESlateVisibility::Collapsed)
	{
		return;
	}

	float Fraction = 0.f;
	float Seconds = 0.f;
	if (!Owner->GetMarkTimeRemaining(MarkTag, Fraction, Seconds))
	{
		return;
	}

	if (MarkSlot.TimeBar)
	{
		MarkSlot.TimeBar->SetPercent(FMath::Clamp(Fraction, 0.f, 1.f));
	}

	// 곧 사라지면 깜빡임 (투명도 0.3 ~ 1)
	const float Opacity = Seconds <= BlinkStartSeconds ? 0.65f + 0.35f * FMath::Cos(Time * BlinkSpeed) : 1.f;
	MarkSlot.Root->SetRenderOpacity(Opacity);
}
