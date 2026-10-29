#pragma once

#include "CoreMinimal.h"
#include "Character/BossCharacter.h"
#include "Character/TwinSlot.h"
#include "SMirrorBoss.generated.h"

class USActionReplayComponent;
class USMirrorDefenseComponent;

// 보스 3: 플레이어 행동을 늦게 따라 하는 거울 기사. 몸 둘(전사·마법사), ASC 하나
UCLASS()
class TWINSWIFT_API ASMirrorBoss : public ABossCharacter
{
	GENERATED_BODY()

public:
	ASMirrorBoss();

	bool IsDefending() const;

	// 재생 컴포넌트가 마법사 거리 벌리기(점멸)에 씀
	USMirrorDefenseComponent* GetDefenseComponent() const { return DefenseComponent; }

	// 몸 전환 (재생 컴포넌트가 기록된 교체를 따라 부름). 몽타주를 틀 메시도 보이는 몸으로 바꿈
	void SwapTo(ETwinSlot NewSlot);

	// 지금 몸
	ETwinSlot GetActiveSlot() const { return ActiveSlot; }

	// 칼 판정(ANS_WeaponTrace)이 쓰는 칼
	virtual UStaticMeshComponent* GetWeaponMesh() const override { return WeaponMesh; }

	// 겨냥 = 지금 플레이어 위치 (움직임은 기록을 따라 하지만 공격은 지금의 플레이어를 노림)
	virtual bool GetAbilityAimPoint(FVector& OutAimPoint) const override;

	// 회피 방향 = 방어 컴포넌트가 넣어 준 값. 비어 있으면 false
	virtual bool GetAbilityMoveDirection(FVector& OutDirection) const override;

	// 방어 컴포넌트가 "이번 회피는 이 방향"을 넣어 줌 (회피 직후 0으로 되돌림)
	void SetReplayMoveDirection(const FVector& Direction) { ReplayMoveDirection = Direction; }

protected:
	// 마법사 몸을 기사 몸과 같은 자리에 (플레이어와 같음)
	virtual void OnConstruction(const FTransform& Transform) override;

	// 처음엔 전사 몸만 보이게
	virtual void BeginPlay() override;

	// 두뇌 = 행동 트리 대신 행동 재생 (전투 시작 / 대지의 분노·큰 경직 동안 멈춤 / 다시)
	virtual bool StartAI() override;
	virtual void StopAI() override;
	virtual void ResumeAI() override;

	// 마법사 몸 (기사 몸은 GetMesh())
	UPROPERTY(VisibleAnywhere, Category = "Mirror")
	TObjectPtr<USkeletalMeshComponent> MageMesh;

	// 칼 (기사 몸의 WeaponSocket에 붙음)
	UPROPERTY(VisibleAnywhere, Category = "Mirror")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	// 플레이어 공격 예고(내려찍기)를 보고 스스로 피함 (회피는 복사하지 않음)
	UPROPERTY(VisibleAnywhere, Category = "Mirror")
	TObjectPtr<USMirrorDefenseComponent> DefenseComponent;

	// 플레이어 기록을 늦게 따라 함
	UPROPERTY(VisibleAnywhere, Category = "Mirror")
	TObjectPtr<USActionReplayComponent> ReplayComponent;

private:
	// 지금 몸
	ETwinSlot ActiveSlot = ETwinSlot::Warrior;

	// 방어 컴포넌트가 넣어 준 회피 방향
	FVector ReplayMoveDirection = FVector::ZeroVector;
};
