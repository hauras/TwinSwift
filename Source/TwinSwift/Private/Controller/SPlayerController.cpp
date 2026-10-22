#include "Controller/SPlayerController.h"

#include "DrawDebugHelpers.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Ability/SAbilitySystemComponent.h"
#include "Ability/SAttributeSet.h"
#include "Camera/SCameraConfig.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/SCameraShake_Shatter.h"
#include "Ability/SGE_Cooldown.h"
#include "Ability/SGE_Damage.h"
#include "Projectile/SProjectile.h"
#include "Pool/SActorPool.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "HAL/IConsoleManager.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/CoreDelegates.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UObjectArray.h"
#include <atomic>
#include "AI/SBossPatternComponent.h"
#include "Character/BossCharacter.h"
#include "Character/EnemyCharacter.h"
#include "Character/PlayerCharacter.h"
#include "EngineUtils.h"
#include "Input/SInputBufferComponent.h"
#include "Input/SInputConfig.h"
#include "SGameplayTags.h"
#include "State/SPlayerState.h"
#include "UI/DamageTextActor.h"
#include "UI/STwinHUDWidget.h"

namespace
{
	// [측정] 측정 동안 새로 만들어진 UObject 수 (액터, 컴포넌트, 이펙트 등). 풀의 효과 = 버리는 객체가 줄어듦
	// 비동기 로딩 스레드에서도 불릴 수 있어서 원자 변수
	class FBenchObjectCreateCounter : public FUObjectArray::FUObjectCreateListener
	{
	public:
		std::atomic<int32> Count{ 0 };

		virtual void NotifyUObjectCreated(const UObjectBase* Object, int32 Index) override
		{
			Count.fetch_add(1, std::memory_order_relaxed);
		}

		virtual void OnUObjectArrayShutdown() override
		{
			GUObjectArray.RemoveUObjectCreateListener(this);
		}
	};
	FBenchObjectCreateCounter BenchObjectCounter;

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
	// C++ 클래스라 에셋 경로 없이 기본값으로 넣음 (BP에서 다른 흔들림으로 바꿀 수 있음)
	MarkCameraShake = USCameraShake_Shatter::StaticClass();
	HurtCameraShake = USCameraShake_Shatter::StaticClass();

	InputBuffer = CreateDefaultSubobject<USInputBufferComponent>("InputBuffer");
}

void ASPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	// [측정] 실행 인자 -Bench=60,20 이면 로딩이 가라앉도록 3초 뒤 자동으로 BenchProjectiles 60 20
	FString BenchArgs;
	// 쉼표에서 끊지 않게 (기본은 쉼표를 구분자로 봄)
	if (FParse::Value(FCommandLine::Get(), TEXT("Bench="), BenchArgs, false))
	{
		FString PerSecondText;
		FString SecondsText;
		if (BenchArgs.Split(TEXT(","), &PerSecondText, &SecondsText))
		{
			const float PerSecond = FCString::Atof(*PerSecondText);
			const float Seconds = FCString::Atof(*SecondsText);
			FTimerHandle WarmupTimer;
			GetWorldTimerManager().SetTimer(WarmupTimer, FTimerDelegate::CreateWeakLambda(this, [this, PerSecond, Seconds]()
			{
				BenchProjectiles(PerSecond, Seconds);
			}), 3.f, false);
		}
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
	SetupInputBuffer();
}

