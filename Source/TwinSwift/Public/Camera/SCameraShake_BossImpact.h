#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "SCameraShake_BossImpact.generated.h"

/**
 * 보스 공격이 바닥을 친 순간의 화면 흔들림 (0.35초 "쿵")
 * - 산산조각(USCameraShake_Shatter, 0.2초 "툭")보다 크고 느림 → 무게가 다르게 느껴짐
 * - 쿼터뷰라 위치만 흔듦. 위아래 위주 (위에서 떨어진 충격)
 * - 피했어도 흔들림: 거리에 따라 약해지게 PlayWorldCameraShake로 재생 (USGA_BossAttack)
 */
UCLASS()
class TWINSWIFT_API USCameraShake_BossImpact : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	USCameraShake_BossImpact(const FObjectInitializer& ObjectInitializer);
};
