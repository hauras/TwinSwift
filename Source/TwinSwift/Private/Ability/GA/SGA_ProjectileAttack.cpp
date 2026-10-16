#include "Ability/GA/SGA_ProjectileAttack.h"

#include "AbilitySystemComponent.h"
#include "Projectile/SProjectile.h"

void USGA_ProjectileAttack::OnCast(const FVector& Origin, const FVector& Direction)
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();
	if (!ProjectileClass || !Avatar || !World || !HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	const FTransform SpawnTransform(Direction.Rotation(), Origin);

	ASProjectile* Projectile = World->SpawnActorDeferred<ASProjectile>(
		ProjectileClass,
		SpawnTransform,
		Avatar,
		Cast<APawn>(Avatar),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Projectile)
	{
		return;
	}

	// 맞으면 적용할 스펙(데미지 + 표식). 발사 순간의 공격력이 담겨서 날아가는 중에 교체해도 그대로
	Projectile->InitializeProjectile(GetAbilitySystemComponentFromActorInfo(), MakeHitSpecs());
	Projectile->FinishSpawning(SpawnTransform);
}
