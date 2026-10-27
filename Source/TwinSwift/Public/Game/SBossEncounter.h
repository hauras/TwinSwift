
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SBossEncounter.generated.h"

class ACameraActor;
class APlayerController;
class APlayerCharacter;
class ABossCharacter;
class UBoxComponent;
class USBossHUDWidget;

UENUM(BlueprintType)
enum class EBossEncounterState : uint8
{
	Waiting,
	Intro,
	Fighting,
	Victory,
	Defeat,
	// 중간 보스 처치: 결과 화면 없이 다음 보스를 기다림 (게임 모드가 다음 보스를 넘김)
	Cleared
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnEncounterStateChanged, EBossEncounterState);
DECLARE_MULTICAST_DELEGATE(FOnEncounterPlayerEntered);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEncounterBossChanged, ABossCharacter*);

UCLASS()
class TWINSWIFT_API ASBossEncounter : public AActor
{
	GENERATED_BODY()
	
public:	
	ASBossEncounter();

	EBossEncounterState GetState() const { return State; }

	ABossCharacter* GetBoss() const { return Boss;}

	// 전투 시간(초): 전투 중이면 지금까지, 결과가 나왔으면 결과 순간에 멈춘 값 (결과 글자에 클리어 시간)
	float GetFightSeconds() const;

	// 게임 모드가 생성한 보스로 한 판 시작 (등장부터). bLastBoss면 이겼을 때 결과 화면, 아니면 Cleared로 다음 보스를 기다림
	void StartWithBoss(ABossCharacter* NewBoss, bool bInLastBoss);

	// 보스를 생성할 자리: 경기장 가운데에서 플레이어 반대쪽 가장자리, 플레이어를 바라봄 (골렘은 여기서 가운데로 뛰어듦)
	FTransform GetBossSpawnTransform() const;

	FOnEncounterStateChanged OnEncounterStateChanged;

	// 플레이어가 처음 경기장에 들어옴 (한 번만) → 게임 모드가 첫 보스를 생성
	FOnEncounterPlayerEntered OnPlayerEntered;

	// 보스가 바뀜 → HUD가 이름·체력바를 새 보스로 다시 묶음
	FOnEncounterBossChanged OnBossChanged;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 경기장 진입 판정 (플레이어가 들어오면 시작)
	UPROPERTY(VisibleAnywhere, Category = "Encounter")
	TObjectPtr<UBoxComponent> Trigger;

	// 지금 싸우는 보스 (게임 모드가 생성해서 StartWithBoss로 넘김. 레벨에 보스를 놓지 않음)
	UPROPERTY(Transient)
	TObjectPtr<ABossCharacter> Boss;

	// 가운데에서 보스 생성 자리까지 거리(cm). 진입 상자 반경(1500) 안쪽
	UPROPERTY(EditAnywhere, Category = "Encounter", meta = (ClampMin = "0"))
	float BossSpawnDistance = 1200.f;

	// 보스 HUD (WBP_BossHUD). 비우면 HUD 없이 진행
	UPROPERTY(EditDefaultsOnly, Category = "Encounter|UI")
	TSubclassOf<USBossHUDWidget> BossHUDClass;

	// 등장신을 할지 (끄면 들어오자마자 전투)
	UPROPERTY(EditAnywhere, Category = "Encounter|Intro")
	bool bPlayIntro = true;

	// 등장신 동안 비출 카메라 (레벨에 놓은 카메라). 비우면 카메라는 그대로 두고 보스만 등장
	UPROPERTY(EditInstanceOnly, Category = "Encounter|Intro")
	TObjectPtr<ACameraActor> IntroCamera;

	// 최소 등장 시간(초). 능력이 더 길면 종료를 기다림. 0이면 건너뜀
	UPROPERTY(EditAnywhere, Category = "Encounter|Intro", meta = (ClampMin = "0"))
	float IntroDuration = 4.f;

	// 능력이 끝나지 않을 때의 안전 제한. 최소 등장 시간보다 짧게 적용하지 않음
	UPROPERTY(EditAnywhere, Category = "Encounter|Intro", meta = (ClampMin = "0.1"))
	float IntroTimeout = 12.f;

	// 카메라가 넘어가는 시간(초, 들어갈 때와 돌아올 때)
	UPROPERTY(EditAnywhere, Category = "Encounter|Intro", meta = (ClampMin = "0"))
	float IntroCameraBlendTime = 1.f;

private:
	// 등장 능력과 최소 연출 시간이 모두 끝나면 전투
	void StartIntro();

	// 등장 끝 (시간이 다 되거나 건너뛰기): 입력·카메라 되돌리고 전투 시작
	void EndIntro();

	// Space·Enter → 보스를 착지 지점에 바로 세우고 EndIntro
	void SkipIntro();

	// 보스가 착지할 곳 = 경기장 가운데 (이 액터 위치, 높이는 보스 발 높이)
	FVector GetIntroLanding() const;

	// 등장신 동안 플레이어 조작 잠금 (이 액터의 건너뛰기 키는 따로라 계속 받음)
	void SetPlayerInputEnabled(bool bEnabled);

	// 이 액터가 받는 키를 모두 해제 (건너뛰기 → 다시 시작 키로 바꿀 때)
	void ClearEncounterKeys();

	FTimerHandle IntroTimer;
	void CheckIntroCompletion();
	void CleanupIntro();
	double IntroStartTime = 0.0;
	bool bIntroInputLocked = false;
	bool bIntroCameraChanged = false;
	TWeakObjectPtr<APlayerController> IntroController;
	TWeakObjectPtr<AActor> PreviousViewTarget;

	void SetState(EBossEncounterState NewState);

	UFUNCTION()
	void OnTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void BeginEncounter();

	// 플레이어가 들어옴 (겹침 또는 처음부터 상자 안) → OnPlayerEntered 한 번
	void NotifyPlayerEntered();

	void StartFight();

	void OnBossDied(ABossCharacter* DeadBoss);
	void OnPlayerDefeated();

	// 승리·패배 뒤 R 키 → 같은 레벨 다시 불러오기
	void BindRestartKey();
	void RestartLevel();

	EBossEncounterState State = EBossEncounterState::Waiting;

	// 지금 보스가 마지막인지 (StartWithBoss가 정함) / 플레이어가 이미 들어왔는지
	bool bLastBoss = true;
	bool bPlayerEntered = false;

	// 전투 시작·결과 시각 (클리어 시간 계산). 결과 전에는 끝 시각이 음수
	double FightStartTime = 0.0;
	double FightEndTime = -1.0;

	// 결과 순간 시간을 멈추고 로그 (승리·패배 공통)
	void FinishFight(EBossEncounterState Result);

	UPROPERTY()
	TObjectPtr<USBossHUDWidget> BossHUD;

	FDelegateHandle BossDiedHandle;
	FDelegateHandle PlayerDefeatedHandle;

	// 패배 알림을 구독한 플레이어 (해제할 때 씀)
	TWeakObjectPtr<APlayerCharacter> Player;
};
