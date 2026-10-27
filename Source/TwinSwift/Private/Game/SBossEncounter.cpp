#include "Game/SBossEncounter.h"

#include "Bullet/SBulletManager.h"
#include "Character/BossCharacter.h"
#include "Character/PlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Camera/CameraActor.h"
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UI/SBossHUDWidget.h"

ASBossEncounter::ASBossEncounter()
{
	// 매 프레임 할 일 없음 (오버랩·사망 알림으로만 움직임)
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>("Trigger");
	SetRootComponent(Trigger);
	Trigger->SetBoxExtent(FVector(1500.f, 1500.f, 300.f));
	// 겹침만 감지하는 엔진 기본 설정 (막지 않음)
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
}

void ASBossEncounter::BeginPlay()
{
	Super::BeginPlay();

	// [측정] 탄막 자동 측정(-BenchBullets=) 중에는 전투를 시작하지 않음 → 보스 탄·AI·등장신이 측정에 섞이지 않게
	FString BulletBenchArgs;
	if (FParse::Value(FCommandLine::Get(), TEXT("BenchBullets="), BulletBenchArgs, false))
	{
		UE_LOG(LogTemp, Display, TEXT("[보스전] %s: 탄막 측정 중이라 전투를 시작하지 않음"), *GetName());
		return;
	}

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnTriggerBeginOverlap);

	// 보스 HUD: 처음부터 만들어 두고 상태에 따라 보이기만 바꿈 (대기 중에는 숨김)
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (BossHUDClass && PlayerController)
	{
		BossHUD = CreateWidget<USBossHUDWidget>(PlayerController, BossHUDClass);
		if (BossHUD)
		{
			BossHUD->AddToViewport();
			BossHUD->BindToEncounter(this);
		}
	}

	// 플레이어가 처음부터 상자 안에 있으면 "들어옴" 알림이 오지 않음 → 다음 프레임에 한 번 확인
	// (이번 프레임에는 플레이어가 아직 생성 중일 수 있음)
	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		TArray<AActor*> Overlapping;
		Trigger->GetOverlappingActors(Overlapping, APlayerCharacter::StaticClass());
		if (!Overlapping.IsEmpty())
		{
			NotifyPlayerEntered();
		}
	}));
}

void ASBossEncounter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CleanupIntro();
	if (State == EBossEncounterState::Intro && IsValid(Boss))
	{
		Boss->SkipIntro(GetIntroLanding());
	}
	if (Boss)
	{
		Boss->OnBossDied.Remove(BossDiedHandle);
	}
	if (APlayerCharacter* PlayerCharacter = Player.Get())
	{
		PlayerCharacter->OnDefeated.Remove(PlayerDefeatedHandle);
	}

	// 진행 액터만 제거돼도 화면과 UI 구독이 남지 않게 정리
	if (BossHUD)
	{
		BossHUD->RemoveFromParent();
		BossHUD = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ASBossEncounter::SetState(EBossEncounterState NewState)
{
	if (State == NewState)
	{
		return;
	}

	State = NewState;
	UE_LOG(LogTemp, Display, TEXT("[보스전] 상태: %s"), *UEnum::GetValueAsString(NewState));
	OnEncounterStateChanged.Broadcast(NewState);
}

void ASBossEncounter::OnTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor->IsA<APlayerCharacter>())
	{
		NotifyPlayerEntered();
	}
}

void ASBossEncounter::NotifyPlayerEntered()
{
	// 상자를 나갔다 다시 들어와도 한 번만 (보스 생성은 게임 모드가)
	if (bPlayerEntered)
	{
		return;
	}
	bPlayerEntered = true;
	OnPlayerEntered.Broadcast();
}

void ASBossEncounter::StartWithBoss(ABossCharacter* NewBoss, bool bInLastBoss)
{
	if (!IsValid(NewBoss))
	{
		UE_LOG(LogTemp, Warning, TEXT("[보스전] %s: 넘겨받은 보스가 없음"), *GetName());
		return;
	}

	// 이전 보스의 사망 알림은 풀고 새 보스에 묶음 (지운 보스의 알림이 남지 않게)
	if (Boss)
	{
		Boss->OnBossDied.Remove(BossDiedHandle);
	}
	Boss = NewBoss;
	bLastBoss = bInLastBoss;
	BossDiedHandle = Boss->OnBossDied.AddUObject(this, &ThisClass::OnBossDied);

	// 대기로 돌린 뒤(HUD가 숨김) 보스를 알리고 등장부터
	SetState(EBossEncounterState::Waiting);
	OnBossChanged.Broadcast(Boss);
	BeginEncounter();
}