void ASPlayerController::SetupInputBuffer()
{
	APlayerCharacter* PlayerCharacter = GetPawn<APlayerCharacter>();
	if (!PlayerCharacter || !InputBuffer)
	{
		return;
	}

	// 교체 알림은 캐릭터마다 한 번만 구독 (같은 캐릭터에 다시 빙의해도 쌓이지 않게)
	// 다른 캐릭터면 이전 캐릭터 구독을 끊고 새로 → 핸들만 보면 새 캐릭터를 놓침
	if (InputBufferCharacter.Get() != PlayerCharacter)
	{
		if (APlayerCharacter* OldCharacter = InputBufferCharacter.Get())
		{
			OldCharacter->OnActiveSlotChanged.Remove(ActiveSlotChangedHandle);
		}
		ActiveSlotChangedHandle = PlayerCharacter->OnActiveSlotChanged.AddUObject(this, &ThisClass::OnActiveSlotChanged);
		InputBufferCharacter = PlayerCharacter;
	}

	// 새 캐릭터의 지금 ASC를 듣고, 전 캐릭터로 누른 입력은 버림
	InputBuffer->Clear();
	InputBuffer->BindToASC(GetSAbilitySystemComponent());
}

void ASPlayerController::OnActiveSlotChanged(ETwinSlot NewSlot)
{
	// 전 캐릭터로 누른 입력이 새 캐릭터로 실행되지 않게 버리고, 새 캐릭터의 창·공격 태그를 들음
	InputBuffer->Clear();
	InputBuffer->BindToASC(GetSAbilitySystemComponent());
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
	// 교체도 다른 능력 입력과 같이 버퍼를 거침 (공격의 끊으면 안 되는 구간이면 창이 열릴 때까지 기다림)
	InputBuffer->HandleInput(SGameplayTags::InputTag_Swap);
}

void ASPlayerController::AbilityInputTagPressed(FGameplayTag InputTag)
{
	// 받기만 하고 실행은 버퍼가 판단 (평타처럼 막을 필요 없는 입력은 버퍼가 바로 ExecuteAbilityInput)
	InputBuffer->HandleInput(InputTag);
}

