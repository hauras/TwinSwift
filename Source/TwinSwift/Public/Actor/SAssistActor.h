#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "SAssistActor.generated.h"

class ASProjectile;
class UAbilitySystemComponent;
class UAnimSequenceBase;
class UGameplayEffect;
class UMaterialInterface;
class UNiagaraSystem;
class USkeletalMeshComponent;
class UStaticMeshComponent;

/**
 * 교체 어시스트 분신: 나가는 캐릭터가 교체한 자리에 반투명하게 남아 한 번 공격하고 사라짐
 * - 정상 교체(SwapTo)에만 생성, 사망으로 인한 강제 교체는 제외
 * - 표식을 걸거나 터뜨리지 않음 (표식 소비는 직접 쓰는 공격에 맡김)
 */
UCLASS()
class TWINSWIFT_API ASAssistActor : public AActor
{
	GENERATED_BODY()

public:
	ASAssistActor();

	// 교체 코드가 생성 직후 부름. 나가는 캐릭터의 ASC(공격력·팀)와 겨냥 방향(커서 쪽, 수평)을 받아 애니 시작 + 타격·사라짐 예약
	void StartAssist(UAbilitySystemComponent* InSourceASC, const FVector& InAimDirection);

protected:
	// 분신 몸 (루트). 메시는 BP에서 지정
	UPROPERTY(VisibleAnywhere, Category = "Assist")
	TObjectPtr<USkeletalMeshComponent> Body;

	// 무기. Body의 WeaponSocket에 붙음. 마법사는 비워 둠
	UPROPERTY(VisibleAnywhere, Category = "Assist")
	TObjectPtr<UStaticMeshComponent> Weapon;

	// 분신이 재생할 애니
	UPROPERTY(EditDefaultsOnly, Category = "Assist")
	TObjectPtr<UAnimSequenceBase> AssistAnimation;

	// 애니를 이 시간(초)부터 재생. 긴 준비 동작을 건너뛸 때 (HitTimes·LifeTime은 생성 순간부터 잼)
	UPROPERTY(EditDefaultsOnly, Category = "Assist", meta = (ClampMin = "0"))
	float AnimStartTime = 0.f;

	// 애니 시작부터 각 타격까지(초). 휘두르는 횟수만큼 (전사 회전 베기: 0.5, 1.15)
	UPROPERTY(EditDefaultsOnly, Category = "Assist")
	TArray<float> HitTimes = { 0.3f };

	// 타격 반경(cm)
	UPROPERTY(EditDefaultsOnly, Category = "Assist", meta = (ClampMin = "0"))
	float Radius = 250.f;

	// 한 번 맞을 때의 데미지 배율 (총 피해 = 배율 × 타격 횟수)
	UPROPERTY(EditDefaultsOnly, Category = "Assist", meta = (ClampMin = "0"))
	float DamageMultiplier = 1.2f;

	// 데미지 GE (생성자에서 USGE_Damage)
	UPROPERTY(EditDefaultsOnly, Category = "Assist")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// 있으면 타격마다 반경 판정 대신 이 투사체를 겨냥 방향으로 발사 (전사 검기). 비우면 반경 타격 (마법사 냉기 장판)
	UPROPERTY(EditDefaultsOnly, Category = "Assist")
	TSubclassOf<ASProjectile> ProjectileClass;

	// 투사체 발사 높이(cm, 발밑 기준)
	UPROPERTY(EditDefaultsOnly, Category = "Assist", meta = (EditCondition = "ProjectileClass != nullptr"))
	float ProjectileHeight = 90.f;

	// 켜면 발마다 가까운 적을 표적으로 유도 (마법사 마탄). 끄면 겨냥 방향으로 곧게 (전사 검기)
	UPROPERTY(EditDefaultsOnly, Category = "Assist", meta = (EditCondition = "ProjectileClass != nullptr"))
	bool bHomingToNearest = false;

	// 유도 표적을 찾는 반경(cm). 안에 적이 없으면 겨냥 방향으로 곧게
	UPROPERTY(EditDefaultsOnly, Category = "Assist", meta = (EditCondition = "bHomingToNearest", ClampMin = "0"))
	float HomingSearchRadius = 1200.f;

