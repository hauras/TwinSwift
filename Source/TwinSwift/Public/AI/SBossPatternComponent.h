#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "SBossPatternComponent.generated.h"

class UAbilitySystemComponent;

// 보스 패턴 하나 (BP의 목록에 한 줄)
USTRUCT(BlueprintType)
struct FSBossPattern
{
	GENERATED_BODY()

	// 발동할 공격 능력의 태그 (능력의 Asset Tags)
	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (Categories = "Ability.Boss"))
	FGameplayTag AbilityTag;

	// 이 거리(cm) 안에서만 씀
	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (ClampMin = "0"))
	float MinRange = 0.f;

	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (ClampMin = "0"))
	float MaxRange = 500.f;

	// 가장 좋아하는 거리(cm). 여기서 거리 점수 1, 범위 끝으로 갈수록 MinDistanceScore까지 내려감
	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (ClampMin = "0"))
	float PreferredRange = 300.f;

	// 기본 가중치 (자주 쓸수록 크게)
	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (ClampMin = "0"))
	float Weight = 1.f;
};

/**
 * 보스 패턴 선택기: "지금 어떤 공격을 할지"를 고름 (BT에는 "골라서 공격" 노드 하나)
 * 1) 거르기: 거리 범위 밖·쿨다운 중·발동 불가인 패턴 제외
 * 2) 유틸리티 점수: 가중치 × 거리 점수(선호 거리에 가까울수록) × 최근 사용 감점
 * 3) 최고점의 CandidateRatio 이상인 후보만 남김 → 4) 셔플 백 순서로 하나 고름
 *    셔플 백 = 카드 덱: 패턴 번호를 한 장씩 넣고 섞어서 꺼냄. 다 꺼내면 다시 채움 → 같은 패턴이 몰리지 않고, 순서는 예측하기 어려움
 * 상황 판단(점수)과 반복 방지(가방)를 나눠서, 패턴을 늘릴 때는 Patterns에 한 줄만 추가
 */
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class TWINSWIFT_API USBossPatternComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USBossPatternComponent();

	// 표적까지 거리(cm)로 패턴을 골라 태그를 돌려줌. 고를 게 없으면 빈 태그. 고른 패턴은 가방에서 빠지고 최근 사용에 기록
	FGameplayTag SelectPattern(float DistanceToTarget);

	// [측정] Count번 고르기를 흉내 내서 선택기와 완전 무작위를 비교해 로그로 (분포, 같은 패턴 최대 연속). 쿨다운은 무시, 거리는 무작위
	// 지금 상태(가방·기록·난수)는 건드리지 않음
	void RunSimulation(int32 Count, int32 Seed) const;

protected:
	virtual void BeginPlay() override;

	// 패턴 목록 (BP_Boss_Golem에서 입력)
	UPROPERTY(EditDefaultsOnly, Category = "Pattern")
	TArray<FSBossPattern> Patterns;

	// 최근에 쓴 패턴 감점: [0] 직전, [1] 그 전 … (곱함, 1이면 감점 없음)
	UPROPERTY(EditDefaultsOnly, Category = "Pattern")
	TArray<float> RecentPenalties = { 0.4f, 0.75f };

	// 범위 끝에서의 거리 점수 (선호 거리 1 → 끝 이 값)
	UPROPERTY(EditDefaultsOnly, Category = "Pattern", meta = (ClampMin = "0", ClampMax = "1"))
	float MinDistanceScore = 0.3f;

	// 최고점의 이 비율 이상이면 후보 (1이면 항상 최고점만 → 가방이 의미 없음, 0이면 점수 무시)
	UPROPERTY(EditDefaultsOnly, Category = "Pattern", meta = (ClampMin = "0", ClampMax = "1"))
	float CandidateRatio = 0.7f;

private:
	// 고르기 한 번에 필요한 상태. 실제 게임과 시뮬레이션이 같은 코드를 쓰게 묶음
	struct FSelectState
	{
		// 셔플 백: 아직 안 꺼낸 패턴 번호
		TArray<int32> Bag;

		// 최근 사용 (0 = 직전)
		TArray<int32> History;

		FRandomStream Random;
	};

	// 점수 계산 → 후보 → 가방에서 고르기 → 기록. 고른 번호 (없으면 INDEX_NONE)
	// bCheckAbility: 쿨다운·발동 가능 검사 (시뮬레이션은 끔). OutScores: 디버그 표시용
	int32 SelectIndex(FSelectState& State, float DistanceToTarget, bool bCheckAbility, TArray<float>* OutScores) const;

	// 패턴 하나의 점수 (못 쓰면 0)
	float ScorePattern(int32 Index, const FSelectState& State, float DistanceToTarget, bool bCheckAbility) const;

	// 쿨다운·차단 태그로 지금 발동할 수 없으면 false
	bool CanActivatePattern(const FSBossPattern& Pattern) const;

	// 가방을 모든 패턴으로 다시 채우고 섞음. 직전 패턴이 맨 앞에 오지 않게 (가방 경계에서 연속 방지)
	void RefillBag(FSelectState& State) const;

	// 고른 이유를 보스 머리 위에 표시 (콘솔 ts.Boss.ShowPatternScores 1)
	void DrawScores(const TArray<float>& Scores, int32 Chosen, float DistanceToTarget) const;

	UAbilitySystemComponent* GetASC() const;

	FSelectState State;
};
