#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Actor.h"
#include "SArcaneOrb.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UMaterialInstanceDynamic;
class UAbilitySystemComponent;
class ASBossEncounter;
struct FGameplayTag;
enum class EBossEncounterState : uint8;

/**
 * 마력 구체 (USGA_ArcaneOrb가 설치)
 * - ChargeDuration초 동안 커지며 충전 → 피해 배율 Min~Max
 * - 재입력(Explode) 또는 LifeTime초 뒤 자동 폭발: 반경 안 적에게 피해
 * - 설치한 캐릭터가 죽거나 전투가 끝나면 피해 없이 사라짐 (교체는 유지)
 */
UCLASS()
class TWINSWIFT_API ASArcaneOrb : public AActor
{
	GENERATED_BODY()

public:
	ASArcaneOrb();

	// 반경 안 적에게 충전 배율 피해 후 사라짐 (한 번만)
	void Explode();

	// 피해 없이 사라짐
	void Cancel();

	// 지연 생성 중(FinishSpawning 전)에 부름: 피해를 보낼 ASC와 원본 스펙
	void Initialize(UAbilitySystemComponent* InSourceASC, const FGameplayEffectSpecHandle& InDamageSpec);

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, Category = "Orb")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Orb")
	TObjectPtr<UStaticMeshComponent> OrbMesh;

	// 이펙트가 있으면 메시는 숨기고 이펙트로 보여 줌
	UPROPERTY(VisibleAnywhere, Category = "Orb")
	TObjectPtr<UNiagaraComponent> OrbEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Orb|Effects")
	TObjectPtr<UNiagaraSystem> ExplosionEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Orb|Effects", meta = (ClampMin = "0"))
	float ExplosionEffectScale = 1.f;

	// 피해 반경을 그대로 보여 주는 바닥 원 (구체 충전 크기와 별개)
	UPROPERTY(VisibleAnywhere, Category = "Orb|Range")
	TObjectPtr<UStaticMeshComponent> RangeMesh;

	// 실제 체력 피해를 준 대상에게만 재생
	UPROPERTY(EditDefaultsOnly, Category = "Orb|Effects")
	TObjectPtr<UNiagaraSystem> HitEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Orb|Effects", meta = (ClampMin = "0"))
	float HitEffectScale = 1.f;

	// 최대 충전까지 걸리는 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "Orb")
	float ChargeDuration = 1.f;

	// 설치 후 자동 폭발까지(초)
	UPROPERTY(EditDefaultsOnly, Category = "Orb")
	float LifeTime = 4.f;

	// 폭발 반경(cm)
	UPROPERTY(EditDefaultsOnly, Category = "Orb")
	float ExplosionRadius = 300.f;

	// 바로 터뜨렸을 때 피해 배율
	UPROPERTY(EditDefaultsOnly, Category = "Orb|Damage", meta = (ClampMin = "0"))
	float MinDamageMultiplier = 1.f;

	// 최대 충전 상태의 피해 배율
	UPROPERTY(EditDefaultsOnly, Category = "Orb|Damage", meta = (ClampMin = "0"))
	float MaxDamageMultiplier = 2.f;

private:
	void SetupRangeIndicator();

	// 바닥 원과 같은 지름까지 퍼지는 독립 파동 (충전량은 밝기에만 반영)
	void SpawnExplosionWave(float ChargeRatio);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RangeMaterial;

	// 충전에 맞춰 크기를 키움 (다 차면 타이머 멈춤)
	void UpdateChargeVisual();

	// 0~1 충전량
	float GetChargeRatio() const;

	// 반경 안 맞힐 수 있는 대상 (한 대상은 한 번만)
	void FindTargets(TArray<AActor*>& OutTargets) const;

	// 교체는 유지하되 설치한 캐릭터 사망·전투 종료 시 피해 없이 제거
	void OnSourceDead(FGameplayTag Tag, int32 NewCount);
	void OnEncounterChanged(EBossEncounterState State);

	double SpawnTime = 0.0;

	// 폭발·취소를 이미 했는지 (두 번 처리 막음)
	bool bResolved = false;

	FTimerHandle ExplosionTimer;
	FTimerHandle ChargeVisualTimer;

	// 충전 0일 때 크기의 기준 (BP에서 정한 크기)
	FVector BaseEffectScale = FVector::OneVector;
	FVector BaseMeshScale = FVector::OneVector;

	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;

	// 설치 때 받은 원본 스펙. 폭발 때 대상마다 복사해서 배율만 넣음
	FGameplayEffectSpecHandle DamageSpec;

	FDelegateHandle SourceDeadHandle;
	FDelegateHandle EncounterHandle;
	TWeakObjectPtr<ASBossEncounter> Encounter;
};
