#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class USCameraConfig;
struct FInputActionValue;

UCLASS()
class TWINSWIFT_API ASPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASPlayerController();

	// 스킬이 겨냥할 위치. 시점마다 구하는 방법만 다르고, 스킬은 이 함수만 부름
	UFUNCTION(BlueprintCallable, Category = "Aim")
	FVector GetAimPoint() const;

	// 시점 바꾸기 (CameraConfigs 순서대로)
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void CycleCameraConfig();

	// [디버그] 겨냥 위치를 구로 표시 (콘솔: ShowAim)
	UFUNCTION(Exec)
	void ShowAim();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void AcknowledgePossession(APawn* P) override;
	virtual void PlayerTick(float DeltaTime) override;

	// 비교할 시점들. 첫 번째가 시작 시점
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	TArray<TObjectPtr<USCameraConfig>> CameraConfigs;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> WalkAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> CycleCameraAction;

private:
	void Move_Input(const FInputActionValue& InputValue);
	void Look_Input(const FInputActionValue& InputValue);
	void WalkStarted_Input();
	void WalkCompleted_Input();

	// 현재 시점 설정을 캐릭터·커서·시야 제한에 적용
	void ApplyCurrentCameraConfig();

	const USCameraConfig* GetCurrentCameraConfig() const;

	FVector GetAimPointUnderCursor() const;
	FVector GetAimPointAtScreenCenter() const;

	// 아무것도 맞지 않았을 때 쓰는 위치 (캐릭터 앞)
	FVector GetFallbackAimPoint() const;

	int32 CurrentCameraIndex = 0;

	bool bDrawAimPoint = false;
};
