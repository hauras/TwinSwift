#include "Controller/SPlayerController.h"

#include "DrawDebugHelpers.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Ability/SAbilitySystemComponent.h"
#include "Ability/SAttributeSet.h"
#include "Camera/SCameraConfig.h"
#include "Camera/PlayerCameraManager.h"
#include "Character/PlayerCharacter.h"
#include "Input/SInputConfig.h"
#include "SGameplayTags.h"
#include "State/SPlayerState.h"
#include "UI/DamageTextActor.h"
#include "UI/STwinHUDWidget.h"

namespace
{
	// 입력을 받을 캐릭터: 캐릭터 전용 스킬 키면 그 캐릭터, 아니면(공격·회피) 지금 캐릭터
	ETwinSlot GetSlotForInput(const FGameplayTag& InputTag, ETwinSlot ActiveSlot)
	{
		if (InputTag == SGameplayTags::InputTag_Warrior_Skill1 || InputTag == SGameplayTags::InputTag_Warrior_Skill2)
		{
			return ETwinSlot::Warrior;
		}
		if (InputTag == SGameplayTags::InputTag_Mage_Skill1 || InputTag == SGameplayTags::InputTag_Mage_Skill2)
		{
			return ETwinSlot::Mage;
		}
		return ActiveSlot;
	}
}

ASPlayerController::ASPlayerController()
{
}

void ASPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ASPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInput)
	{
		UE_LOG(LogTemp, Error, TEXT("입력 컴포넌트가 EnhancedInputComponent가 아님 (프로젝트 설정 > Input 확인)"));
		return;
	}

	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move_Input);
	EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look_Input);
	EnhancedInput->BindAction(WalkAction, ETriggerEvent::Started, this, &ThisClass::WalkStarted_Input);
	EnhancedInput->BindAction(WalkAction, ETriggerEvent::Completed, this, &ThisClass::WalkCompleted_Input);
	EnhancedInput->BindAction(CycleCameraAction, ETriggerEvent::Started, this, &ThisClass::CycleCameraConfig);
	EnhancedInput->BindAction(SwapAction, ETriggerEvent::Started, this, &ThisClass::Swap_Input);

	// 능력 입력: 키마다 입력 태그를 함께 넘겨서, 어떤 키인지 태그로 구분
	if (AbilityInputConfig)
	{
		for (const FSInputActionConfig& Action : AbilityInputConfig->AbilityInputActions)
		{
			if (!Action.InputAction || !Action.InputTag.IsValid())
			{
				continue;
			}

			EnhancedInput->BindAction(Action.InputAction, ETriggerEvent::Started, this, &ThisClass::AbilityInputTagPressed, Action.InputTag);
			EnhancedInput->BindAction(Action.InputAction, ETriggerEvent::Completed, this, &ThisClass::AbilityInputTagReleased, Action.InputTag);
		}
	}
}

void ASPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);

	// 캐릭터를 잡은 직후 불림 → 카메라 설정·HUD 연결은 여기서
	ApplyCurrentCameraConfig();
	SetupHUD();
}

void ASPlayerController::SetupHUD()
{
	if (!IsLocalController())
	{
		return;
	}

	if (!HUDWidget)
	{
		// 화면 배치는 WBP에 있으므로 BP_PlayerController의 HUDWidgetClass에 WBP_TwinHUD를 넣어야 함
		if (!HUDWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("HUDWidgetClass가 비어 있음 (BP_PlayerController에 WBP_TwinHUD 지정)"));
			return;
		}

		HUDWidget = CreateWidget<USTwinHUDWidget>(this, HUDWidgetClass);
		if (!HUDWidget)
		{
			return;
		}
		HUDWidget->AddToViewport();
	}

	HUDWidget->BindToCharacter(GetPawn<APlayerCharacter>());
}

void ASPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (bDrawAimPoint)
	{
		DrawDebugSphere(
			GetWorld(),
			GetAimPoint(),
			30.f,
			12,
			FColor::Red);
	}
}

void ASPlayerController::Move_Input(const FInputActionValue& InputValue)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !PlayerCameraManager || IsMoveInputBlocked())
	{
		return;
	}

	// 컨트롤러가 아니라 카메라가 보는 방향 기준 → 쿼터뷰(카메라 고정)와 3인칭 모두 화면 위쪽이 "앞"
	const FRotator CameraYaw(0.f, PlayerCameraManager->GetCameraRotation().Yaw, 0.f);
	const FVector ForwardDirection = FRotationMatrix(CameraYaw).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(CameraYaw).GetUnitAxis(EAxis::Y);

	const FVector2D InputAxis = InputValue.Get<FVector2D>();
	ControlledPawn->AddMovementInput(ForwardDirection, InputAxis.Y);
	ControlledPawn->AddMovementInput(RightDirection, InputAxis.X);
}

