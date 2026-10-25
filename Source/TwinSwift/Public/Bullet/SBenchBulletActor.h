#pragma once

#include "CoreMinimal.h"
#include "Projectile/SProjectile.h"
#include "SBenchBulletActor.generated.h"

class UStaticMeshComponent;

/**
 * [측정 전용] 탄막 매니저와 같은 외형의 액터 탄: 구 메시 하나, 나이아가라 없음
 * BenchBullets의 액터 방식이 이걸 써서 "액터 하나씩 vs 배열" 구조 차이만 비교 (화염탄은 궤적 이펙트·속도가 달라 섞임)
 * 메시는 명중·반납 때 ASProjectile이 숨기고, 발사 때 다시 보이게 함 (붙은 정적 메시 전부)
 */
UCLASS()
class TWINSWIFT_API ASBenchBulletActor : public ASProjectile
{
	GENERATED_BODY()

public:
	ASBenchBulletActor();

protected:
	// 탄 모양 (BP에서 엔진 구 + M_Bullet). 충돌·그림자 없음 → 판정은 부모의 Collision 하나
	UPROPERTY(VisibleAnywhere, Category = "Bench")
	TObjectPtr<UStaticMeshComponent> BulletMesh;
};