	// 유도 발사의 부채꼴 반각(도). 3발이면 -30, 0, +30도로 퍼졌다가 표적으로 휘어 들어감
	UPROPERTY(EditDefaultsOnly, Category = "Assist", meta = (EditCondition = "bHomingToNearest", ClampMin = "0", ClampMax = "90"))
	float HomingFanAngle = 30.f;

	// 타격 순간 이펙트 (마법사는 냉기 장판)
	UPROPERTY(EditDefaultsOnly, Category = "Assist")
	TObjectPtr<UNiagaraSystem> HitEffect;

	// 분신이 남아 있는 시간(초). 지나면 사라짐 (최소 마지막 타격 + 0.1초)
	UPROPERTY(EditDefaultsOnly, Category = "Assist", meta = (ClampMin = "0"))
	float LifeTime = 0.8f;

	// 반투명 분신 머티리얼. 몸과 무기의 재질 칸을 모두 이것으로 바꿈
	UPROPERTY(EditDefaultsOnly, Category = "Assist")
	TObjectPtr<UMaterialInterface> GhostMaterial;

private:
	// 다음 타격(SortedHitTimes[NextHitIndex])을 예약. 남은 타격이 없으면 아무것도 안 함
	void ScheduleNextHit();

	// 타격 타이머가 불림: 타격하고 다음 타격 예약
	void OnHitTimer();

	// 타격 한 번: 이펙트 + (투사체가 있으면 발사, 없으면 반경 피해)
	void DoHit();

	// 반경 안의 적에게 피해
	void HitInRadius(UAbilitySystemComponent* ASC);

	// Center에서 SearchRadius 안의 적 모두 (중복·같은 편·ASC 없는 것 제외). HitInRadius와 FindNearestEnemies가 같이 씀
	void GatherEnemies(const FVector& Center, float SearchRadius, TArray<AActor*>& OutEnemies) const;

	// Center에서 SearchRadius 안의 적 중 가까운 순서로 최대 Count명 (가까운 적이 앞)
	// 전부 정렬하지 않고 크기 Count인 최대 힙으로 O(n log k)
	void FindNearestEnemies(
		const FVector& Center,
		float SearchRadius,
		int32 Count,
		TArray<AActor*>& OutTargets) const;

	// 겨냥 방향으로 투사체 발사 (나가는 캐릭터의 공격력 스펙을 들고 감)
	void FireProjectile(UAbilitySystemComponent* ASC);

	// 데미지 스펙 (나가는 캐릭터의 ASC로 → 그 캐릭터의 공격력, 데미지 숫자도 그대로)
	FGameplayEffectSpecHandle MakeDamageSpec(UAbilitySystemComponent* ASC) const;

	// 나가는 캐릭터의 ASC (공격력·팀·데미지 숫자). 분신이 사는 동안 플레이어가 사라질 수 있어 약한 포인터
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;

	// 겨냥 방향 (수평 단위 벡터). 교체할 때의 커서 쪽
	FVector AimDirection = FVector::ForwardVector;

	// 타격 타이머 (한 번에 다음 타격 하나만 예약)
	FTimerHandle HitTimer;

	// HitTimes를 빠른 순으로 정리한 것 (BP에서 순서 없이 넣어도 됨)
	TArray<float> SortedHitTimes;

	// 다음에 할 타격 번호
	int32 NextHitIndex = 0;

	// 이번 발의 유도 표적. 첫 발에서 AssignShotTargets로 발마다 표적을 정해 둠
	AActor* PickHomingTarget();

	// 발마다 표적 정하기 (예상 피해 배분): 가까운 적부터 지금 체력이 0이 될 만큼만, 남은 탄은 다음 적
	void AssignShotTargets();

	// 발 번호 → 부채꼴 각도(도)
	float GetFanAngle(int32 ShotIndex) const;

	// 발 번호 → 표적 (같은 적이 여러 번 나올 수 있음). 날아가는 동안 사라질 수 있어 약한 포인터
	TArray<TWeakObjectPtr<AActor>> ShotTargets;

	// 표적을 이미 골랐는지 (첫 발에서 한 번만)
	bool bHomingTargetsChosen = false;
};
