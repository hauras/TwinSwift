#include "UI/SMainMenu.h"

#include "Camera/CameraActor.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "TwinSwiftMainMenu"

namespace
{
	const FLinearColor Ink(0.018f, 0.026f, 0.043f, 0.97f);
	const FLinearColor Ivory(1.f, 0.92f, 0.73f);
	const FLinearColor Gold(0.86f, 0.51f, 0.16f);
	const FLinearColor Blue(0.39f, 0.79f, 1.f);
	constexpr float TitleWidth = 1672.f;
	constexpr float TitleHeight = 941.f;
	constexpr float FrameLimits[] = { 60.f, 120.f, 0.f };
	constexpr EWindowMode::Type WindowModes[] = {
		EWindowMode::Fullscreen, EWindowMode::WindowedFullscreen, EWindowMode::Windowed
	};

	TSharedRef<STextBlock> Label(const TAttribute<FText>& Text, int32 Size = 18, FLinearColor Color = Ivory)
	{
		return SNew(STextBlock)
			.Text(Text)
			.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), Size))
			.ColorAndOpacity(Color)
			.Justification(ETextJustify::Center);
	}

	TSharedRef<SWidget> Button(const FText& Text, TFunction<void()> Action, bool bPrimary = false,
		float Height = 45.f, int32 TextSize = 18)
	{
		return SNew(SBox).MinDesiredHeight(Height)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(Gold)
			.Padding(1.f)
			[
				SNew(SButton)
				.ButtonColorAndOpacity(bPrimary ? FLinearColor(0.43f, 0.23f, 0.08f) : Ink)
				.ContentPadding(FMargin(20.f, 7.f))
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.OnClicked_Lambda([Action = MoveTemp(Action)]
				{
					Action();
					return FReply::Handled();
				})
				[
					Label(Text, TextSize)
				]
			]
		];
	}

	TSharedRef<SWidget> Panel(const TSharedRef<SWidget>& Content)
	{
		const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
		return SNew(SBorder).BorderImage(White).BorderBackgroundColor(Gold).Padding(1.f)
		[
			SNew(SBorder).BorderImage(White).BorderBackgroundColor(Ink).Padding(27.f)
			[
				Content
			]
		];
	}

	TSharedRef<SWidget> SettingRow(const FText& Name, TFunction<FText()> Value, TFunction<void(int32)> Step)
	{
		return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).HAlign(HAlign_Left)
		[
			Label(Name)
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			Button(FText::FromString(TEXT("‹")), [Step] { Step(-1); })
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(205.f).HAlign(HAlign_Center)
			[
				Label(TAttribute<FText>::CreateLambda([Value = MoveTemp(Value)] { return Value(); }), 17, Blue)
			]
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			Button(FText::FromString(TEXT("›")), [Step] { Step(1); })
		];
	}

	template <typename T>
	T StepOption(T Value, TArrayView<const T> Options, int32 Step)
	{
		int32 Index = Options.Find(Value);
		if (Index == INDEX_NONE)
		{
			return Options[0];
		}
		return Options[(Index + Step + Options.Num()) % Options.Num()];
	}

	FText WindowModeText(EWindowMode::Type Mode)
	{
		switch (Mode)
		{
		case EWindowMode::Fullscreen: return LOCTEXT("Fullscreen", "전체 화면");
		case EWindowMode::Windowed: return LOCTEXT("Windowed", "창 모드");
		default: return LOCTEXT("Borderless", "테두리 없는 창");
		}
	}
}

USMainMenuWidget::USMainMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static const ConstructorHelpers::FObjectFinder<UTexture2D> Backdrop(
		TEXT("/Game/UI/Menu/T_TitleBackdrop.T_TitleBackdrop"));
	if (Backdrop.Succeeded())
	{
		TitleBackdrop.SetResourceObject(Backdrop.Object.Get());
		TitleBackdrop.SetImageSize(FVector2D(TitleWidth, TitleHeight));
		TitleBackdrop.DrawAs = ESlateBrushDrawType::Image;
	}

	static const ConstructorHelpers::FObjectFinder<UTexture2D> Controls(
		TEXT("/Game/UI/Menu/T_ControlsArtwork.T_ControlsArtwork"));
	if (Controls.Succeeded())
	{
		ControlsArtwork.SetResourceObject(Controls.Object.Get());
		ControlsArtwork.SetImageSize(FVector2D(TitleWidth, TitleHeight));
		ControlsArtwork.DrawAs = ESlateBrushDrawType::Image;
	}
}

