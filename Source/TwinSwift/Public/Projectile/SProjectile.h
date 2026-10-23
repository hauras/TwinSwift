#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "Pool/SPoolable.h"
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
 * - bPierce: 적을 관통하며 지나간 적마다 한 번씩 (교체 어시스트 검기 등)
 * - 오브젝트 풀(USActorPool)로 다시 씀: Acquire(꺼진 상태) → InitializeProjectile → (SetHomingTarget) → Launch → … → 끝나면 풀에 반납
 *   풀 없이 SpawnActor로 만든 투사체는 BeginPlay에서 바로 Launch, 끝나면 Destroy (예전 방식)
 */
UCLASS()
class TWINSWIFT_API ASProjectile : public AActor, public ISPoolable
{
	GENERATED_BODY()

public:
	ASProjectile();

	// 발사 직전에 부름 (풀: Acquire와 Launch 사이 / 직접 생성: SpawnActorDeferred와 FinishSpawning 사이). 맞은 적에게 Specs를 차례로 적용
	void InitializeProjectile(
		UAbilitySystemComponent* InSourceASC,
		const TArray<FGameplayEffectSpecHandle>& InSpecs);

	// 표적을 따라 휘어 날아가고 그 적에게만 맞음. 발사 직전에 부름 (부르지 않으면 곧게, 아무 적이나)
	void SetHomingTarget(AActor* Target);

	// 발사: 보이기, 충돌·이동·이펙트 켜기, 수명 타이머. InitializeProjectile·SetHomingTarget 다음에 부름
	void Launch();

	// 끄기: 숨기기, 충돌·이동·이펙트 끄기, 타이머 정리, 지난 발의 상태 비우기 (풀이 반납받을 때)
	void ResetForPool();

	// 아무것도 안 맞을 때 사라지기까지(초). 측정 명령이 떠 있는 탄 수를 맞출 때
	float GetMaxLifeTime() const { return MaxLifeTime; }

	// ISPoolable: 풀이 만든 투사체 표시 / 반납 때 끄기
	virtual void OnCreatedByPool() override;
	virtual void OnReleasedToPool() override;

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

	// 관통: 켜면 적에게 닿아도 멈추지 않고, 지나간 적마다 한 번씩 스펙 적용 (검기 등). 벽에서는 멈춤
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	bool bPierce = false;

	// 날아가는 동안 비행 이펙트를 이 간격(초)마다 하나 더 붙여 띄움. 0이면 처음 한 번만 (한 번 쓸고 흐려지는 반달 검기용)
	UPROPERTY(EditDefaultsOnly, Category = "Projectile", meta = (ClampMin = "0"))
	float FlightEffectRepeatInterval = 0.f;

	// 유도 세기(cm/s²). 약하면 표적 주위를 빙빙 돎
	UPROPERTY(EditDefaultsOnly, Category = "Projectile", meta = (ClampMin = "0"))
	float HomingAcceleration = 6000.f;

private:
	// 끝: 풀이 관리하면 반납, 아니면 지움 (예전 SetLifeSpan 자리)
	void ReturnToPool();

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

	// 관통 중 적 하나에 닿음: 처음 닿은 적이면 스펙 적용 + 명중 이펙트 (멈추지 않음)
	void Pierce(AActor* Target, const FVector& Point);

	// 들고 있는 스펙을 대상에게 모두 적용
	void ApplySpecsTo(AActor* Target) const;

	// FlightEffectRepeatInterval마다: 비행 이펙트와 같은 이펙트를 같은 자리에 하나 더 붙여 띄움
	void SpawnFlightEffectCopy();

	FTimerHandle FlightRepeatTimer;

	// 아무것도 안 맞았을 때 MaxLifeTime 뒤 끝
	FTimerHandle LifeTimer;

	// 명중 후 FadeOutTime 뒤 끝 (꼬리 이펙트가 꺼질 시간)
	FTimerHandle FadeTimer;

	// 풀이 만든 투사체인지. 아니면 BeginPlay에서 바로 Launch하고 끝나면 Destroy
	bool bManagedByPool = false;

	// 날아가는 중인지 (Launch ~ ResetForPool). 두 번 반납하는 것을 막음
	bool bLaunched = false;

	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;

	// 관통으로 이미 맞은 적 (한 적은 한 번만)
	TSet<TWeakObjectPtr<AActor>> PiercedActors;

	// 유도 표적 (SetHomingTarget). 있으면 이 적에게만 맞음
	TWeakObjectPtr<AActor> HomingTarget;

	TArray<FGameplayEffectSpecHandle> Specs;

	bool bResolved = false;
};
