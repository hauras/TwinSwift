#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "SCameraShake_Shatter.generated.h"

/**
 * 산산조각 순간의 짧은 화면 흔들림 (0.2초 "툭")
 * - 쿼터뷰라 회전 대신 위치만 흔듦 (회전은 멀리 있는 카메라에서 크게 보여 어지러움)
 * - 흔들림 모양이 BP의 인스턴스 하위 객체라 스크립트로 설정하기 어려움 → C++ 생성자에서 숫자로 정함
 */
UCLASS()
class TWINSWIFT_API USCameraShake_Shatter : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	USCameraShake_Shatter(const FObjectInitializer& ObjectInitializer);
};
