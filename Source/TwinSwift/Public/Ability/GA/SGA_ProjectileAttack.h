#pragma once

#include "CoreMinimal.h"
#include "Ability/GA/SGA_CastAbility.h"
#include "SGA_ProjectileAttack.generated.h"

class ASProjectile;

/**
 * 투사체 발사 (화염탄)
 * - 몽타주·발사 신호·손 → 커서 방향·끝나는 처리는 부모(USGA_CastAbility)
 * - 여기서는 신호가 오면 투사체를 만들고 데미지(+ 표식) 스펙을 넘기는 것만
 * 예: 화염탄 = 입력 Attack, 쿨 0.45초, BP_Projectile_FireBolt
 */
UCLASS()
class TWINSWIFT_API USGA_ProjectileAttack : public USGA_CastAbility
{
	GENERATED_BODY()

protected:
	// 쏠 투사체 (ASProjectile을 부모로 한 블루프린트: 속도·크기·이펙트)
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<ASProjectile> ProjectileClass;

	// 손 위치에서 커서 방향으로 투사체 생성
	virtual void OnCast(const FVector& Origin, const FVector& Direction) override;
};