TSharedRef<SWidget> USMainMenuWidget::RebuildWidget()
{
	const bool bHasBackdrop = TitleBackdrop.GetResourceObject() != nullptr;
	SAssignNew(Pages, SWidgetSwitcher)
		+ SWidgetSwitcher::Slot()[BuildTitle()]
		+ SWidgetSwitcher::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[BuildControls()]
		+ SWidgetSwitcher::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[BuildSettings()]
		+ SWidgetSwitcher::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[BuildVideoConfirmation()];

	if (!bHasBackdrop)
	{
		return SNew(SOverlay) + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[Pages.ToSharedRef()];
	}

	return SNew(SOverlay)
	+ SOverlay::Slot()
	[
		SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor::Black)
	]
	+ SOverlay::Slot()
	[
		SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
		[
			SNew(SBox).WidthOverride(TitleWidth).HeightOverride(TitleHeight)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SImage).Image(&TitleBackdrop).Visibility(EVisibility::HitTestInvisible)
				]
				+ SOverlay::Slot()
				[
					SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.BorderBackgroundColor(FLinearColor(0.015f, 0.022f, 0.04f, 0.58f))
					.Visibility_Lambda([this]
					{
						return Pages.IsValid() && Pages->GetActiveWidgetIndex() > 0
							? EVisibility::HitTestInvisible : EVisibility::Collapsed;
					})
				]
				+ SOverlay::Slot()[Pages.ToSharedRef()]
			]
		]
	];
}

void USMainMenuWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Pages.Reset();
}

void USMainMenuWidget::NativeDestruct()
{
	RevertVideo();
	Super::NativeDestruct();
}

TSharedRef<SWidget> USMainMenuWidget::BuildTitle()
{
	const bool bHasBackdrop = TitleBackdrop.GetResourceObject() != nullptr;
	const float Gap = bHasBackdrop ? 21.f : 12.f;
	const float Height = bHasBackdrop ? 74.f : 45.f;
	const int32 TextSize = bHasBackdrop ? 35 : 18;
	const TSharedRef<SWidget> MenuButtons = SNew(SBox).WidthOverride(bHasBackdrop ? 440.f : 355.f)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).Padding(0.f, 0.f, 0.f, 28.f)
			.Visibility(bHasBackdrop ? EVisibility::Collapsed : EVisibility::Visible)
			[
				Label(LOCTEXT("Title", "TwinSwift"), 58)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, Gap)
		[
			Button(LOCTEXT("Start", "보스 러시 시작"), [this]
			{
				UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Maps/L_StoneArena")));
			}, true, bHasBackdrop ? 88.f : Height, TextSize)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, Gap)
		[
			Button(LOCTEXT("Controls", "조작법"), [this] { Pages->SetActiveWidgetIndex(1); }, false, Height, TextSize)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, Gap)
		[
			Button(LOCTEXT("Settings", "설정"), [this] { OpenSettings(); }, false, Height, TextSize)
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			Button(LOCTEXT("Quit", "종료"), [this]
			{
				UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
			}, false, Height, TextSize)
		]
	];

	if (!bHasBackdrop)
	{
		return MenuButtons;
	}

	return SNew(SOverlay)
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0.f, 376.f, 0.f, 0.f)
		[
			MenuButtons
		];
}

TSharedRef<SWidget> USMainMenuWidget::BuildControls()
{
	if (TitleBackdrop.GetResourceObject() && ControlsArtwork.GetResourceObject())
	{
		return SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SImage).Image(&ControlsArtwork).Visibility(EVisibility::HitTestInvisible)
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 0.f, 55.f)
			[
				SNew(SBox).WidthOverride(260.f).HeightOverride(74.f)
				[
					SNew(SButton)
					.ButtonStyle(FCoreStyle::Get(), "NoBorder")
					.Cursor(EMouseCursor::Hand)
					.ToolTipText(LOCTEXT("Back", "뒤로"))
					.OnClicked_Lambda([this]
					{
						Pages->SetActiveWidgetIndex(0);
						return FReply::Handled();
					})
				]
			];
	}

	return Panel(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 20.f)
		[
			Label(LOCTEXT("ControlsHeading", "조작법"), 32)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 24.f)
		[
			Label(LOCTEXT("ControlsList", "WASD  이동     마우스  조준\n좌클릭  기본 공격     우클릭  특수 스킬\nSpace  회피     1 / 2  전사 스킬     3 / 4  마법사 스킬\n\n다른 캐릭터의 스킬 키를 누르면 교체하면서 바로 시전합니다."), 17)
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			Button(LOCTEXT("Back", "뒤로"), [this] { Pages->SetActiveWidgetIndex(0); })
		]);
}

