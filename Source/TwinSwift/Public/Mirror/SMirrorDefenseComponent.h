#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SMirrorDefenseComponent.generated.h"

class ASMirrorBoss;
class UGameplayAbility;
class USGA_Dodge;
struct FGameplayAbilitySpecHandle;

// 공격 예고를 받아 방어를 판단하고, 기존 회피 능력을 실행하는 컴포넌트.
UCLASS(ClassGroup = (Mirror), meta = (BlueprintSpawnableComponent))
class TWINSWIFT_API USMirrorDefenseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USMirrorDefenseComponent();

	// 공격 예고. SourceAbility가 끝났으면 예약된 회피도 하지 않음.
	void NotifyIncomingAttack(
		const FVector& AttackCenter,
		float AttackRadius,
		UGameplayAbility* SourceAbility = nullptr);

	// 예약 중·실제 회피 중에는 복사 공격과 교체가 끼어들지 않게 함.
	bool IsDefending() const;

	// 전투 종료·그로기·사망에서 예약과 남은 방향을 정리.
	void CancelPendingEvade();

	// 위협(Center)에서 지금보다 멀어지는 안전한 방향으로 바로 회피 (마법사가 붙은 플레이어에게서 점멸로 거리 벌리기)
	// 회피를 못 하거나(쿨다운 등) 안전한 방향이 없으면 false
	bool TryEvadeFrom(const FVector& Center);

protected:
	// 내려찍기 알림(USGA_GroundSlam::OnTelegraphed)에 묶음 / 풂
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "Defense", meta = (ClampMin = "0.01"))
	float ReactionDelay = 0.2f;

	// 공격 범위 밖으로 이만큼 더 여유를 둠(cm).
	UPROPERTY(EditDefaultsOnly, Category = "Defense", meta = (ClampMin = "0"))
	float SafetyMargin = 35.f;

private:
	// 내려찍기 알림을 받음. 보스 자신이 복사해 쓴 내려찍기는 거름
	void HandleGroundSlam(const FVector& LandingLocation, float Radius, UGameplayAbility* SourceAbility);

	void TryEvade();
	bool CanPrepareEvade(const ASMirrorBoss* Boss) const;
	bool IsInsideAttack(const ASMirrorBoss* Boss) const;
	const USGA_Dodge* FindDodgeAbility(
		const ASMirrorBoss* Boss,
		FGameplayAbilitySpecHandle& OutHandle) const;
	// Center에서 DangerRadius(+ 캡슐·여유) 밖으로 나가는 방향 중 벽·낭떠러지 없는 쪽
	bool FindSafeDirection(
		const ASMirrorBoss* Boss,
		const FVector& Center,
		float DangerRadius,
		float DodgeDistance,
		FVector& OutDirection) const;

	// 방향을 넣고 회피 능력 실행 (방향은 ActivateAbility 안에서 읽어서 바로 비움)
	bool ActivateDodge(ASMirrorBoss* Boss, const FGameplayAbilitySpecHandle& Handle, const USGA_Dodge* Dodge, const FVector& Direction);

	bool bEvadePending = false;
	bool bTrackSourceAbility = false;
	FVector PendingAttackCenter = FVector::ZeroVector;
	float PendingAttackRadius = 0.f;
	TWeakObjectPtr<UGameplayAbility> PendingSourceAbility;
	FTimerHandle EvadeTimer;

	FDelegateHandle GroundSlamHandle;
};
