#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pool/SPoolable.h"
#include "SBossTelegraph.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * 보스 공격 바닥 예고: 바닥 바로 위의 얇은 판 하나. 원·부채꼴 모양은 재질 M_BossTelegraph가 월드 좌표로 계산
 * (판 중심에서의 거리·방향 → 메시 UV 방향과 상관없이 부채꼴이 정면을 향함)
 * 데칼이 아닌 이유: 이 프로젝트의 DBuffer 데칼은 발광이 안 나와 어두운 바닥에서 안 보임. 경기장 바닥이 평평해 판으로 충분
 * USGA_BossAttack이 공격마다 풀에서 꺼내 모양을 정하고, 공격 시계로 진행률을 올림 → 가득 차는 순간 맞음
 * 공격마다 생기고 사라져서 오브젝트 풀(USActorPool)로 다시 씀
 */
UCLASS()
class TWINSWIFT_API ASBossTelegraph : public AActor, public ISPoolable
{
	GENERATED_BODY()

public:
	ASBossTelegraph();

	// 위치(바닥 중심)·방향(부채꼴 정면)·반경(cm)·부채꼴 전체 각도(도, 360이면 원)
	void Setup(const FVector& Center, const FRotator& Facing, float Radius, float FanAngle);

	// 안쪽이 차오른 정도 (0 = 테두리만, 1 = 가득 = 맞는 순간)
	void SetProgress(float Progress);

	// 맞는 순간: 밝게 번쩍 → FlashDuration 뒤 풀에 반납
	void FlashAndRelease();

	// 공격이 맞기 전에 끝남(취소): 번쩍 없이 바로 반납
	void Release();

	// ISPoolable: 꺼낼 때 보임 / 넣을 때 숨기고 번쩍 타이머 정리
	virtual void OnAcquiredFromPool() override;
	virtual void OnReleasedToPool() override;

protected:
	// 바닥 판 (BP에서 평면 메시 + M_BossTelegraph 지정). 크기 100cm 기준으로 반경에 맞춰 늘림
	UPROPERTY(VisibleAnywhere, Category = "Telegraph")
	TObjectPtr<UStaticMeshComponent> Plane;

	// 번쩍이는 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "Telegraph", meta = (ClampMin = "0.05"))
	float FlashDuration = 0.15f;

	// 바닥에서 띄우는 높이(cm). 바닥과 겹쳐 깜빡이지 않게
	UPROPERTY(EditDefaultsOnly, Category = "Telegraph", meta = (ClampMin = "0"))
	float HeightOffset = 3.f;

private:
	// 재질 파라미터를 바꾸려고 만드는 사본. 처음 한 번만 만들고 풀에서 다시 꺼내도 그대로 씀
	UMaterialInstanceDynamic* GetMaterial();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MaterialInstance;

	FTimerHandle FlashTimer;

	// 꺼내져서 쓰이는 중인지. 반납을 두 번 불러도(번쩍 타이머 + 능력 종료) 풀에 두 번 들어가지 않게
	bool bInUse = false;
};
