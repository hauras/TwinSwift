#include "Bullet/SBenchBulletActor.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

ASBenchBulletActor::ASBenchBulletActor()
{
	BulletMesh = CreateDefaultSubobject<UStaticMeshComponent>("BulletMesh");
	BulletMesh->SetupAttachment(GetRootComponent());
	BulletMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BulletMesh->SetCastShadow(false);
}
