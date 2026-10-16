#pragma once

#include "CoreMinimal.h"
#include "Ability/GA/SGA_CastAbility.h"
#include "Curves/CurveFloat.h"
#include "Engine/TimerHandle.h"
#include "SGA_CrystalTorrent.generated.h"

class UNiagaraSystem;

// 얼음 쇄도가 한 적을 맞힌 기록
struct FTorrentHitRecord
{
	// 맞은 횟수
	int32 Count = 0;

	// 마지막으로 맞은 시각 (게임 시간, 초)
	double LastHitTime = 0.0;
};

/**
 * 얼음 쇄도 (키 3)
 * - 손에서 커서 쪽으로 얼음 결정 무더기(NS_Crystal_Torrent)가 날아가고, 그 앞쪽 끝까지의 띠 안의 적을 타격
 * - 앞쪽 끝 위치는 이펙트를 재서 만든 커브(FrontDistanceCurve)로 계산 → 처음엔 느리고 점점 빨라지는 이펙트와 판정이 맞음
 * - 관통: 띠 안의 적 모두. 같은 적은 HitInterval 간격으로 HitsPerTarget번까지, 첫 타에 [빙결]
 * - 판정은 Duration 동안. 캐릭터는 RecoveryTime 뒤 움직일 수 있고, 판정이 끝나면 능력 종료 (부모 USGA_CastAbility)
 */
UCLASS()
class TWINSWIFT_API USGA_CrystalTorrent : public USGA_CastAbility
{
	GENERATED_BODY()

public:
	USGA_CrystalTorrent();

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 시간(초) → 얼음 앞쪽 끝까지의 거리(cm). 이펙트(NS_Crystal_Torrent)를 재서 만든 값. 생성자에서 키를 넣음
	UPROPERTY(EditDefaultsOnly, Category = "Torrent")
	FRuntimeFloatCurve FrontDistanceCurve;

	// 띠 최대 길이(cm). 앞쪽 끝이 여기까지 오면 더 늘어나지 않음
	UPROPERTY(EditDefaultsOnly, Category = "Torrent", meta = (ClampMin = "0"))
	float MaxRange = 1200.f;

	// 띠 반폭(cm). 폭 = 2배
	UPROPERTY(EditDefaultsOnly, Category = "Torrent", meta = (ClampMin = "0"))
	float HalfWidth = 60.f;

	// 판정이 살아 있는 시간(초). 이펙트는 이 뒤에도 끝까지 재생
	UPROPERTY(EditDefaultsOnly, Category = "Torrent", meta = (ClampMin = "0.05"))
	float Duration = 1.2f;

	// 같은 적을 맞히는 횟수
	UPROPERTY(EditDefaultsOnly, Category = "Torrent", meta = (ClampMin = "1"))
	int32 HitsPerTarget = 3;

	// 같은 적을 다시 맞히기까지 최소 간격(초)
	UPROPERTY(EditDefaultsOnly, Category = "Torrent", meta = (ClampMin = "0"))
	float HitInterval = 0.1f;

	// 손에서 생성하는 얼음 쇄도 이펙트
	UPROPERTY(EditDefaultsOnly, Category = "Torrent")
	TObjectPtr<UNiagaraSystem> TorrentEffect;

	// 발사 신호: 시작 위치·방향·시각 저장, 이펙트 생성, 기록 비우기, 판정 타이머 시작
	virtual void OnCast(const FVector& Origin, const FVector& Direction) override;

	// 판정 타이머가 끝났으면 true
	virtual bool IsCastFinished() const override;

private:
	// 0.05초마다: 앞쪽 끝까지의 띠 안에서 맞힐 차례인 적에게 데미지
	void TickTorrent();

	// 발사 위치·방향 (손 → 커서, 수평)
	FVector TorrentOrigin = FVector::ZeroVector;
	FVector TorrentDirection = FVector::ForwardVector;

	// 발사 시각 (게임 시간, 초)
	double StartTime = 0.0;

	FTimerHandle TickTimer;

	// 적마다 맞은 횟수·마지막 시각. 키를 약한 포인터로 → 적이 사라져도 안전
	TMap<TWeakObjectPtr<AActor>, FTorrentHitRecord> HitRecords;
};