void ASPlayerController::ExecuteAbilityInput(const FGameplayTag& InputTag)
{
	if (InputTag == SGameplayTags::InputTag_Swap)
	{
		if (APlayerCharacter* PlayerCharacter = GetPawn<APlayerCharacter>())
		{
			PlayerCharacter->SwapCharacter();
		}
		return;
	}

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

void ASPlayerController::ShowDamageNumber(float Damage, AActor* Target, EDamageNumberType Type)
{
	if (!IsValid(Target))
	{
		return;
	}

	// 표식 피해(산산조각·균열 폭발)는 일반 숫자와 겹치지 않게 오른쪽 위로, 내가 맞은 숫자는 머리 위 가운데
	const bool bMarkDamage = Type == EDamageNumberType::Shatter || Type == EDamageNumberType::CrackBurst;
	const bool bPlayerHurt = Type == EDamageNumberType::PlayerHurt;

	// 월드 Z만 벌리면 쿼터뷰에서 겹침: 화면의 좌우로 두 숫자를 분리
	const FRotator ViewRotation = PlayerCameraManager
		? PlayerCameraManager->GetCameraRotation()
		: GetControlRotation();
	const FVector ScreenRight = FRotationMatrix(ViewRotation).GetUnitAxis(EAxis::Y);
	const FVector ScreenUp = FRotationMatrix(ViewRotation).GetUnitAxis(EAxis::Z);
	const float SideOffset = bMarkDamage ? 90.f : (bPlayerHurt ? 0.f : -60.f);
	const FVector Location = Target->GetActorLocation()
		+ FVector(0.f, 0.f, 120.f)
		+ ScreenRight * (SideOffset + FMath::RandRange(-8.f, 8.f))
		+ ScreenUp * (bMarkDamage ? 30.f : 0.f);

	// 맞을 때마다 생겨서 풀에서 꺼내 씀 (1초 뒤 스스로 반납)
	USActorPool* Pool = GetWorld()->GetSubsystem<USActorPool>();
	ADamageTextActor* DamageText = Pool ? Pool->Acquire<ADamageTextActor>(
		ADamageTextActor::StaticClass(),
		FTransform(Location)) : nullptr;
	if (DamageText)
	{
		DamageText->SetDamage(Damage, Type);
	}

	// 표식 피해면 때린 플레이어 화면만 흔듦 (Client RPC라 멀티에서도 그 플레이어 화면에만)
	// 균열 연쇄는 단계마다 흔들림이 다시 시작됨 (bSingleInstance) → "쾅, 쾅, 쾅"
	if (bMarkDamage && MarkCameraShake)
	{
		ClientStartCameraShake(MarkCameraShake);
	}
}

void ASPlayerController::OnPawnDamaged(float Damage)
{
	// 보스는 플레이어 컨트롤러가 없어 "때린 쪽이 숫자를 띄우는" 경로로는 안 보임 → 맞은 쪽이 직접 띄움
	ShowDamageNumber(Damage, GetPawn(), EDamageNumberType::PlayerHurt);

	if (HurtCameraShake)
	{
		ClientStartCameraShake(HurtCameraShake);
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

void ASPlayerController::BossPatternSim(int32 Count)
{
	for (TActorIterator<ABossCharacter> It(GetWorld()); It; ++It)
	{
		if (const USBossPatternComponent* Patterns = It->GetPatternComponent())
		{
			// 고정 시드: 몇 번을 돌려도 같은 결과 (README 숫자를 다시 확인할 수 있게)
			Patterns->RunSimulation(Count, 42);
			return;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("[패턴 시뮬레이션] 맵에 보스가 없음"));
}

void ASPlayerController::DebugMarkAll(const FString& MarkName)
{
	FGameplayTag MarkTag;
	if (MarkName.Equals(TEXT("Crack"), ESearchCase::IgnoreCase))
	{
		MarkTag = SGameplayTags::Mark_Crack;
	}
	else if (MarkName.Equals(TEXT("Frost"), ESearchCase::IgnoreCase))
	{
		MarkTag = SGameplayTags::Mark_Frost;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DebugMarkAll: Crack 또는 Frost"));
		return;
	}

	// 실제 스킬과 같은 방식: 공통 시간 태그 GE에 표식 태그와 시간을 넣어 적에게 스스로 적용
	constexpr float DebugMarkDuration = 10.f;
	int32 MarkedCount = 0;
	for (TActorIterator<AEnemyCharacter> It(GetWorld()); It; ++It)
	{
		UAbilitySystemComponent* EnemyASC = It->GetAbilitySystemComponent();
		if (!EnemyASC)
		{
			continue;
		}
		const FGameplayEffectSpecHandle Spec = EnemyASC->MakeOutgoingSpec(USGE_Cooldown::StaticClass(), 1.f, EnemyASC->MakeEffectContext());
		if (!Spec.IsValid())
		{
			continue;
		}
		Spec.Data->DynamicGrantedTags.AddTag(MarkTag);
		Spec.Data->SetSetByCallerMagnitude(SGameplayTags::SetByCaller_Duration, DebugMarkDuration);
		EnemyASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		++MarkedCount;
	}
	UE_LOG(LogTemp, Log, TEXT("DebugMarkAll: 적 %d명에게 %s %.0f초"), MarkedCount, *MarkTag.ToString(), DebugMarkDuration);
}

void ASPlayerController::BenchProjectiles(float PerSecond, float Seconds)
{
	const APawn* ControlledPawn = GetPawn();
	if (!BenchProjectileClass || !ControlledPawn || PerSecond <= 0.f || Seconds <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[벤치] BenchProjectiles 초당발수 초 (예: 60 20). BP_PlayerController의 BenchProjectileClass 확인"));
		return;
	}

	// 같은 조건으로 다시 잴 수 있게: 방향은 가장 가까운 적 쪽으로 고정 (커서는 실행마다 달라서), 퍼짐은 고정 시드
	BenchDirection = ControlledPawn->GetActorForwardVector();
	float NearestDistSq = TNumericLimits<float>::Max();
	for (TActorIterator<AEnemyCharacter> It(GetWorld()); It; ++It)
	{
		const float DistSq = FVector::DistSquared(It->GetActorLocation(), ControlledPawn->GetActorLocation());
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			BenchDirection = (It->GetActorLocation() - ControlledPawn->GetActorLocation()).GetSafeNormal2D();
		}
	}
	BenchRandom.Initialize(42);

	// 실행 인자 -BenchNoPool: 모든 풀(액터 풀 + 나이아가라 이펙트 풀)을 끄고 잼 → 같은 빌드에서 풀 전후 비교
	if (FParse::Param(FCommandLine::Get(), TEXT("BenchNoPool")))
	{
		if (IConsoleVariable* PoolEnabled = IConsoleManager::Get().FindConsoleVariable(TEXT("ts.Pool.Enabled")))
		{
			PoolEnabled->Set(0, ECVF_SetByConsole);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[벤치] 풀: %s"), USActorPool::IsPoolingEnabled() ? TEXT("켬") : TEXT("끔"));

	// GC 간격을 10초로 고정 (기본 61초면 20초 측정 안에 GC가 안 일어날 수 있음). 풀 전후 같은 조건
	if (IConsoleVariable* GCInterval = IConsoleManager::Get().FindConsoleVariable(TEXT("gc.TimeBetweenPurgingPendingKillObjects")))
	{
		GCInterval->Set(10.f, ECVF_SetByConsole);
	}

	BenchRemaining = FMath::RoundToInt(PerSecond * Seconds);
	BenchFired = 0;
	BenchSpawnSeconds = 0.0;
	BenchMaxSpawnSeconds = 0.0;
	BenchFrameTimes.Reset();
	BenchFrameTimes.Reserve(FMath::CeilToInt(Seconds * 200.f));
	BenchGCCount = 0;
	BenchGCSeconds = 0.0;
	BenchGCMaxSeconds = 0.0;

	// 새로 만들어진 UObject 수 세기 시작
	BenchObjectCounter.Count = 0;
	GUObjectArray.AddUObjectCreateListener(&BenchObjectCounter);

	// 프레임 시간과 GC 시간을 측정하는 동안만 기록
	BenchFrameHandle = FCoreDelegates::OnEndFrame.AddWeakLambda(this, [this]()
	{
		BenchFrameTimes.Add(FApp::GetDeltaTime());
	});
	BenchPreGCHandle = FCoreUObjectDelegates::GetPreGarbageCollectDelegate().AddWeakLambda(this, [this]()
	{
		BenchGCStart = FPlatformTime::Seconds();
	});
	BenchPostGCHandle = FCoreUObjectDelegates::GetPostGarbageCollect().AddWeakLambda(this, [this]()
	{
		const double Elapsed = FPlatformTime::Seconds() - BenchGCStart;
		++BenchGCCount;
		BenchGCSeconds += Elapsed;
		BenchGCMaxSeconds = FMath::Max(BenchGCMaxSeconds, Elapsed);
	});

	UE_LOG(LogTemp, Log, TEXT("[벤치] 시작: 초당 %.0f발 × %.0f초 = %d발"), PerSecond, Seconds, BenchRemaining);
	GetWorldTimerManager().SetTimer(BenchTimer, this, &ThisClass::BenchFireOne, 1.f / PerSecond, true);
}

void ASPlayerController::FinishBench()
{
	GetWorldTimerManager().ClearTimer(BenchTimer);
	FCoreDelegates::OnEndFrame.Remove(BenchFrameHandle);
	FCoreUObjectDelegates::GetPreGarbageCollectDelegate().Remove(BenchPreGCHandle);
	FCoreUObjectDelegates::GetPostGarbageCollect().Remove(BenchPostGCHandle);
	GUObjectArray.RemoveUObjectCreateListener(&BenchObjectCounter);

	// 프레임 시간: 평균, 최대, 상위 1% (느린 쪽 1%의 경계 = 끊김 정도)
	TArray<float> Sorted = BenchFrameTimes;
	Sorted.Sort();
	double FrameSum = 0.0;
	for (const float FrameTime : Sorted)
	{
		FrameSum += FrameTime;
	}
	const int32 FrameCount = Sorted.Num();
	const double FrameAverageMs = FrameCount > 0 ? FrameSum / FrameCount * 1e3 : 0.0;
	const double FrameMaxMs = FrameCount > 0 ? Sorted.Last() * 1e3 : 0.0;
	const double FrameP99Ms = FrameCount > 0 ? Sorted[FMath::Min(FrameCount - 1, FMath::FloorToInt(FrameCount * 0.99f))] * 1e3 : 0.0;

	const double SpawnAverageMicros = BenchFired > 0 ? BenchSpawnSeconds / BenchFired * 1e6 : 0.0;
	UE_LOG(LogTemp, Log, TEXT("[벤치] 끝: %d발, 생성 평균 %.1f us, 최대 %.1f us, 합계 %.2f ms"),
		BenchFired,
		SpawnAverageMicros,
		BenchMaxSpawnSeconds * 1e6,
		BenchSpawnSeconds * 1e3);
	UE_LOG(LogTemp, Log, TEXT("[벤치] 프레임: %d개, 평균 %.2f ms, 상위1%% %.2f ms, 최대 %.2f ms"),
		FrameCount,
		FrameAverageMs,
		FrameP99Ms,
		FrameMaxMs);
	UE_LOG(LogTemp, Log, TEXT("[벤치] GC: %d번, 합계 %.2f ms, 최대 %.2f ms"),
		BenchGCCount,
		BenchGCSeconds * 1e3,
		BenchGCMaxSeconds * 1e3);
	UE_LOG(LogTemp, Log, TEXT("[벤치] 새 UObject: %d개 (1발당 %.1f개)"),
		BenchObjectCounter.Count.load(),
		BenchFired > 0 ? static_cast<double>(BenchObjectCounter.Count.load()) / BenchFired : 0.0);

	// 자동 측정 (실행 인자 -BenchQuit): 결과를 남기고 종료
	if (FParse::Param(FCommandLine::Get(), TEXT("BenchQuit")))
	{
		ConsoleCommand(TEXT("quit"));
	}
}

void ASPlayerController::BenchFireOne()
{
	const APawn* ControlledPawn = GetPawn();
	UAbilitySystemComponent* ASC = GetSAbilitySystemComponent();
	if (BenchRemaining <= 0 || !ControlledPawn || !ASC)
	{
		FinishBench();
		return;
	}
	--BenchRemaining;

	// 실제 화염탄과 같은 내용: 데미지 스펙을 들고 날아감 (맞으면 데미지 숫자도 생김)
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(USGE_Damage::StaticClass(), 1.f, ASC->MakeEffectContext());
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(SGameplayTags::SetByCaller_DamageMultiplier, 1.f);
	}
	const FVector Direction = BenchDirection.RotateAngleAxis(BenchRandom.FRandRange(-20.f, 20.f), FVector::UpVector);
	const FTransform SpawnTransform(Direction.Rotation(), ControlledPawn->GetActorLocation() + FVector(0.f, 0.f, 40.f));

	// 잴 구간: 꺼내기 → 초기화 → 발사. 풀이 꺼져 있으면(-BenchNoPool) 꺼내기가 매번 새로 생성 (같은 코드 경로)
	const double Start = FPlatformTime::Seconds();
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(BenchProjectileSpawn);
		USActorPool* Pool = GetWorld()->GetSubsystem<USActorPool>();
		ASProjectile* Projectile = Pool ? Pool->Acquire<ASProjectile>(
			BenchProjectileClass,
			SpawnTransform,
			GetPawn(),
			GetPawn()) : nullptr;
		if (Projectile)
		{
			Projectile->InitializeProjectile(ASC, { Spec });
			Projectile->Launch();
		}
	}
	const double Elapsed = FPlatformTime::Seconds() - Start;
	BenchSpawnSeconds += Elapsed;
	BenchMaxSpawnSeconds = FMath::Max(BenchMaxSpawnSeconds, Elapsed);
	++BenchFired;
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