void ASPlayerController::Look_Input(const FInputActionValue& InputValue)
{
	// 고정 시점에서는 마우스로 카메라를 돌리지 않음
	const USCameraConfig* Config = GetCurrentCameraConfig();
	if (Config && !Config->bFreeLook)
	{
		return;
	}

	const FVector2D LookAxis = InputValue.Get<FVector2D>();
	AddYawInput(LookAxis.X);
	AddPitchInput(LookAxis.Y);
}

void ASPlayerController::WalkStarted_Input()
{
	if (APlayerCharacter* PlayerCharacter = GetPawn<APlayerCharacter>())
	{
		PlayerCharacter->SetWalking(true);
	}
}

void ASPlayerController::WalkCompleted_Input()
{
	if (APlayerCharacter* PlayerCharacter = GetPawn<APlayerCharacter>())
	{
		PlayerCharacter->SetWalking(false);
	}
}

void ASPlayerController::Swap_Input()
{
	if (APlayerCharacter* PlayerCharacter = GetPawn<APlayerCharacter>())
	{
		PlayerCharacter->SwapCharacter();
	}
}

void ASPlayerController::AbilityInputTagPressed(FGameplayTag InputTag)
{
	APlayerCharacter* PlayerCharacter = GetPawn<APlayerCharacter>();
	const ASPlayerState* SPlayerState = GetPlayerState<ASPlayerState>();
	if (!PlayerCharacter || !SPlayerState || PlayerCharacter->IsDefeated())
	{
		return;
	}

	// 스킬 입력 = 교체: 상대 캐릭터의 스킬 키면 먼저 교체
	// 교체가 막히면(공격 중 등) 그 입력은 버림
	const ETwinSlot Slot = GetSlotForInput(InputTag, PlayerCharacter->GetActiveSlot());
	if (Slot != PlayerCharacter->GetActiveSlot())
	{
		PlayerCharacter->SwapTo(Slot);
		if (PlayerCharacter->GetActiveSlot() != Slot)
		{
			return;
		}
	}

	SPlayerState->GetSAbilitySystemComponent(Slot)->AbilityInputTagPressed(InputTag);
}

void ASPlayerController::AbilityInputTagReleased(FGameplayTag InputTag)
{
	const APlayerCharacter* PlayerCharacter = GetPawn<APlayerCharacter>();
	const ASPlayerState* SPlayerState = GetPlayerState<ASPlayerState>();
	if (!PlayerCharacter || !SPlayerState)
	{
		return;
	}

	// 뗀 키는 교체 없이 그 키의 캐릭터에게만 알림
	const ETwinSlot Slot = GetSlotForInput(InputTag, PlayerCharacter->GetActiveSlot());
	SPlayerState->GetSAbilitySystemComponent(Slot)->AbilityInputTagReleased(InputTag);
}

USAbilitySystemComponent* ASPlayerController::GetSAbilitySystemComponent() const
{
	// PlayerState의 GetAbilitySystemComponent는 지금 나와 있는 캐릭터 것을 돌려줌
	const ASPlayerState* SPlayerState = GetPlayerState<ASPlayerState>();
	return SPlayerState ? Cast<USAbilitySystemComponent>(SPlayerState->GetAbilitySystemComponent()) : nullptr;
}

bool ASPlayerController::IsMoveInputBlocked() const
{
	const USAbilitySystemComponent* SASC = GetSAbilitySystemComponent();
	if (!SASC)
	{
		return false;
	}

	FGameplayTagContainer BlockTags;
	BlockTags.AddTag(SGameplayTags::State_Attacking);
	BlockTags.AddTag(SGameplayTags::State_Stun);
	BlockTags.AddTag(SGameplayTags::State_Dead);
	return SASC->HasAnyMatchingGameplayTags(BlockTags);
}

void ASPlayerController::ShowDamageNumber(float Damage, AActor* Target, bool bShatter)
{
	if (!IsValid(Target))
	{
		return;
	}

	// 월드 Z만 벌리면 쿼터뷰에서 겹침: 화면의 좌우로 두 숫자를 분리
	const FRotator ViewRotation = PlayerCameraManager
		? PlayerCameraManager->GetCameraRotation()
		: GetControlRotation();
	const FVector ScreenRight = FRotationMatrix(ViewRotation).GetUnitAxis(EAxis::Y);
	const FVector ScreenUp = FRotationMatrix(ViewRotation).GetUnitAxis(EAxis::Z);
	const float SideOffset = bShatter ? 90.f : -60.f;
	const FVector Location = Target->GetActorLocation()
		+ FVector(0.f, 0.f, 120.f)
		+ ScreenRight * (SideOffset + FMath::RandRange(-8.f, 8.f))
		+ ScreenUp * (bShatter ? 30.f : 0.f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ADamageTextActor* DamageText = GetWorld()->SpawnActor<ADamageTextActor>(Location, FRotator::ZeroRotator, Params))
	{
		DamageText->SetDamage(Damage, bShatter);
	}
}

