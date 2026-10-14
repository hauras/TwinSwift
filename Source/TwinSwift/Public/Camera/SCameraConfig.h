#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SCameraConfig.generated.h"

// 스킬이 겨냥할 곳을 어떻게 구할지
UENUM(BlueprintType)
enum class ESAimMode : uint8
{
	// 마우스 커서 아래 땅 (쿼터뷰)
	Cursor,

	// 화면 가운데가 가리키는 곳 (3인칭)
	ScreenCenter
};

/**
 * 시점 하나의 카메라 설정.
 * 시점을 바꿀 때 코드를 고치지 않고 이 에셋만 바꿔 끼움 (DA_Camera_Quarter, DA_Camera_ThirdPerson)
 */
UCLASS(BlueprintType)
class TWINSWIFT_API USCameraConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 캐릭터에서 카메라까지 거리
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float ArmLength = 800.f;

	// 카메라를 캐릭터 기준으로 옮기는 값 (Z를 올리면 위에서 내려다봄)
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	FVector SocketOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float FieldOfView = 90.f;

	// true: 마우스로 카메라를 돌림 (3인칭). false: 각도 고정 (쿼터뷰)
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Rotation")
	bool bFreeLook = true;

	// 고정 시점일 때 카메라 각도 (Pitch는 음수 = 위에서 내려다봄)
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Rotation", meta = (EditCondition = "!bFreeLook"))
	FRotator FixedRotation = FRotator(-55.f, 45.f, 0.f);

	// 마우스로 위아래를 볼 수 있는 범위
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Rotation", meta = (EditCondition = "bFreeLook"))
	float MinPitch = -60.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Rotation", meta = (EditCondition = "bFreeLook"))
	float MaxPitch = 10.f;

	// 카메라가 캐릭터를 살짝 늦게 따라감 (뚝뚝 끊기는 느낌 줄이기)
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Lag")
	bool bEnableCameraLag = true;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Lag", meta = (EditCondition = "bEnableCameraLag"))
	float CameraLagSpeed = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	ESAimMode AimMode = ESAimMode::ScreenCenter;
};
