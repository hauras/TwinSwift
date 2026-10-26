#pragma once

#include "CoreMinimal.h"
#include "Ability/SGameplayAbility.h"
#include "Bullet/SBulletPatternData.h"
#include "SGA_ConductorPattern.generated.h"

class UAnimMontage;
class UMaterialInterface;

/**
 * 보스 2 지휘자의 탄막 패턴 (BP마다 패턴 데이터만 바꿈. Asset Tags에 Ability.Boss.Conductor.*)
 * 패턴 내용(언제·어떤 모양·몇 발·색)은 데이터 에셋(USBulletPatternData). 능력은 펼친 발사 목록을 커서로 차례대로 실행
 * 지휘 몽타주 재생 위치로 발사 시점을 잼 (골렘 공격과 같은 원칙: 히트스톱·재생 속도와 어긋나지 않음)
 * 첫 발 전까지가 예고(손을 드는 동작), 쉬는 시간은 행동 트리의 Wait
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
	// 지휘 몽타주. 패턴의 마지막 발사가 블렌드 아웃 전에 와야 함 (아니면 발동 거부)
	UPROPERTY(EditDefaultsOnly, Category = "Pattern")
	TObjectPtr<UAnimMontage> CastMontage;

	// 패턴 내용 (DA_Conductor_*). 능력 BP의 기본값이라 EditDefaultsOnly (데이터 에셋 안의 칸은 EditAnywhere)
	UPROPERTY(EditDefaultsOnly, Category = "Pattern")
	TObjectPtr<USBulletPatternData> PatternData;

	// 무작위 색 패턴의 예고: 손을 모으는 동안 보스 몸 위에 그 색으로 한 겹 더 그림 (오버레이 재질). 비우면 예고 없음
	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Telegraph")
	TObjectPtr<UMaterialInterface> OrangeTelegraphMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Pattern|Telegraph")
	TObjectPtr<UMaterialInterface> BlueTelegraphMaterial;

private:
	// 짧은 주기로 호출: 몽타주 위치가 커서 칸의 발사 시각을 지났으면 쏘고 커서를 옮김
	void UpdatePattern();

	// 발사 한 번: 지시의 모양대로 탄 수만큼 방향을 나눠 탄막 매니저에 넣음
	// 어떤 시계로 재는지와 상관없게 둠 → 나중에 능력 없이 쏘는 액터(악단 포탑)가 생기면 떼어 쓸 수 있게
	void FireShot(const FSBulletShot& Shot);

	// 보스 몸의 색 예고 켜기/끄기. 끌 때는 이 능력이 씌운 재질일 때만 지움 (다른 오버레이를 지우지 않게)
	void ShowColorTelegraph();
	void HideColorTelegraph();

	// 몽타주 끝·끊김·취소 → EndAbility
	UFUNCTION()
	void OnMontageEnded();

	// 이번 발동의 펼친 발사 목록 (시간순 정렬) / 다음에 쏠 칸 (커서)
	TArray<FSBulletShot> Shots;
	int32 NextShot = 0;

	// 다시 조준하는 지시(bReaim)가 쓸 표적 (날아가는 동안 사라질 수 있어 약한 포인터)
	TWeakObjectPtr<const AActor> TargetActor;

	// 발동할 때 표적 쪽 방향(도)과 표적 몸 높이 (보스 캡슐 높이로 쏘면 머리 위로 지나감)
	float StartYaw = 0.f;
	float BulletHeight = 0.f;

	// 이번 발동의 무작위 색 (bRandomColor 지시가 씀). 발동마다 다시 뽑음
	EBulletColor RolledColor = EBulletColor::Orange;

	FTimerHandle PatternTimer;
};
