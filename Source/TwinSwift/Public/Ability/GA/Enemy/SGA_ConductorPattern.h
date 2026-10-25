#pragma once

#include "CoreMinimal.h"
#include "Ability/SGameplayAbility.h"
#include "SGA_ConductorPattern.generated.h"

class UAnimMontage;

// 지휘자 패턴 모양
UENUM(BlueprintType)
enum class EConductorPattern : uint8
{
	// 사방으로 한 바퀴 (같은 방향으로 여러 번 → 줄)
	Ring,
	// 표적 쪽 FanAngle 안에 고르게
	AimedFan,
	// 한 번 쏠 때마다 SpiralStep도씩 돌려 휘어진 팔
	Spiral
};

/**
 * 보스 2 지휘자의 탄막 패턴 (BP로 패턴마다 값만 바꿈. Asset Tags에 Ability.Boss.Conductor.*)
 * 지휘 몽타주 재생 위치로 발사 시점을 잼 (골렘 공격과 같은 원칙: 히트스톱·재생 속도와 어긋나지 않음)
 * FirstShotTime 전까지가 예고(손을 드는 동작), 쉬는 시간은 행동 트리의 Wait
 * 탄의 이동·충돌·그리기는 탄막 매니저(ASBulletManager)가 함
 */
UCLASS()
class TWINSWIFT_API USGA_ConductorPattern : public USGameplayAbility
{
	GENERATED_BODY()

public:
	USGA_ConductorPattern();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 패턴 모양
	UPROPERTY(EditDefaultsOnly, Category = "Pattern")
	EConductorPattern Pattern = EConductorPattern::Ring;

	// 지휘 몽타주. 마지막 발사가 블렌드 아웃 전에 와야 함 (아니면 발동 거부)
	UPROPERTY(EditDefaultsOnly, Category = "Pattern")
	TObjectPtr<UAnimMontage> CastMontage;

	// 한 번에 쏘는 탄 수
	UPROPERTY(EditDefaultsOnly, Category = "Pattern", meta = (ClampMin = "1"))
	int32 BulletsPerVolley = 16;

	// 몇 번 쏘는지
	UPROPERTY(EditDefaultsOnly, Category = "Pattern", meta = (ClampMin = "1"))
	int32 VolleyCount = 1;

	// 첫 발 시각(초, 몽타주 시간). 그 전까지가 예고
	UPROPERTY(EditDefaultsOnly, Category = "Pattern", meta = (ClampMin = "0"))
	float FirstShotTime = 0.5f;

	// 발사 간격(초, 몽타주 시간)
	UPROPERTY(EditDefaultsOnly, Category = "Pattern", meta = (ClampMin = "0.05"))
	float VolleyInterval = 0.15f;

	// 탄 속도(cm/초)
	UPROPERTY(EditDefaultsOnly, Category = "Pattern", meta = (ClampMin = "1"))
	float BulletSpeed = 500.f;

	// 조준 부채꼴 전체 각도(도)
	UPROPERTY(EditDefaultsOnly, Category = "Pattern", meta = (ClampMin = "0", ClampMax = "360"))
	float FanAngle = 70.f;

	// 나선: 한 번 쏠 때마다 돌리는 각도(도)
	UPROPERTY(EditDefaultsOnly, Category = "Pattern")
	float SpiralStep = 12.f;

	// 탄 1발 피해 = 보스 공격력 × 이 값
	UPROPERTY(EditDefaultsOnly, Category = "Pattern", meta = (ClampMin = "0"))
	float DamageMultiplier = 1.f;

	// 켜면 파랑 탄 (마법사가 막음), 끄면 주황 탄 (전사가 막음)
	UPROPERTY(EditDefaultsOnly, Category = "Pattern")
	bool bBlue = false;

private:
	// 짧은 주기로 호출: 몽타주 위치가 다음 발사 시각을 지났으면 발사
	void UpdatePattern();

	// 한 번 발사 (모양에 맞게 BulletsPerVolley발)
	void FireVolley();

	// 몽타주 끝·끊김·취소 → EndAbility
	UFUNCTION()
	void OnMontageEnded();

	// 이번 발동에서 다음에 쏠 번호
	int32 NextVolley = 0;

	// 발동할 때 표적 쪽 방향(도)과 표적 몸 높이 (보스 캡슐 높이로 쏘면 머리 위로 지나감)
	float StartYaw = 0.f;
	float BulletHeight = 0.f;

	FTimerHandle PatternTimer;
};
