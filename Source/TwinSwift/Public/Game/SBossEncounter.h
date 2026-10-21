
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SBossEncounter.generated.h"

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
	Defeat
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnEncounterStateChanged, EBossEncounterState);

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

	FOnEncounterStateChanged OnEncounterStateChanged;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

		// 경기장 진입 판정 (플레이어가 들어오면 시작)
    	UPROPERTY(VisibleAnywhere, Category = "Encounter")
    	TObjectPtr<UBoxComponent> Trigger;

	// 이 경기장의 보스 (레벨에 놓은 보스를 지정)
	UPROPERTY(EditInstanceOnly, Category = "Encounter")
	TObjectPtr<ABossCharacter> Boss;

	// 보스 HUD (WBP_BossHUD). 비우면 HUD 없이 진행
	UPROPERTY(EditDefaultsOnly, Category = "Encounter|UI")
	TSubclassOf<USBossHUDWidget> BossHUDClass;

private:
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

	void StartFight();

	void OnBossDied(ABossCharacter* DeadBoss);
	void OnPlayerDefeated();

	// 승리·패배 뒤 R 키 → 같은 레벨 다시 불러오기
	void BindRestartKey();
	void RestartLevel();

	EBossEncounterState State = EBossEncounterState::Waiting;

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
