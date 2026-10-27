#include "GameMode/SGameMode.h"

#include "Character/BossCharacter.h"
#include "Controller/SPlayerController.h"
#include "EngineUtils.h"
#include "State/SPlayerState.h"
#include "TimerManager.h"

ASGameMode::ASGameMode()
{
	PlayerControllerClass = ASPlayerController::StaticClass();

	// ASC·AttributeSet을 가진 PlayerState
	PlayerStateClass = ASPlayerState::StaticClass();
}

void ASGameMode::StartPlay()
{
	Super::StartPlay();

	// 경기장은 하나 (보스가 차례로 같은 경기장에 등장). 없으면 런 진행 없이 그냥 플레이
	TActorIterator<ASBossEncounter> It(GetWorld());
	if (!It) return;
	Encounter = *It;

	PlayerEnteredHandle = It->OnPlayerEntered.AddUObject(this, &ThisClass::OnPlayerEnteredArena);
	StateChangedHandle = It->OnEncounterStateChanged.AddUObject(this, &ThisClass::OnEncounterStateChanged);

	if (BossCharacterClasses.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[런] 보스 순서가 비어 있음 (BP_GameMode의 BossCharacterClasses에 입력)"));
	}
}

void ASGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(NextBossTimer);

	if (ASBossEncounter* CurrentEncounter = Encounter.Get())
	{
		CurrentEncounter->OnPlayerEntered.Remove(PlayerEnteredHandle);
		CurrentEncounter->OnEncounterStateChanged.Remove(StateChangedHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void ASGameMode::OnPlayerEnteredArena()
{
	// 목록이 비면 마지막 칸 번호가 -1 → Clamp 범위가 뒤집힘
	if (BossCharacterClasses.IsEmpty()) return;

	CurrentIndex = FMath::Clamp(StartIndex, 0, BossCharacterClasses.Num() - 1);
	SpawnBoss(CurrentIndex);
}

void ASGameMode::OnEncounterStateChanged(EBossEncounterState NewState)
{
	// 중간 처치만 다음 보스로 (마지막 보스의 승리·패배는 전투 진행 액터가 결과 화면을 띄움)
	if (NewState != EBossEncounterState::Cleared) return;

	// 사망 연출을 보여 준 뒤. 0이면 타이머가 안 켜져서 아주 작은 값 아래로 내려가지 않게
	GetWorldTimerManager().SetTimer(
		NextBossTimer,
		this,
		&ThisClass::AdvanceToNextBoss,
		FMath::Max(NextBossDelay, 0.01f),
		false);
}

void ASGameMode::SpawnBoss(int32 Index)
{
	// 칸 번호가 맞아도 에디터에서 그 칸이 비어(None) 있을 수 있음
	ASBossEncounter* CurrentEncounter = Encounter.Get();
	if (!CurrentEncounter || !BossCharacterClasses.IsValidIndex(Index) || !BossCharacterClasses[Index])
	{
		UE_LOG(LogTemp, Warning, TEXT("[런] %d번째 보스를 생성할 수 없음 (전투 진행 액터 %d, 보스 수 %d)"), Index, CurrentEncounter != nullptr, BossCharacterClasses.Num());
		return;
	}

	// 생성 자리에서 바닥·벽과 겹치면 가까운 빈자리로 옮겨서라도 생성 (보스마다 캡슐 크기가 다름)
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ABossCharacter* Boss = GetWorld()->SpawnActor<ABossCharacter>(
		BossCharacterClasses[Index],
		CurrentEncounter->GetBossSpawnTransform(),
		SpawnParameters);
	if (!Boss) return;

	// 마지막 칸이면 이겼을 때 결과 화면, 아니면 Cleared로 다음 보스를 기다림
	CurrentBoss = Boss;
	const bool bLastBoss = Index == BossCharacterClasses.Num() - 1;
	UE_LOG(LogTemp, Display, TEXT("[런] %d / %d번째 보스: %s"), Index + 1, BossCharacterClasses.Num(), *Boss->GetName());
	CurrentEncounter->StartWithBoss(Boss, bLastBoss);
}

void ASGameMode::AdvanceToNextBoss()
{
	// 쓰러진 보스를 지움 (한 번에 보스 하나). 마지막 다음 칸이면 SpawnBoss의 칸 확인이 막음
	if (ABossCharacter* DeadBoss = CurrentBoss.Get())
	{
		DeadBoss->Destroy();
	}
	CurrentBoss.Reset();
	CurrentIndex++;
	SpawnBoss(CurrentIndex);
}
