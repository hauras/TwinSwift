#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Mirror/SActionRecorderComponent.h"
#include "SActionReplayComponent.generated.h"


// 거울 등장: 기록이 없을 때 대신 쓸 기술 한 칸 (BP에서 채움)
USTRUCT(BlueprintType)
struct FSMirrorIntroStep
{
	GENERATED_BODY()

	// 쓸 기술
	UPROPERTY(EditAnywhere, Category = "Intro")
	TSubclassOf<UGameplayAbility> AbilityClass;

	// 그 기술을 쓸 몸
	UPROPERTY(EditAnywhere, Category = "Intro")
	ETwinSlot Slot = ETwinSlot::Warrior;

	// 앞 기술을 누르고 몇 초 뒤에 누를지 (첫 칸은 무시)
	UPROPERTY(EditAnywhere, Category = "Intro", meta = (ClampMin = "0"))
	float Gap = 0.6f;
};

/**
 * 거울 보스: 플레이어가 실제로 쓴 기술을 Delay초 늦게 따라 함 ("내 기술을 배워서 맞서는 거울 기사")
 * 행동(능력·교체)만 기록을 따라 함. 위치는 따라 하지 않음 (과거 위치·관계는 지금 전투와 안 맞아 엉뚱한 데로 감)
 * 이동: 지금 몸에 맞는 거리를 잡음 (전사는 붙고, 마법사는 거리 유지). 능력이 켜져 있으면 능력이 움직임을 맡음
 * 회피는 복사하지 않음: 그때의 위협에 대한 반응이라 지금 따라 하면 이유 없는 움직임이 됨 → 방어 컴포넌트가 스스로 판단
 * 채우기: 따라 할 행동이 IdleFillSeconds 동안 없으면(플레이어가 가만히 있으면) 기록에 남은 내 기술 하나를 바로 씀 → 가만히 서서 버티기를 막음
 * 2페이즈: 지연을 줄이고, ReverseInterval마다 "거울 반전" = 예고(멈춰서 빛남) 뒤 최근 ReverseWindow초의 내 기술을 거꾸로 순서로
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

	// 2페이즈 시작 (거울 보스가 2페이즈 알림을 받으면 부름)
	void EnterPhaseTwo();

	// 거울 반전 예고 중인지 (거울 보스가 예고 겹침 재질을 켬)
	bool IsReverseWarning() const { return ReverseState == EReverseState::Warning; }

	// 등장 재연 시작: 기록의 마지막 IntroStepCount개 기술 (없으면 DefaultIntroSteps). 쓸 게 없으면 false
	bool StartIntro();

	// 등장 재연 중인지 (보스의 IsIntroPlaying이 물음)
	bool IsIntroPlaying() const;

	// 등장 재연 중단 (건너뛰기)
	void StopIntro();
	
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

	// 2페이즈 지연 (내 기술을 더 빨리 돌려줌)
	UPROPERTY(EditDefaultsOnly, Category = "Mirror|Phase2", meta = (ClampMin = "0.1"))
	float PhaseTwoDelay = 1.5f;

	// 거울 반전 간격(초). 2페이즈에 들어가고 이만큼 뒤 첫 반전
	UPROPERTY(EditDefaultsOnly, Category = "Mirror|Phase2", meta = (ClampMin = "1"))
	float ReverseInterval = 15.f;

	// 거울 반전이 거꾸로 쓸 구간 (지금부터 이만큼 전까지의 내 기술)
	UPROPERTY(EditDefaultsOnly, Category = "Mirror|Phase2", meta = (ClampMin = "0.5"))
	float ReverseWindow = 5.f;

	// 거울 반전 예고 시간 (멈춰서 빛남 → 플레이어가 대비)
	UPROPERTY(EditDefaultsOnly, Category = "Mirror|Phase2", meta = (ClampMin = "0"))
	float ReverseWarnTime = 1.f;

	// 거울 반전에서 기술이 안 나가면(앞 기술 동작 중·쿨다운) 이만큼 다시 시도하다 건너뜀
	UPROPERTY(EditDefaultsOnly, Category = "Mirror|Phase2", meta = (ClampMin = "0"))
	float ReverseRetryTime = 0.6f;

	// 등장에서 재연할 기술 수 (기록의 마지막 몇 개)
	UPROPERTY(EditDefaultsOnly, Category = "Mirror|Intro", meta = (ClampMin = "1"))
	int32 IntroStepCount = 4;

	// 기록을 얼마나 거슬러 볼지(초). 지휘자를 잡은 뒤 처치 연출·카드 고르기를 지나도 남게
	UPROPERTY(EditDefaultsOnly, Category = "Mirror|Intro", meta = (ClampMin = "1"))
	float IntroLookBack = 30.f;

	// 기록이 없을 때 대신 쓸 순서 (거울부터 시작한 시험 등)
	UPROPERTY(EditDefaultsOnly, Category = "Mirror|Intro")
	TArray<FSMirrorIntroStep> DefaultIntroSteps;

	// 첫 기술까지 기다리는 시간(초). 등장 카메라가 다 옮겨 간 뒤 시작 (카메라 이동 1초)
	UPROPERTY(EditDefaultsOnly, Category = "Mirror|Intro", meta = (ClampMin = "0"))
	float IntroStartDelay = 1.f;

	// 기록의 기술 간격이 이보다 길면 줄임(초). 지휘자전에서 몇 초씩 쉬었어도 등장이 늘어지지 않게
	UPROPERTY(EditDefaultsOnly, Category = "Mirror|Intro", meta = (ClampMin = "0.1"))
	float IntroMaxGap = 0.8f;
	
private:
	enum class EReverseState : uint8
	{
		None,
		Warning,
		Playing,
	};

	// 거울 반전이 쓸 기술 하나 (거꾸로 놓은 순서, 원래 간격 그대로)
	struct FReverseStep
	{
		TSubclassOf<UGameplayAbility> AbilityClass;
		ETwinSlot Slot = ETwinSlot::Warrior;
		double PlayAt = 0.0;
	};

	// 공격·시전·회피 중이거나 경직이면 바쁨 (채우기·반전은 한가할 때만 시작)
	bool IsBusy() const;

	// 거울 반전 진행 (기다림 → 예고 → 재생). 예고·재생 중이면 true → 따라 하기·채우기를 쉼
	bool UpdateReverse(double Now);

	// 시각표(ReverseSteps)에서 때가 된 기술을 누름 (실패하면 ReverseRetryTime까지 다시, 넘으면 건너뜀). 다 썼으면 true
	bool PlaySteps(double Now);

	// 등장 재연 진행: 시각표를 다 쓰고 켜진 능력이 없으면 끝
	void UpdateIntro(double Now);
	
	// 최근 ReverseWindow초 기술을 거꾸로 놓음. 없으면 false
	bool BuildReverseSteps(double Now);

	// 기술 하나 쓰기: 몸이 다르면 쓸 수 있는지 먼저 보고 바꿈 (못 쓸 기술 때문에 진행 중 공격을 끊지 않게). 먹혔으면 LastActionTime 갱신
	bool PressAction(TSubclassOf<UGameplayAbility> AbilityClass, ETwinSlot Slot);

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

	bool bPhaseTwo = false;

	EReverseState ReverseState = EReverseState::None;

	// 다음 반전을 시작할 수 있는 시각 / 지금 상태가 시작된 시각
	double NextReverseTime = 0.0;
	double ReverseStateTime = 0.0;

	// 반전 순서와 다음에 쓸 칸
	TArray<FReverseStep> ReverseSteps;
	int32 ReverseIndex = 0;

	// 등장 재연 중 (시각표는 반전과 같은 ReverseSteps·ReverseIndex를 씀)
	bool bIntroPlaying = false;
};
