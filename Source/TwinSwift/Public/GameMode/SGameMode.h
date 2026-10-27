#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Game/SBossEncounter.h"
#include "SGameMode.generated.h"

class ABossCharacter;
class USUpgradeData;
class USUpgradeSelectWidget;

/**
 * 기본 게임 모드. 폰은 BP_GameMode에서 BP_Player로 지정
 * 런 진행 (보스 러시): 보스 순서를 클래스 목록으로 들고, 차례마다 생성해 경기장의 전투 진행 액터에 넘김
 * 레벨에는 보스를 놓지 않음 → 한 번에 보스 하나. 게임 규칙이라 게임 모드에 둠 (멀티에서는 서버에만 있어 보스 생성도 서버만 함)
 * 보스 사이: 사망 연출 → 강화 카드(후보 중 무작위 ChoiceCount장, 게임 정지) → 고르면 대상 ASC에 GE + 살아 있는 캐릭터 회복 → 다음 보스
 */
UCLASS()
class TWINSWIFT_API ASGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASGameMode();

protected:
	// 레벨의 전투 진행 액터를 찾아 알림을 구독
	virtual void StartPlay() override;

	// 구독 해제, 다음 보스 타이머 정리
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 보스 순서 (BP_Boss_Golem → BP_Boss_Conductor ...)
	UPROPERTY(EditDefaultsOnly, Category = "Run")
	TArray<TSubclassOf<ABossCharacter>> BossCharacterClasses;

	// 몇 번째 보스부터 (0부터. 보스 하나만 시험할 때: 1이면 두 번째 보스부터)
	UPROPERTY(EditDefaultsOnly, Category = "Run", meta = (ClampMin = "0"))
	int32 StartIndex = 0;

	// 보스를 이긴 뒤 강화 카드까지(초): 사망 연출을 보여 주는 시간
	UPROPERTY(EditDefaultsOnly, Category = "Run", meta = (ClampMin = "0"))
	float NextBossDelay = 3.f;

	// 강화 후보 (DA_Upgrade_*). 비우면 카드 없이 바로 다음 보스
	UPROPERTY(EditDefaultsOnly, Category = "Run|Upgrade")
	TArray<TObjectPtr<USUpgradeData>> UpgradePool;

	// 카드 화면 (WBP_UpgradeSelect)
	UPROPERTY(EditDefaultsOnly, Category = "Run|Upgrade")
	TSubclassOf<USUpgradeSelectWidget> UpgradeSelectClass;

	// 보여 줄 카드 수 (후보가 적으면 후보 수만큼)
	UPROPERTY(EditDefaultsOnly, Category = "Run|Upgrade", meta = (ClampMin = "1"))
	int32 ChoiceCount = 3;

	// 고른 뒤 잃은 체력 중 회복할 비율 (살아 있는 캐릭터만. 쓰러진 캐릭터 부활은 아직 없음)
	UPROPERTY(EditDefaultsOnly, Category = "Run|Upgrade", meta = (ClampMin = "0", ClampMax = "1"))
	float HealMissingRatio = 0.5f;

private:
	// 플레이어가 경기장에 처음 들어옴 → 첫 보스
	void OnPlayerEnteredArena();

	// 중간 보스 처치(Cleared) → NextBossDelay 뒤 강화 카드
	void OnEncounterStateChanged(EBossEncounterState NewState);

	// 후보를 섞어 앞에서 ChoiceCount장 → 카드 화면, 게임 정지, UI 입력
	void ShowUpgradeSelect();

	// 카드 화면이 알림: 적용 + 회복 → 화면 닫고 재개 → 다음 보스
	void OnUpgradeChosen(USUpgradeData* Upgrade);

	// 대상(전사·마법사·둘 다) ASC에 강화 GE
	void ApplyUpgrade(const USUpgradeData* Upgrade);

	// 살아 있는 캐릭터마다 잃은 체력 × HealMissingRatio 회복
	void HealPlayers();

	// Index번째 보스를 생성해 전투 진행 액터에 넘김
	void SpawnBoss(int32 Index);

	// 쓰러진 보스를 지우고 다음 보스 (강화를 고른 뒤, 후보가 없으면 타이머가 바로)
	void AdvanceToNextBoss();

	// 찾은 전투 진행 액터 / 지금 보스 (가리키기만 함. 지워지면 자동으로 무효)
	TWeakObjectPtr<ASBossEncounter> Encounter;
	TWeakObjectPtr<ABossCharacter> CurrentBoss;

	// 지금 몇 번째 보스인지 (BossCharacterClasses 칸)
	int32 CurrentIndex = 0;

	// 구독 해제용
	FDelegateHandle PlayerEnteredHandle;
	FDelegateHandle StateChangedHandle;

	FTimerHandle NextBossTimer;

	// 띄운 카드 화면 (닫을 때. 들고 있는 동안 GC가 지우지 않게 UPROPERTY)
	UPROPERTY()
	TObjectPtr<USUpgradeSelectWidget> UpgradeSelect;
};