FTransform ASBossEncounter::GetBossSpawnTransform() const
{
	// 플레이어 → 가운데 방향으로 BossSpawnDistance 더 간 곳 (플레이어 반대쪽 가장자리). 플레이어가 없으면 이 액터의 뒤쪽
	const FVector Center = GetIntroLanding();
	FVector Away = -GetActorForwardVector();
	if (const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		const FVector ToCenter = (Center - PlayerPawn->GetActorLocation()).GetSafeNormal2D();
		if (!ToCenter.IsNearlyZero())
		{
			Away = ToCenter;
		}
	}

	// 바닥보다 조금 위 (캡슐 크기는 보스마다 달라 생성할 때 겹치면 위로 밀어 올림)
	const FVector Location = Center + Away * BossSpawnDistance + FVector(0.f, 0.f, 200.f);
	return FTransform((-Away).Rotation(), Location);
}

void ASBossEncounter::BeginEncounter()
{
	// 대기 중일 때만 (상자를 나갔다 다시 들어와도 한 번만)
	if (State != EBossEncounterState::Waiting || !Boss)
	{
		return;
	}

	// 패배 알림 구독은 한 번만 (보스가 바뀌어도 플레이어는 같음. 지금은 1명. 2인이면 플레이어 목록을 돌아야 함)
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (PlayerCharacter && !PlayerDefeatedHandle.IsValid())
	{
		Player = PlayerCharacter;
		PlayerDefeatedHandle = PlayerCharacter->OnDefeated.AddUObject(this, &ThisClass::OnPlayerDefeated);
	}

	SetState(EBossEncounterState::Intro);
	StartIntro();
}

void ASBossEncounter::StartIntro()
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();

	if (bPlayIntro && IntroDuration <= 0.f)
	{
		Boss->SkipIntro(GetIntroLanding());
		StartFight();
		return;
	}

	// 등장신을 끄거나 보스에게 등장 능력이 없으면 바로 전투 (실패 이유는 보스가 로그로)
	if (!bPlayIntro || !PlayerController || !Boss->PlayIntro(GetIntroLanding()))
	{
		StartFight();
		return;
	}

	IntroController = PlayerController;
	PreviousViewTarget = PlayerController->GetViewTarget();
	IntroStartTime = GetWorld()->GetTimeSeconds();
	SetPlayerInputEnabled(false);
	if (IntroCamera)
	{
		bIntroCameraChanged = true;
		PlayerController->SetViewTargetWithBlend(IntroCamera, IntroCameraBlendTime, VTBlend_Cubic);
	}

	// 건너뛰기: Space·Enter (아무 키로 하면 마우스 움직임까지 걸릴 수 있음)
	EnableInput(PlayerController);
	InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ThisClass::SkipIntro);
	InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ThisClass::SkipIntro);

	GetWorldTimerManager().SetTimer(IntroTimer, this, &ThisClass::CheckIntroCompletion, 0.05f, true);
}

void ASBossEncounter::SkipIntro()
{
	if (State != EBossEncounterState::Intro)
	{
		return;
	}

	if (IsValid(Boss))
	{
		Boss->SkipIntro(GetIntroLanding());
	}
	EndIntro();
}

void ASBossEncounter::EndIntro()
{
	// 시간이 다 된 것과 건너뛰기가 같은 프레임에 와도 한 번만
	if (State != EBossEncounterState::Intro)
	{
		return;
	}
	CleanupIntro();
	StartFight();
}

void ASBossEncounter::CheckIntroCompletion()
{
	if (State != EBossEncounterState::Intro)
	{
		return;
	}
	const double Elapsed = GetWorld()->GetTimeSeconds() - IntroStartTime;
	if (!IsValid(Boss) || Elapsed >= FMath::Max(IntroDuration, IntroTimeout))
	{
		UE_LOG(LogTemp, Warning, TEXT("[보스전] 등장 제한 시간 또는 보스 소멸: 연출 정리"));
		SkipIntro();
	}
	else if (Elapsed >= IntroDuration && !Boss->IsIntroPlaying())
	{
		EndIntro();
	}
}

void ASBossEncounter::CleanupIntro()
{
	GetWorldTimerManager().ClearTimer(IntroTimer);
	ClearEncounterKeys();
	SetPlayerInputEnabled(true);
	if (APlayerController* Controller = IntroController.Get())
	{
		AActor* Target = PreviousViewTarget.Get();
		if (!Target)
		{
			Target = Controller->GetPawn();
		}
		if (bIntroCameraChanged && IsValid(Target))
		{
			Controller->SetViewTargetWithBlend(Target, IntroCameraBlendTime, VTBlend_Cubic);
		}
	}
	bIntroCameraChanged = false;
	PreviousViewTarget.Reset();
	IntroController.Reset();
}

FVector ASBossEncounter::GetIntroLanding() const
{
	// 경기장 가운데의 바닥 (보스가 기둥 위 등 높은 곳에서 시작해도 바닥에 착지)
	// 위에서 아래로 고정 물체만 찾음. 못 찾으면 이 액터 높이
	const FVector Center = GetActorLocation();
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(EncounterLanding), false, this);
	if (GetWorld()->LineTraceSingleByObjectType(
		Hit,
		Center + FVector(0.f, 0.f, 1000.f),
		Center - FVector(0.f, 0.f, 5000.f),
		FCollisionObjectQueryParams(ECC_WorldStatic),
		Params))
	{
		return Hit.ImpactPoint;
	}
	return Center;
}

