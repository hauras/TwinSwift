#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CharacterAnimInstance.generated.h"

class ACharacterBase;
class UCharacterMovementComponent;

/**
 * 이동 애니메이션 값 계산. 애님 그래프는 이 값을 연결만 함
 */
UCLASS()
class TWINSWIFT_API UCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;

	// 워커 스레드: 이동 값만 계산 (UObject 상태 읽기는 조각 2에서 NativeUpdateAnimation에 추가)
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY()
	TObjectPtr<ACharacterBase> OwningCharacter;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> OwningMovementComponent;

	// 땅 위 속도 (블렌드 스페이스 입력)
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Locomotion")
	float GroundSpeed = 0.f;

	// 키를 누르고 있는지 (멈추는 중인지 구분)
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Locomotion")
	bool bHasAcceleration = false;

	// Idle ↔ 이동 상태 전환 조건
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Locomotion")
	bool bShouldMove = false;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Locomotion")
	bool bIsFalling = false;

	// 원본 애니가 허리를 너무 숙여서(Idle 약 31°) 세우는 보정값
	// AnimGraph의 Transform (Modify) Bone(spine_001, Add to Existing, Component Space)에 연결
	// 기사 기준 Roll -15 = 뒤로 15° 젖힘. 캐릭터마다 ABP 기본값에서 조절
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimData|Correction")
	FRotator SpineLeanCorrection = FRotator(0.f, 0.f, -15.f);
};