TSharedRef<SWidget> USMainMenuWidget::BuildSettings()
{
	return Panel(
		SNew(SBox).WidthOverride(530.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 20.f)
			[
				Label(LOCTEXT("SettingsHeading", "설정"), 32)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f)
			[
				SettingRow(LOCTEXT("DisplayMode", "화면 모드"), [this] { return WindowModeText(WindowMode); },
					[this](int32 Step) { WindowMode = StepOption(WindowMode, MakeArrayView(WindowModes), Step); })
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f)
			[
				SettingRow(LOCTEXT("Resolution", "해상도"), [this]
				{
					const FIntPoint Size = WindowMode == EWindowMode::WindowedFullscreen
						? GEngine->GetGameUserSettings()->GetDesktopResolution() : Resolution;
					return FText::Format(LOCTEXT("ResolutionValue", "{0} × {1}"),
						FText::AsNumber(Size.X, &FNumberFormattingOptions::DefaultNoGrouping()),
						FText::AsNumber(Size.Y, &FNumberFormattingOptions::DefaultNoGrouping()));
				}, [this](int32 Step)
				{
					if (WindowMode != EWindowMode::WindowedFullscreen && !Resolutions.IsEmpty())
					{
						Resolution = StepOption(Resolution, TArrayView<const FIntPoint>(Resolutions), Step);
					}
				})
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f)
			[
				SettingRow(LOCTEXT("Quality", "그래픽 품질"), [this]
				{
					const FText Options[] = { LOCTEXT("Low", "낮음"), LOCTEXT("Medium", "중간"),
						LOCTEXT("High", "높음"), LOCTEXT("Epic", "매우 높음"), LOCTEXT("Cinematic", "시네마틱") };
					return Quality == INDEX_NONE ? LOCTEXT("Custom", "사용자 지정") : Options[Quality];
				}, [this](int32 Step) { Quality = FMath::Clamp(Quality + Step, 0, 4); })
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f)
			[
				SettingRow(LOCTEXT("FrameLimit", "프레임 제한"), [this]
				{
					return FrameLimit > 0.f
						? FText::Format(LOCTEXT("Fps", "{0} FPS"), FText::AsNumber(FMath::RoundToInt(FrameLimit)))
						: LOCTEXT("Unlimited", "제한 없음");
				}, [this](int32 Step) { FrameLimit = StepOption(FrameLimit, MakeArrayView(FrameLimits), Step); })
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 20.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 10.f, 0.f)
				[
					Button(LOCTEXT("Defaults", "기본값"), [this] { ResetDraft(); })
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 10.f, 0.f)
				[
					Button(LOCTEXT("Back", "뒤로"), [this] { Pages->SetActiveWidgetIndex(0); })
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					Button(LOCTEXT("Apply", "적용"), [this] { ApplySettings(); }, true)
				]
			]
		]);
}

TSharedRef<SWidget> USMainMenuWidget::BuildVideoConfirmation()
{
	return Panel(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 18.f)
		[
			Label(LOCTEXT("ConfirmHeading", "화면 설정을 유지할까요?"), 26)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 18.f)
		[
			Label(TAttribute<FText>::CreateLambda([this]
			{
				const float Remaining = GetWorld() ? GetWorld()->GetTimerManager().GetTimerRemaining(VideoTimer) : 0.f;
				return FText::Format(LOCTEXT("ConfirmCountdown", "{0}초 뒤 이전 설정으로 돌아갑니다."),
					FText::AsNumber(FMath::Max(0, FMath::CeilToInt(Remaining))));
			}))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 10.f, 0.f)
			[
				Button(LOCTEXT("Revert", "되돌리기"), [this] { RevertVideo(); OpenSettings(); })
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				Button(LOCTEXT("Confirm", "유지"), [this] { SaveSettings(); }, true)
			]
		]);
}

