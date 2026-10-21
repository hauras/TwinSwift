#include "Game/SBossEncounter.h"

#include "Character/BossCharacter.h"
#include "Character/PlayerCharacter.h"
#include "Components/BoxComponent.h"
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

	if (!Boss)
	{
		UE_LOG(LogTemp, Warning, TEXT("[보스전] %s: Boss가 비어 있음 (레벨에서 지정)"), *GetName());
		return;
	}

	BossDiedHandle = Boss->OnBossDied.AddUObject(this, &ThisClass::OnBossDied);
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
			BeginEncounter();
		}
	}));
}

void ASBossEncounter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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
		BeginEncounter();
	}
}

void ASBossEncounter::BeginEncounter()
{
	// 대기 중일 때만 (상자를 나갔다 다시 들어와도 한 번만)
	if (State != EBossEncounterState::Waiting || !Boss)
	{
		return;
	}

	// 패배 알림 구독 (지금은 플레이어 1명. 2인이면 플레이어 목록을 돌아야 함)
	if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		Player = PlayerCharacter;
		PlayerDefeatedHandle = PlayerCharacter->OnDefeated.AddUObject(this, &ThisClass::OnPlayerDefeated);
	}

	// 등장신은 10-22에 이 상태에 넣음. 지금은 바로 전투
	SetState(EBossEncounterState::Intro);
	StartFight();
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
	EnableInput(PlayerController);
	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ThisClass::RestartLevel);
}

void ASBossEncounter::RestartLevel()
{
	// 보스·플레이어·표식·쿨다운을 하나씩 되돌리지 않고 레벨을 새로 불러옴 → 빠뜨리는 것 없이 처음 상태
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
}