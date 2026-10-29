#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Mirror/SActionRecorderComponent.h"
#include "SActionReplayComponent.generated.h"

/**
 * 거울 보스: 플레이어가 실제로 쓴 기술을 Delay초 늦게 따라 함 ("내 기술을 배워서 맞서는 거울 기사")
 * 행동(능력·교체)만 기록을 따라 함. 위치는 따라 하지 않음 (과거 위치·관계는 지금 전투와 안 맞아 엉뚱한 데로 감)
 * 이동: 지금 몸에 맞는 거리를 잡음 (전사는 붙고, 마법사는 거리 유지). 능력이 켜져 있으면 능력이 움직임을 맡음
 * 회피는 복사하지 않음: 그때의 위협에 대한 반응이라 지금 따라 하면 이유 없는 움직임이 됨 → 방어 컴포넌트가 스스로 판단
 * 채우기: 따라 할 행동이 IdleFillSeconds 동안 없으면(플레이어가 가만히 있으면) 기록에 남은 내 기술 하나를 바로 씀 → 가만히 서서 버티기를 막음
 */
UCLASS(ClassGroup = (Mirror), meta = (BlueprintSpawnableComponent))
class TWINSWIFT_API USActionReplayComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USActionReplayComponent();

	// 따라 하기 시작 (전투 시작·큰 경직 회복). 멈춘 사이의 행동은 건너뜀
	void StartReplay();

	// 멈춤 (전투 중지·큰 경직·사망)
	void StopReplay();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// 몇 초 늦게 따라 할지 (기록 컴포넌트의 HistorySeconds보다 작아야 함)
	UPROPERTY(EditDefaultsOnly, Category = "Mirror", meta = (ClampMin = "0.1"))
	float Delay = 3.f;

	// 전사 몸: 이 거리까지 다가감 (평타가 닿는 거리)
	UPROPERTY(EditDefaultsOnly, Category = "Mirror|Move", meta = (ClampMin = "0"))
	float WarriorRange = 150.f;

	// 마법사 몸: 이보다 가까우면 점멸로 물러남 (점멸 쿨다운이면 걸어서)
	UPROPERTY(EditDefaultsOnly, Category = "Mirror|Move", meta = (ClampMin = "0"))
	float MageMinRange = 500.f;

	// 마법사 몸: 이보다 멀면 다가감
	UPROPERTY(EditDefaultsOnly, Category = "Mirror|Move", meta = (ClampMin = "0"))
	float MageMaxRange = 900.f;

	// 이만큼(초) 보스가 행동을 안 했으면 최근 기술 하나를 바로 씀
	UPROPERTY(EditDefaultsOnly, Category = "Mirror|Fill", meta = (ClampMin = "0.5"))
	float IdleFillSeconds = 2.5f;

private:
	// 지금 몸에 맞는 거리를 잡음 (능력 사용·경직·사망 중에는 안 움직임). 거리 안이면 서서 플레이어를 봄
	void UpdateMovement(float DeltaTime);

	// From 미포함 ~ To 포함 사이 행동을 보스에게
	void ReplayActions(double FromTime, double ToTime);

	// 가만히 있으면 채우기: 한가할 때 기록에 남은 기술 중 지금 쓸 수 있는 것 하나를 무작위로
	void TryFillIdle(double Now);

	// 플레이어의 기록 컴포넌트 (가리키기만 함. 없어졌으면 다시 찾음)
	USActionRecorderComponent* FindRecorder();

	TWeakObjectPtr<USActionRecorderComponent> Recorder;

	// 커서: 여기까지 따라 했음 (기록 시각 기준)
	double LastReplayTime = 0.0;

	// 처음 시작한 시각. 그 전 기록(보스가 나오기 전 플레이어 행동)은 따라 하지 않음
	double FirstStartTime = -1.0;

	bool bReplaying = false;

	// 보스가 마지막으로 행동한 시각 (따라 하기·채우기). 채우기 기준
	double LastActionTime = 0.0;

	// 채우기 후보 그릇 (재사용)
	TArray<FSRecordedAction> FillCandidates;

	// 꺼낸 행동을 담는 그릇 (매 틱 새로 만들지 않게 재사용)
	TArray<FSRecordedAction> PendingActions;
};
