#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "SProjectile.generated.h"

class UAbilitySystemComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UProjectileMovementComponent;
class USphereComponent;

/**
 * 투사체 (화염탄, 냉기탄 등). 종류별 차이(속도·크기·이펙트)는 이 클래스를 부모로 한 블루프린트의 설정값으로
 * - 능력이 발사할 때 만든 GE 스펙들(데미지, 표식 등)을 들고 날아가서, 적에게 한 번 닿으면 모두 적용
 *   → 스펙은 발사 순간의 공격력을 담고 있어서, 날아가는 도중 교체해도 마법사 공격력 그대로
 * - 같은 편·쏜 캐릭터는 통과, 벽에 닿으면 사라짐
 * - 오브젝트 풀을 붙일 자리: 지금은 끝나면 잠깐 뒤 Destroy. 풀(USProjectilePool)을 만들면 Destroy 대신 반납
 */
UCLASS()
class TWINSWIFT_API ASProjectile : public AActor
{
	GENERATED_BODY()

public:
	ASProjectile();

	// 발사 직전에 능력이 부름 (SpawnActorDeferred와 FinishSpawning 사이). 맞은 적에게 Specs를 차례로 적용
	void InitializeProjectile(
		UAbilitySystemComponent* InSourceASC,
		const TArray<FGameplayEffectSpecHandle>& InSpecs);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> Movement;

	// 날아가는 동안 켜져 있는 이펙트 (에셋은 블루프린트에서)
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UNiagaraComponent> FlightEffect;

	// 맞은 자리에 한 번 터지는 이펙트
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TObjectPtr<UNiagaraSystem> ImpactEffect;

	// 아무것도 안 맞으면 이 시간(초) 뒤 사라짐
	UPROPERTY(EditDefaultsOnly, Category = "Projectile", meta = (ClampMin = "0.1"))
	float MaxLifeTime = 1.5f;

private:
	// 적(Pawn)과 겹침 → 같은 편·쏜 캐릭터가 아니면 명중
	UFUNCTION()
	void OnOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	// 벽 등에 막힘 → 데미지 없이 사라짐
	UFUNCTION()
	void OnBlocked(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

	// 명중 처리는 한 번만: 적이면 스펙 적용, 이펙트, 멈추고 잠깐 뒤 사라짐 (남은 꼬리 이펙트가 자연스럽게 꺼지도록)
	void Resolve(AActor* Target, const FVector& Point);

	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;

	TArray<FGameplayEffectSpecHandle> Specs;

	bool bResolved = false;
};
