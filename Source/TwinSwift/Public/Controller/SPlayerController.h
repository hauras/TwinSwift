#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "SPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class USCameraConfig;
class USInputConfig;
class USAbilitySystemComponent;
class USTwinHUDWidget;
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

	// HUD 키 표시는 실제 입력 매핑에서 읽는다.
	const USInputConfig* GetHUDInputConfig() const { return AbilityInputConfig; }
	const UInputMappingContext* GetHUDMappingContext() const { return DefaultMappingContext; }

	// 시점 바꾸기 (CameraConfigs 순서대로)
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void CycleCameraConfig();

	// 내가 때린 데미지 숫자를 대상 머리 위에 띄움
	void ShowDamageNumber(float Damage, AActor* Target, bool bShatter = false);

	// [디버그] 겨냥 위치를 구로 표시 (콘솔: ShowAim)
	UFUNCTION(Exec)
	void ShowAim();

	// [디버그] 지금 나와 있는 캐릭터에게 데미지 (콘솔: DebugDamage 50). 적이 아직 플레이어를 때리지 않아서 교체 규칙 시험용
	UFUNCTION(Exec)
	void DebugDamage(float Amount);

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

	// 캐릭터 교체 (전사 ↔ 마법사)
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SwapAction;

	// 능력 입력 목록 (IA_Attack → InputTag.Attack ...)
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<USInputConfig> AbilityInputConfig;

	// 전투 HUD 위젯 블루프린트 (WBP_TwinHUD, 부모 USTwinHUDWidget)
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USTwinHUDWidget> HUDWidgetClass;

private:
	void Move_Input(const FInputActionValue& InputValue);
	void Look_Input(const FInputActionValue& InputValue);
	void WalkStarted_Input();
	void WalkCompleted_Input();
	void Swap_Input();

	// 능력 키 → 입력 태그 → 그 입력을 받을 캐릭터의 ASC (상대 캐릭터 스킬 키면 먼저 교체)
	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);

	// 지금 나와 있는 캐릭터의 ASC (PlayerState가 아직 없으면 null)
	USAbilitySystemComponent* GetSAbilitySystemComponent() const;

	// 공격·경직·사망 중에는 이동 입력을 받지 않음 (멈추게 하지 않고 입력만 막아 자연 감속)
	bool IsMoveInputBlocked() const;

	// HUD를 (처음 한 번만) 만들고 지금 캐릭터에 연결
	void SetupHUD();

	// 현재 시점 설정을 캐릭터·커서·시야 제한에 적용
	void ApplyCurrentCameraConfig();

	const USCameraConfig* GetCurrentCameraConfig() const;

	FVector GetAimPointUnderCursor() const;
	FVector GetAimPointAtScreenCenter() const;

	// 아무것도 맞지 않았을 때 쓰는 위치 (캐릭터 앞)
	FVector GetFallbackAimPoint() const;

	// 한 번만 만들고, 다시 빙의해도 연결만 바꿈 (1차: 교체·빙의마다 위젯이 쌓였음)
	UPROPERTY()
	TObjectPtr<USTwinHUDWidget> HUDWidget;

	int32 CurrentCameraIndex = 0;

	bool bDrawAimPoint = false;
};