void USMainMenuWidget::OpenSettings()
{
	const UGameUserSettings* Settings = GEngine->GetGameUserSettings();
	WindowMode = Settings->GetFullscreenMode();
	Resolution = Settings->GetScreenResolution();
	if (Resolution.X <= 0 || Resolution.Y <= 0)
	{
		Resolution = Settings->GetDesktopResolution();
	}
	Quality = FMath::Clamp(Settings->GetOverallScalabilityLevel(), INDEX_NONE, 4);
	FrameLimit = Settings->GetFrameRateLimit();

	Resolutions.Reset();
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions);
	const FIntPoint Desktop = Settings->GetDesktopResolution();
	Resolutions.RemoveAll([Desktop](FIntPoint Size)
	{
		return Size.X < 1024 || Size.Y < 576 || Size.X > Desktop.X || Size.Y > Desktop.Y;
	});
	Resolutions.AddUnique(Resolution);
	Resolutions.Sort([](FIntPoint A, FIntPoint B) { return A.X == B.X ? A.Y < B.Y : A.X < B.X; });
	Pages->SetActiveWidgetIndex(2);
}

void USMainMenuWidget::ResetDraft()
{
	WindowMode = EWindowMode::WindowedFullscreen;
	Resolution = GEngine->GetGameUserSettings()->GetDesktopResolution();
	Resolutions.AddUnique(Resolution);
	Quality = 2;
	FrameLimit = 60.f;
}

void USMainMenuWidget::ApplySettings()
{
	UGameUserSettings* Settings = GEngine->GetGameUserSettings();
	const FIntPoint RequestedSize = WindowMode == EWindowMode::WindowedFullscreen
		? Settings->GetDesktopResolution() : Resolution;
	if (Settings->GetFullscreenMode() == WindowMode
		&& (WindowMode == EWindowMode::WindowedFullscreen || Settings->GetScreenResolution() == RequestedSize))
	{
		SaveSettings();
		return;
	}

	// PIE의 편집기 창은 비디오 모드 미리보기 대상에서 제외한다.
	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		Settings->SetFullscreenMode(WindowMode);
		Settings->SetScreenResolution(RequestedSize);
		SaveSettings();
		return;
	}

	Settings->ConfirmVideoMode();
	Settings->SetFullscreenMode(WindowMode);
	Settings->SetScreenResolution(RequestedSize);
	bPreviewingVideo = true;
	Settings->ApplyResolutionSettings(false);
	Pages->SetActiveWidgetIndex(3);
	GetWorld()->GetTimerManager().SetTimer(VideoTimer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		RevertVideo();
		OpenSettings();
	}), 10.f, false);
}

void USMainMenuWidget::SaveSettings()
{
	GetWorld()->GetTimerManager().ClearTimer(VideoTimer);
	bPreviewingVideo = false;

	UGameUserSettings* Settings = GEngine->GetGameUserSettings();
	Settings->ConfirmVideoMode();
	if (Quality != INDEX_NONE)
	{
		Settings->SetOverallScalabilityLevel(Quality);
	}
	Settings->SetFrameRateLimit(FrameLimit);
	Settings->ApplyNonResolutionSettings();
	Settings->SaveSettings();
	OpenSettings();
}

void USMainMenuWidget::RevertVideo()
{
	if (!bPreviewingVideo)
	{
		return;
	}
	bPreviewingVideo = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(VideoTimer);
	}
	UGameUserSettings* Settings = GEngine->GetGameUserSettings();
	Settings->RevertVideoMode();
	Settings->ApplyResolutionSettings(false);
}

void ASMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}

	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("MainMenuCamera")))
		{
			bAutoManageActiveCameraTarget = false;
			SetViewTarget(*It);
			break;
		}
	}

	Menu = CreateWidget<USMainMenuWidget>(this, USMainMenuWidget::StaticClass());
	if (Menu)
	{
		Menu->AddToViewport();
		SetShowMouseCursor(true);
		SetInputMode(FInputModeUIOnly());
	}
}

ASMainMenuGameMode::ASMainMenuGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = ASMainMenuPlayerController::StaticClass();
}

#undef LOCTEXT_NAMESPACE
