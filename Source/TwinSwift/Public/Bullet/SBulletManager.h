#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SBulletManager.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UInstancedStaticMeshComponent;

// 탄 색: 주황은 전사가, 파랑은 마법사가 막음 (그 캐릭터로 교체해야 통과)
enum class EBulletColor : uint8
{
	Orange,
	Blue,
};

// 탄 1발. 에디터에 보일 필요도, GC가 지킬 물건도 없어서 UPROPERTY 없는 일반 구조체
// → 메모리에 붙어 있어 반복문이 빠르고, GC가 훑지 않음
struct FSBullet
{
	FVector Location = FVector::ZeroVector;

	// 초당 이동 (방향 × 속도)
	FVector Velocity = FVector::ZeroVector;

	// 남은 시간(초). 0 이하면 지움
	float LifeRemaining = 0.f;

	// 판정 반경(cm)
	float Radius = 0.f;

	EBulletColor Color = EBulletColor::Orange;

	// 쏜 쪽 공격력 × 배율
	float DamageMultiplier = 1.f;

	// 쏜 쪽 ASC (피해 GE를 만들 때). 날아가는 동안 보스가 사라질 수 있어 약한 포인터 → 쓸 때 살아 있는지 확인
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
};

/**
 * 보스 탄막 매니저 (레벨에 하나)
 * 탄 1발 = 액터가 아니라 배열 한 칸. Tick 한 번에 이동 → 플레이어와 거리 검사 → 정리 → 인스턴스 메시 하나로 그리기
 * 플레이어 마법(수십 발, 유도·관통·궤적 이펙트)은 액터 풀(ASProjectile) 그대로, 단순하고 많은 탄막만 여기
 */
UCLASS()
class TWINSWIFT_API ASBulletManager : public AActor
{
	GENERATED_BODY()

public:
	ASBulletManager();

	// 월드에 놓인 매니저 (없으면 nullptr). 보스 능력·측정 명령이 이걸로 가져다 씀
	static ASBulletManager* Get(const UObject* WorldContext);

	// 탄 1발 추가 (배열 끝에). MaxBullets를 넘으면 쏘지 않음
	// LifeTime: 0 이하면 BulletLifeTime (측정에서 액터 투사체와 수명을 맞출 때만 넘김)
	void SpawnBullet(
		const FVector& Location,
		const FVector& Velocity,
		EBulletColor Color,
		UAbilitySystemComponent* SourceASC,
		float DamageMultiplier,
		float LifeTime = 0.f);

	// 전부 지우기 (전투 끝, 측정 끝). 탄을 도는 중(충돌 처리 안에서 패배 등)에 불리면 그 처리가 끝난 뒤 지움
	void ClearAllBullets();

	// 지금 탄 수 (측정 로그)
	int32 GetBulletCount() const { return Bullets.Num(); }

	// 탄 수명(초). 측정 명령이 "떠 있는 탄 수 = 초당 발수 × 수명"을 맞출 때
	float GetBulletLifeTime() const { return BulletLifeTime; }

	// 지난 Tick에서 이동·충돌·그리기에 걸린 시간(초). 측정용
	double GetLastUpdateSeconds() const { return LastUpdateSeconds; }

protected:
	// 이동 → 충돌 → 그리기
	virtual void Tick(float DeltaSeconds) override;

	// 탄 그리기: 탄마다 컴포넌트가 아니라 이 인스턴스 메시 하나로 묶어서 그림 (실제 드로우콜 수는 패스·뷰마다 따로, stat RHI로 측정)
	UPROPERTY(VisibleAnywhere, Category = "Bullet")
	TObjectPtr<UInstancedStaticMeshComponent> BulletMeshes;

	// 맞았을 때 피해 GE (보스 공격과 같은 GE_Damage)
	UPROPERTY(EditDefaultsOnly, Category = "Bullet")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// 탄 수명(초)
	UPROPERTY(EditDefaultsOnly, Category = "Bullet", meta = (ClampMin = "0.1"))
	float BulletLifeTime = 6.f;

	// 판정 반경(cm)
	UPROPERTY(EditDefaultsOnly, Category = "Bullet", meta = (ClampMin = "1"))
	float BulletRadius = 15.f;

	// 한 번 맞으면 이 시간(초) 동안 탄에 안 맞음 (탄 여러 발이 한 프레임에 겹쳐 여러 번 맞지 않게)
	UPROPERTY(EditDefaultsOnly, Category = "Bullet", meta = (ClampMin = "0"))
	float HitCooldown = 0.25f;

	// 최대 탄 수 (넘으면 더 쏘지 않음)
	UPROPERTY(EditDefaultsOnly, Category = "Bullet", meta = (ClampMin = "1"))
	int32 MaxBullets = 4000;

private:
	TArray<FSBullet> Bullets;

	TArray<FTransform> InstanceTransforms;

	// 인스턴스마다 색 값 1개 (주황 0, 파랑 1). 재질이 PerInstanceCustomData로 읽어 색을 고름 → 색마다 컴포넌트를 나누지 않음
	TArray<float> InstanceColors;

	// GetLastUpdateSeconds 값
	double LastUpdateSeconds = 0.0;

	// 지난 이동에 쓴 시간(초). 충돌 검사가 "이번 프레임 이동 선분"을 되짚을 때 (이전 위치 = 위치 - 속도 × 이 값)
	float LastMoveDeltaSeconds = 0.f;

	// 탄 배열을 도는 중인지 (HitPlayers). 이 사이에 배열을 비우거나 늘리면 돌던 칸·참조가 깨짐
	bool bIteratingBullets = false;

	// 도는 중에 ClearAllBullets가 불림 → 다 돈 뒤에 지움
	bool bClearRequested = false;

	// 도는 중에 쏜 탄 (피해 처리 안에서 쏘는 경우) → 다 돈 뒤에 배열에 넣음
	TArray<FSBullet> PendingBullets;
	
	// 캐릭터(ASC)별 마지막으로 맞은 시각. 몸 하나에 ASC가 둘이라 액터가 아니라 ASC로 구분
	TMap<TWeakObjectPtr<UAbilitySystemComponent>, double> LastHitTimes;

	// 1. 모든 탄 이동 + 남은 시간 감소
	void MoveBullets(float DeltaSeconds);

	// 2. 플레이어 몸과 거리 검사 → 맞은 탄·시간이 다 된 탄 지우기
	void HitPlayers();

	// 탄 하나 × 플레이어 하나. 맞아서 탄을 지워야 하면 true
	bool TryHit(FSBullet& Bullet, APawn* Target);

	// 3. 맨 끝 칸과 바꿔 지우기 (O(1), 대신 순서가 바뀜)
	void RemoveBulletAt(int32 Index);

	// 4. 남은 탄 위치를 인스턴스 메시에 넘김
	void UpdateInstances();
};
