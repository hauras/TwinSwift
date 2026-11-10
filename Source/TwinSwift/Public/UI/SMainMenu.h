#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "Styling/SlateBrush.h"
#include "SMainMenu.generated.h"

class SWidgetSwitcher;

UCLASS()
class TWINSWIFT_API USMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	USMainMenuWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void NativeDestruct() override;

private:
	TSharedRef<SWidget> BuildTitle();
	TSharedRef<SWidget> BuildControls();
	TSharedRef<SWidget> BuildSettings();
	TSharedRef<SWidget> BuildVideoConfirmation();

	void OpenSettings();
	void ApplySettings();
	void SaveSettings();
	void RevertVideo();
	void ResetDraft();

	UPROPERTY()
	FSlateBrush TitleBackdrop;

	UPROPERTY()
	FSlateBrush ControlsArtwork;

	TSharedPtr<SWidgetSwitcher> Pages;
	TArray<FIntPoint> Resolutions;
	FIntPoint Resolution = FIntPoint::ZeroValue;
	EWindowMode::Type WindowMode = EWindowMode::WindowedFullscreen;
	int32 Quality = 2;
	float FrameLimit = 60.f;
	FTimerHandle VideoTimer;
	bool bPreviewingVideo = false;
};

UCLASS()
class TWINSWIFT_API ASMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<USMainMenuWidget> Menu;
};

UCLASS()
class TWINSWIFT_API ASMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASMainMenuGameMode();
};
