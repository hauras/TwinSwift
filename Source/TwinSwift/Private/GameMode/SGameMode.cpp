#include "GameMode/SGameMode.h"

#include "AbilitySystemComponent.h"
#include "EngineUtils.h"
#include "SGameplayTags.h"
#include "TimerManager.h"
#include "Ability/SAbilitySystemComponent.h"
#include "Ability/SAttributeSet.h"
#include "Blueprint/UserWidget.h"
#include "Character/BossCharacter.h"
#include "Controller/SPlayerController.h"
#include "State/SPlayerState.h"
#include "UI/SUpgradeSelectWidget.h"
#include "Upgrade/SUpgradeData.h"

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

	// 사망 연출을 보여 준 뒤 강화 카드. 0이면 타이머가 안 켜져서 아주 작은 값 아래로 내려가지 않게
	GetWorldTimerManager().SetTimer(
		NextBossTimer,
		this,
		&ThisClass::ShowUpgradeSelect,
		FMath::Max(NextBossDelay, 0.01f),
		false);
}

void ASGameMode::ShowUpgradeSelect()
{
	// 강화 없이도 런이 끊기지 않게: 후보·화면이 없으면 바로 다음 보스
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (UpgradePool.IsEmpty() || !UpgradeSelectClass || !PlayerController)
	{
		AdvanceToNextBoss();
		return;
	}

	// 섞기 (피셔-예이츠: 뒤에서부터 앞쪽 아무 칸과 바꿈 → 모든 순서가 같은 확률) → 앞에서 ChoiceCount장
	TArray<USUpgradeData*> Pool(UpgradePool);
	for (int32 Index = Pool.Num() - 1; Index > 0; --Index)
	{
		Pool.Swap(Index, FMath::RandRange(0, Index));
	}
	Pool.SetNum(FMath::Min(ChoiceCount, Pool.Num()));

	UpgradeSelect = CreateWidget<USUpgradeSelectWidget>(PlayerController, UpgradeSelectClass);
	if (!UpgradeSelect)
	{
		AdvanceToNextBoss();
		return;
	}
	UpgradeSelect->AddToViewport();
	UpgradeSelect->OnUpgradeChosen.AddUObject(this, &ThisClass::OnUpgradeChosen);
	UpgradeSelect->ShowChoices(Pool);

	// 게임 시간 정지 (UI는 계속 돔) + 입력은 카드 화면으로 (마우스 커서는 원래 보임)
	SetPause(PlayerController);
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(UpgradeSelect->TakeWidget());
	PlayerController->SetInputMode(InputMode);
}

void ASGameMode::OnUpgradeChosen(USUpgradeData* Upgrade)
{
	ApplyUpgrade(Upgrade);
	HealPlayers();

	if (UpgradeSelect)
	{
		UpgradeSelect->RemoveFromParent();
		UpgradeSelect = nullptr;
	}

	// 재개 + 입력을 게임으로 (시점마다 입력 모드가 달라서 컨트롤러에 맡김)
	ClearPause();
	if (ASPlayerController* PlayerController = Cast<ASPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		PlayerController->ApplyGameInputMode();
	}

	AdvanceToNextBoss();
}

void ASGameMode::ApplyUpgrade(const USUpgradeData* Upgrade)
{
	const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	const ASPlayerState* PlayerState = PlayerController ? PlayerController->GetPlayerState<ASPlayerState>() : nullptr;
	if (!Upgrade || !Upgrade->Effect || !PlayerState) return;

	// 몸 하나에 ASC가 둘 (전사·마법사) → 대상에 맞는 쪽에만
	TArray<UAbilitySystemComponent*, TInlineAllocator<2>> Targets;
	if (Upgrade->Target != EUpgradeTarget::Mage)
	{
		Targets.Add(PlayerState->GetSAbilitySystemComponent(ETwinSlot::Warrior));
	}
	if (Upgrade->Target != EUpgradeTarget::Warrior)
	{
		Targets.Add(PlayerState->GetSAbilitySystemComponent(ETwinSlot::Mage));
	}

	for (UAbilitySystemComponent* ASC : Targets)
	{
		if (!ASC)
		{
			continue;
		}
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Upgrade->Effect, 1.f, Context);
		if (Spec.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
	UE_LOG(LogTemp, Display, TEXT("[런] 강화: %s (%d명)"), *Upgrade->DisplayName.ToString(), Targets.Num());
}

void ASGameMode::HealPlayers()
{
	const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	const ASPlayerState* PlayerState = PlayerController ? PlayerController->GetPlayerState<ASPlayerState>() : nullptr;
	if (!PlayerState) return;

	for (const ETwinSlot Slot : { ETwinSlot::Warrior, ETwinSlot::Mage })
	{
		// 쓰러진 캐릭터는 건너뜀 (부활은 쓰러짐 태그·대기 회복·외형까지 엮여 있어 따로)
		UAbilitySystemComponent* ASC = PlayerState->GetSAbilitySystemComponent(Slot);
		if (!ASC || ASC->HasMatchingGameplayTag(SGameplayTags::State_Dead))
		{
			continue;
		}

		// 피해 처리(SAttributeSet)와 같이 그 ASC의 base 값에 직접 (최대 체력 강화를 고른 직후면 늘어난 최대치 기준)
		const float Health = ASC->GetNumericAttribute(USAttributeSet::GetHealthAttribute());
		const float MaxHealth = ASC->GetNumericAttribute(USAttributeSet::GetMaxHealthAttribute());
		ASC->SetNumericAttributeBase(USAttributeSet::GetHealthAttribute(), Health + (MaxHealth - Health) * HealMissingRatio);
	}
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
