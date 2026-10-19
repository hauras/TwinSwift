#pragma once

#include "CoreMinimal.h"
#include "Ability/GA/SGA_CastAbility.h"
#include "Engine/TimerHandle.h"
#include "SGA_ThunderStrike.generated.h"

class UNiagaraSystem;

/**
 * 낙뢰 (키 4)
 * - 손을 내려치는 신호에 커서 위치(최대 MaxRange)에 경고 원 → StrikeDelay 뒤 번개, 반경 Radius 안의 적을 DamageMultiplier 배율로 타격
 * - 캐릭터는 RecoveryTime 뒤 움직일 수 있고, 번개가 떨어지면 능력 종료 (부모 USGA_CastAbility)
 * - [균열] 연쇄 폭발 (BFS): 번개에 맞은 [균열] 적이 터지고, 폭발 범위 안의 다른 [균열] 적이 다음 단계에 터짐
 *   - 단계 = BFS 깊이. 같은 단계는 동시에, 단계 사이는 ChainStepDelay초 차이 ("쾅 → 쾅 → 쾅")
 *   - 방문 표시(ChainVisited)로 한 적은 한 번만 터짐. MaxChainDepth 단계까지
 *   - 폭발은 범위 안 모든 적에게 피해, 연쇄는 [균열]이 있는 적으로만 이어짐
 *   - 번개가 떨어진 뒤 회피 등으로 능력이 끊겨도, 이미 시작된 연쇄는 끝까지 감
 */
UCLASS()
class TWINSWIFT_API USGA_ThunderStrike : public USGA_CastAbility
{
	GENERATED_BODY()

public:
	USGA_ThunderStrike();

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 최대 사거리(cm). 커서가 더 멀면 그 방향으로 여기까지만
	UPROPERTY(EditDefaultsOnly, Category = "Thunder", meta = (ClampMin = "0"))
	float MaxRange = 1000.f;

	// 타격 반경(cm)
	UPROPERTY(EditDefaultsOnly, Category = "Thunder", meta = (ClampMin = "0"))
	float Radius = 250.f;

	// 경고 원을 띄우고 번개가 떨어질 때까지(초). 적이 피할 시간
	UPROPERTY(EditDefaultsOnly, Category = "Thunder", meta = (ClampMin = "0"))
	float StrikeDelay = 0.5f;

	// 타격 순간 떨어지는 번개
	UPROPERTY(EditDefaultsOnly, Category = "Thunder")
	TObjectPtr<UNiagaraSystem> StrikeEffect;

	// 경고 원 이펙트. 비우면 디버그 원으로 그림
	UPROPERTY(EditDefaultsOnly, Category = "Thunder")
	TObjectPtr<UNiagaraSystem> WarningEffect;

	// [균열] 폭발 반경(cm). 이 안의 적이 피해를 받고, 그중 [균열] 적이 다음 단계에 터짐
	UPROPERTY(EditDefaultsOnly, Category = "Thunder|Crack Burst", meta = (ClampMin = "0"))
	float BurstRadius = 300.f;

	// [균열] 폭발 피해 배율
	UPROPERTY(EditDefaultsOnly, Category = "Thunder|Crack Burst", meta = (ClampMin = "0"))
	float BurstMultiplier = 1.5f;

	// 연쇄 단계 사이 시간(초). 0이면 한 프레임에 모두 터짐
	UPROPERTY(EditDefaultsOnly, Category = "Thunder|Crack Burst", meta = (ClampMin = "0"))
	float ChainStepDelay = 0.12f;

	// 연쇄 최대 단계 (번개에 직접 맞은 적 = 1단계)
	UPROPERTY(EditDefaultsOnly, Category = "Thunder|Crack Burst", meta = (ClampMin = "1"))
	int32 MaxChainDepth = 5;

	// [균열] 폭발 이펙트 (터지는 적 위치). 비우면 디버그 원으로 그림
	UPROPERTY(EditDefaultsOnly, Category = "Thunder|Crack Burst")
	TObjectPtr<UNiagaraSystem> BurstEffect;

	// 손을 내려치는 신호: 커서 위치에 경고 원 + StrikeDelay 타이머
	virtual void OnCast(const FVector& Origin, const FVector& Direction) override;

	// 번개가 떨어졌으면 true
	virtual bool IsCastFinished() const override;

private:
	// StrikeDelay 뒤에 불림: 번개 이펙트 + 범위 타격 + 종료 시도
	void Strike();

	// 번개가 떨어질 위치 (바닥)
	FVector StrikeLocation = FVector::ZeroVector;

	// 경고 → 번개까지의 타이머 (반복 안 함)
	FTimerHandle StrikeTimer;

	// 번개가 이미 떨어졌는지 (IsCastFinished가 씀)
	bool bStruck = false;

	// 번개에 맞은 적 중 [균열] 적을 1단계로 넣고 연쇄 시작
	void StartCrackChain(const TArray<AActor*>& StruckActors);

	// 지금 단계(ChainWave)의 적을 모두 터뜨리고, 다음 단계를 모아 ChainStepDelay 뒤 다시 부름
	void ExplodeChainWave();

	// 적 하나 터뜨리기: [균열] 제거 + 이펙트 + 범위 피해. 범위 안에서 다음 단계로 갈 적을 OutNextWave에
	// 터졌으면 true (기다리는 동안 사라졌거나 [균열]이 끝났으면 false)
	bool ExplodeOne(
		AActor* Source,
		TSet<AActor*>& DamagedThisWave,
		TArray<TWeakObjectPtr<AActor>>& OutNextWave);

	// BFS 큐의 현재 단계 (이번에 터질 적들). 비어 있으면 연쇄가 끝난 것
	TArray<TWeakObjectPtr<AActor>> ChainWave;

	// BFS 방문 표시: 한 번 줄에 들어간 적은 다시 넣지 않음 (같은 적이 두 번 터지지 않음)
	TSet<TWeakObjectPtr<AActor>> ChainVisited;

	// 지금 단계 번호 (1부터)
	int32 ChainDepth = 0;

	// 단계 사이 타이머
	FTimerHandle ChainTimer;
};
