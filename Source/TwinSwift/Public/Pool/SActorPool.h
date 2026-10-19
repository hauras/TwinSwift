#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SActorPool.generated.h"

// 액터 클래스 하나의 빈 칸 스택 (TMap 값으로 TArray를 바로 못 넣어서 감쌈)
USTRUCT()
struct FSActorFreeList
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AActor>> Items;
};

/**
 * 액터 오브젝트 풀 (월드마다 하나, 엔진이 자동 생성)
 * - 매번 생성·삭제하지 않고 끝난 액터를 숨겨 두었다가 다시 씀 → 생성 비용·버리는 객체(GC 부담) 감소
 * - 클래스별 빈 칸 스택: 꺼내기·반납 O(1). 방금 반납한 것을 먼저 다시 씀 (메모리가 캐시에 남아 있을 가능성)
 * - 넣을 액터는 ISPoolable 구현 (꺼낼 때·넣을 때 할 일은 액터가 정함). 투사체, 데미지 숫자
 * - 처음엔 투사체 전용(USProjectilePool)이었다가, 데미지 숫자에도 필요해져서 인터페이스로 일반화
 * - 콘솔 변수 ts.Pool.Enabled 0: 다시 쓰지 않고 매번 생성·삭제 (풀 전후 측정용, 코드 경로는 그대로)
 */
UCLASS()
class TWINSWIFT_API USActorPool : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// 꺼내기: 스택에 있으면 꺼내고 없으면 새로 생성. OnAcquiredFromPool까지 부른 뒤 돌려줌
	AActor* AcquireActor(
		TSubclassOf<AActor> Class,
		const FTransform& Transform,
		AActor* Owner,
		APawn* InInstigator);

	// 타입을 맞춰 꺼내기 (예: Pool->Acquire<ASProjectile>(ProjectileClass, ...))
	template <typename T>
	T* Acquire(
		TSubclassOf<T> Class,
		const FTransform& Transform,
		AActor* Owner = nullptr,
		APawn* InInstigator = nullptr)
	{
		return Cast<T>(AcquireActor(Class, Transform, Owner, InInstigator));
	}

	// 반납: 액터가 끝날 때 스스로 부름. OnReleasedToPool 후 스택에. 꽉 찼거나 풀이 꺼져 있으면 지움
	void Release(AActor* Actor);

	// 풀이 켜져 있는지 (ts.Pool.Enabled). 나이아가라 이펙트 풀(SpawnOneShotEffect)도 같이 따름
	static bool IsPoolingEnabled();

private:
	// 클래스별 빈 칸 스택 (투사체 종류·데미지 숫자가 섞이지 않게)
	UPROPERTY()
	TMap<TSubclassOf<AActor>, FSActorFreeList> FreeLists;

	// 클래스마다 쌓아 둘 최대 개수. 넘치면 반납 대신 지움
	static constexpr int32 MaxFreePerClass = 64;
};
