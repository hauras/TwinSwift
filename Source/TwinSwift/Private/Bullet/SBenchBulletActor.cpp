#include "Bullet/SBenchBulletActor.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

ASBenchBulletActor::ASBenchBulletActor()
{
	BulletMesh = CreateDefaultSubobject<UStaticMeshComponent>("BulletMesh");
	BulletMesh->SetupAttachment(GetRootComponent());
	BulletMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BulletMesh->SetCastShadow(false);

	// 탄끼리 막지 않게: 투사체는 WorldDynamic이면서 WorldDynamic을 막아서, 한 점에서 나선으로 쏘면 방금 나간 탄에 다음 탄이 부딪혀 사라짐
	// (2026-10-27 단독 측정에서 목표 1000/2000/3000발 중 150/280/400발만 떠 있었음). 탄막 매니저 탄도 서로 안 막으므로 같은 조건으로
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
}