void ASBossEncounter::SetPlayerInputEnabled(bool bEnabled)
{
	APlayerController* PlayerController = IntroController.Get();
	if (!PlayerController)
	{
		return;
	}

	// 컨트롤러의 입력(이동·스킬·교체)만 끔. 다른 액터가 EnableInput으로 넣은 입력(건너뛰기 키)은 계속 받음
	if (bEnabled && bIntroInputLocked)
	{
		bIntroInputLocked = false;
		PlayerController->EnableInput(PlayerController);
	}
	else if (!bEnabled && !bIntroInputLocked)
	{
		bIntroInputLocked = true;
		PlayerController->DisableInput(PlayerController);
	}
}

void ASBossEncounter::ClearEncounterKeys()
{
	if (InputComponent)
	{
		InputComponent->KeyBindings.Empty();
	}
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		DisableInput(PlayerController);
	}
}

void ASBossEncounter::StartFight()
{
	// 보스가 실제로 움직이기 시작했을 때만 전투 (실패 이유는 보스가 로그로 남김)
	if (!IsValid(Boss) || !Boss->StartCombat())
	{
		// 실패 후 재진입으로 다시 시도할 수 있도록 대기로 복구
		if (APlayerCharacter* PlayerCharacter = Player.Get())
		{
			PlayerCharacter->OnDefeated.Remove(PlayerDefeatedHandle);
		}
		PlayerDefeatedHandle.Reset();
		Player.Reset();
		SetState(EBossEncounterState::Waiting);
		UE_LOG(LogTemp, Warning, TEXT("[보스전] 시작 실패: 대기로 복귀. 설정 확인 후 경기장에 다시 진입하세요."));
		return;
	}

	// 시간을 먼저 정하고 상태를 바꿈 (상태 알림을 받은 쪽이 전투 시간을 읽어도 맞게)
	FightStartTime = GetWorld()->GetTimeSeconds();
	FightEndTime = -1.0;
	SetState(EBossEncounterState::Fighting);
}

float ASBossEncounter::GetFightSeconds() const
{
	if (State == EBossEncounterState::Waiting || State == EBossEncounterState::Intro)
	{
		return 0.f;
	}

	const double End = FightEndTime >= 0.0 ? FightEndTime : GetWorld()->GetTimeSeconds();
	return static_cast<float>(End - FightStartTime);
}

void ASBossEncounter::OnBossDied(ABossCharacter* DeadBoss)
{
	// 전투 중일 때만 (이미 패배했으면 무시 → 같은 순간 둘 다 일어나도 결과는 하나)
	if (State != EBossEncounterState::Fighting)
	{
		return;
	}

	FinishFight(EBossEncounterState::Victory);
}

void ASBossEncounter::OnPlayerDefeated()
{
	if (State != EBossEncounterState::Fighting)
	{
		return;
	}

	// 결과 화면 뒤에서 보스가 계속 싸우지 않게
	Boss->StopCombat();

	FinishFight(EBossEncounterState::Defeat);
}

void ASBossEncounter::FinishFight(EBossEncounterState Result)
{
	// 시간을 멈춘 뒤 상태를 바꿈 → HUD가 결과 알림에서 읽는 클리어 시간이 로그와 같음
	FightEndTime = GetWorld()->GetTimeSeconds();
	UE_LOG(LogTemp, Display, TEXT("[보스전] %s %.1f초"),
		Result == EBossEncounterState::Victory ? TEXT("클리어") : TEXT("패배"),
		GetFightSeconds());

	// 결과 화면 위로 탄이 계속 날아다니지 않게
	if (ASBulletManager* BulletManager = ASBulletManager::Get(this))
	{
		BulletManager->ClearAllBullets();
	}

	// 중간 보스를 이김: 결과 화면 없이 다음 보스를 기다림 (게임 모드가 사망 연출 뒤 다음 보스를 넘김)
	if (Result == EBossEncounterState::Victory && !bLastBoss)
	{
		SetState(EBossEncounterState::Cleared);
		return;
	}

	SetState(Result);
	BindRestartKey();
}

void ASBossEncounter::BindRestartKey()
{
	// 이 액터가 입력을 받게 하고 R 키만 연결 (결과가 나온 뒤에만)
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController)
	{
		return;
	}
	// 등장신 건너뛰기 키가 남아 있지 않게 비우고 R만
	ClearEncounterKeys();
	EnableInput(PlayerController);
	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ThisClass::RestartLevel);
}

void ASBossEncounter::RestartLevel()
{
	// 보스·플레이어·표식·쿨다운을 하나씩 되돌리지 않고 레벨을 새로 불러옴 → 빠뜨리는 것 없이 처음 상태
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
}