void ASPlayerController::CycleCameraConfig()
{
	if (CameraConfigs.Num() < 2)
	{
		return;
	}

	CurrentCameraIndex = (CurrentCameraIndex + 1) % CameraConfigs.Num();
	ApplyCurrentCameraConfig();
}

void ASPlayerController::ShowAim()
{
	bDrawAimPoint = !bDrawAimPoint;
}

void ASPlayerController::DebugDamage(float Amount)
{
	const ASPlayerState* SPlayerState = GetPlayerState<ASPlayerState>();
	UAbilitySystemComponent* ASC = SPlayerState ? SPlayerState->GetAbilitySystemComponent() : nullptr;
	if (!ASC || Amount <= 0.f)
	{
		return;
	}

	// 체력을 직접 깎음 (0 아래로는 안 내려가게). 체력 변경 알림이 나가서 HUD·쓰러짐 처리가 실제 피격과 같이 동작
	// ApplyModToAttribute는 GE 실행 콜백(PostGameplayEffectExecute)을 거치지 않아 IncomingDamage로는 체력이 안 줄어듦
	const float Health = ASC->GetNumericAttribute(USAttributeSet::GetHealthAttribute());
	ASC->ApplyModToAttribute(USAttributeSet::GetHealthAttribute(), EGameplayModOp::Override, FMath::Max(0.f, Health - Amount));
}

void ASPlayerController::ApplyCurrentCameraConfig()
{
	const USCameraConfig* Config = GetCurrentCameraConfig();
	APlayerCharacter* PlayerCharacter = GetPawn<APlayerCharacter>();
	if (!Config || !PlayerCharacter)
	{
		return;
	}

	// 고정 시점 → 자유 시점으로 바뀔 때 화면이 휙 돌지 않게, 지금 보던 방향에서 시작
	if (Config->bFreeLook && PlayerCameraManager)
	{
		FRotator StartRotation = PlayerCameraManager->GetCameraRotation();
		StartRotation.Pitch = FMath::Clamp(StartRotation.Pitch, Config->MinPitch, Config->MaxPitch);
		StartRotation.Roll = 0.f;
		SetControlRotation(StartRotation);
	}

	PlayerCharacter->ApplyCameraConfig(Config);

	if (PlayerCameraManager)
	{
		PlayerCameraManager->ViewPitchMin = Config->MinPitch;
		PlayerCameraManager->ViewPitchMax = Config->MaxPitch;
	}

	// 커서로 겨냥하는 시점에서만 커서를 보여 줌
	const bool bUseCursor = Config->AimMode == ESAimMode::Cursor;
	SetShowMouseCursor(bUseCursor);
	if (bUseCursor)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
	}
}

const USCameraConfig* ASPlayerController::GetCurrentCameraConfig() const
{
	return CameraConfigs.IsValidIndex(CurrentCameraIndex) ? CameraConfigs[CurrentCameraIndex].Get() : nullptr;
}

FVector ASPlayerController::GetAimPoint() const
{
	const USCameraConfig* Config = GetCurrentCameraConfig();
	if (Config && Config->AimMode == ESAimMode::Cursor)
	{
		return GetAimPointUnderCursor();
	}

	return GetAimPointAtScreenCenter();
}

FVector ASPlayerController::GetAimPointUnderCursor() const
{
	FHitResult Hit;
	if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		return Hit.ImpactPoint;
	}

	return GetFallbackAimPoint();
}

FVector ASPlayerController::GetAimPointAtScreenCenter() const
{
	int32 ViewportX = 0;
	int32 ViewportY = 0;
	GetViewportSize(ViewportX, ViewportY);

	FVector WorldLocation;
	FVector WorldDirection;
	if (!DeprojectScreenPositionToWorld(ViewportX * 0.5f, ViewportY * 0.5f, WorldLocation, WorldDirection))
	{
		return GetFallbackAimPoint();
	}

	const FVector TraceEnd = WorldLocation + WorldDirection * 10000.f;

	// 카메라와 캐릭터 사이에 내 캐릭터가 걸리지 않게 제외
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetPawn());

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, WorldLocation, TraceEnd, ECC_Visibility, Params))
	{
		return Hit.ImpactPoint;
	}

	return TraceEnd;
}

FVector ASPlayerController::GetFallbackAimPoint() const
{
	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return FVector::ZeroVector;
	}

	return ControlledPawn->GetActorLocation() + ControlledPawn->GetActorForwardVector() * 1000.f;
}
