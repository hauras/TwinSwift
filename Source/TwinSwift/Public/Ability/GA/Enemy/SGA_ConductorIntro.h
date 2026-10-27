#pragma once

#include "CoreMinimal.h"
#include "Ability/GA/Enemy/SGA_ConductorPattern.h"
#include "SGA_ConductorIntro.generated.h"

/**
 * 지휘자 등장 (Asset Tags에 Ability.Boss.Conductor.Intro, 보스의 IntroAbilityTag)
 * 1) 하강: 착지 지점(전투 진행 액터가 넘김) 바로 위에서 일직선으로 천천히, 끝에서 감속하며 내려옴
 * 2) 땅에 닿으면 패턴 능력 그대로: 손을 모으는 동안 몸 색 주황 → 파랑 → 손을 펴며 첫 박 고리 (DA_Conductor_Intro, 피해 0·짧은 수명)
 *    → "몸 색 = 탄 색" 규칙을 전투 전에 한 번 보여 줌 (충격파와 같은 몽타주)
 * 표적은 AI가 아직 안 돌아서 블랙보드 대신 플레이어. 카메라·입력 잠금·건너뛰기는 전투 진행 액터가 함
 */
UCLASS()
class TWINSWIFT_API USGA_ConductorIntro : public USGA_ConductorPattern
{
	GENERATED_BODY()

public:
	// 하강부터 시작. 패턴(몽타주·색·첫 박)은 땅에 닿은 뒤 부모 ActivateAbility로
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	virtual const AActor* FindTarget(const APawn* Boss) const override;

	virtual void OnPatternStarted() override;

	// 착지 지점보다 얼마나 위에서 시작하나(cm). 등장 카메라 화면 위쪽 밖에서 들어오게
	UPROPERTY(EditDefaultsOnly, Category = "Intro", meta = (ClampMin = "0"))
	float DescendHeight = 1200.f;

	// 내려오는 시간(초). 천천히 출발해 가운데서 가장 빠르고 땅 근처에서 다시 느려짐 (평균 속도 = 높이 / 시간)
	UPROPERTY(EditDefaultsOnly, Category = "Intro", meta = (ClampMin = "0.05"))
	float DescendDuration = 4.f;

	// 몸 색을 보여 주는 때(초, 몽타주 시작부터): 주황 → 파랑 → 끔 (끄는 때 = 첫 박 근처)
	UPROPERTY(EditDefaultsOnly, Category = "Intro", meta = (ClampMin = "0"))
	float OrangeShowTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Intro", meta = (ClampMin = "0"))
	float BlueShowTime = 0.75f;

	UPROPERTY(EditDefaultsOnly, Category = "Intro", meta = (ClampMin = "0"))
	float ColorEndTime = 1.3f;

private:
	// 짧은 주기로 호출: 시간 비율로 높이만 바꿈 (수평은 그대로 → 일직선). 다 내려오면 FinishDescend
	void UpdateDescend();
	void FinishDescend();

	// 타이머가 부름: RolledColor를 바꿔 몸 색 예고를 다시 씌움 / 끔
	void ShowOrange();
	void ShowBlue();
	void EndColorShow();

	// 하강 시작·끝 위치, 시작 시각 (월드 시간)
	FVector DescendStart = FVector::ZeroVector;
	FVector DescendEnd = FVector::ZeroVector;
	double DescendStartTime = 0.0;

	// 하강 중인지 (건너뛰기로 중간에 끝나면 이동 모드를 되돌려야 함)
	bool bDescending = false;

	FTimerHandle DescendTimer;
	FTimerHandle OrangeTimer;
	FTimerHandle BlueTimer;
	FTimerHandle ColorEndTimer;
};
