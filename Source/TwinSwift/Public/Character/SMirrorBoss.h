#pragma once

#include "CoreMinimal.h"
#include "Character/BossCharacter.h"
#include "Character/TwinSlot.h"
#include "SMirrorBoss.generated.h"

// 보스 3: 플레이어 행동을 늦게 따라 하는 거울 기사. 몸 둘(전사·마법사), ASC 하나
UCLASS()
class TWINSWIFT_API ASMirrorBoss : public ABossCharacter
{
	GENERATED_BODY()

public:
	ASMirrorBoss();

	// 몸 전환 (재생 컴포넌트가 기록된 교체를 따라 부름). 몽타주를 틀 메시도 보이는 몸으로 바꿈
	void SwapTo(ETwinSlot NewSlot);

	// 지금 몸
	ETwinSlot GetActiveSlot() const { return ActiveSlot; }

	// 칼 판정(ANS_WeaponTrace)이 쓰는 칼
	virtual UStaticMeshComponent* GetWeaponMesh() const override { return WeaponMesh; }

	// 겨냥 = 지금 플레이어 위치 (움직임은 기록을 따라 하지만 공격은 지금의 플레이어를 노림)
	virtual bool GetAbilityAimPoint(FVector& OutAimPoint) const override;

	// 회피 방향 = 재생 컴포넌트가 넣어 준 값. 비어 있으면 false
	virtual bool GetAbilityMoveDirection(FVector& OutDirection) const override;

	// 재생 컴포넌트가 "이번 회피는 이 방향"을 넣어 줌
	void SetReplayMoveDirection(const FVector& Direction) { ReplayMoveDirection = Direction; }

protected:
	// 마법사 몸을 기사 몸과 같은 자리에 (플레이어와 같음)
	virtual void OnConstruction(const FTransform& Transform) override;

	// 처음엔 전사 몸만 보이게
	virtual void BeginPlay() override;

	// 마법사 몸 (기사 몸은 GetMesh())
	UPROPERTY(VisibleAnywhere, Category = "Mirror")
	TObjectPtr<USkeletalMeshComponent> MageMesh;

	// 칼 (기사 몸의 WeaponSocket에 붙음)
	UPROPERTY(VisibleAnywhere, Category = "Mirror")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

private:
	// 지금 몸
	ETwinSlot ActiveSlot = ETwinSlot::Warrior;

	// 재생 쪽이 넣어 준 회피 방향
	FVector ReplayMoveDirection = FVector::ZeroVector;
};
