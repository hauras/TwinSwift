#pragma once

#include "CoreMinimal.h"
#include "Ability/SGameplayAbility.h"
#include "SGA_GroundSlam.generated.h"

class UAnimMontage;
class UCurveVector;

// 내려찍기가 떨어질 곳 (착지점, 반경, 이 능력). 듣는 쪽: 거울 보스 방어 (능력은 누가 듣는지 모름)
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnGroundSlamTelegraphed, const FVector&, float, UGameplayAbility*);

/**
 * 전사 내려찍기 (키 2)
 * - 커서 위치(최대 MaxRange)로 LeapDuration 동안 포물선(최고 LeapHeight)을 그리며 날아감 (루트모션 MoveTo + 높이 커브)
 * - 몽타주의 착지 신호(Event.Montage.Hit, AN_SendEvent)가 오면 반경 Radius 안의 적을 DamageMultiplier 배율로 타격
 * - 맞은 적에게 균열 표식(Mark.Crack, CrackDuration초). 마법사 낙뢰가 터뜨림 (단계 8)
 * - 착지 후 RecoveryTime이 지나면 능력 종료 (몽타주 뒷부분의 긴 마무리 동작은 블렌드 아웃으로 자름)
 * - 몽타주가 먼저 끝나기 시작(BlendOut)·끊김·취소되어도 능력 종료
 * - 쿨다운: 기본 10초, Cooldown.Warrior.GroundSlam
 */
UCLASS()
class TWINSWIFT_API USGA_GroundSlam : public USGameplayAbility
{
	GENERATED_BODY()

public:
	USGA_GroundSlam();

	// 도약을 시작할 때 알림 (모든 내려찍기 공용. 보스가 복사한 것도 나오니 듣는 쪽이 자기 것은 거름)
	static FOnGroundSlamTelegraphed OnTelegraphed;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 최대 도약 거리(cm). 커서가 더 멀면 그 방향으로 여기까지만
	UPROPERTY(EditDefaultsOnly, Category = "Slam", meta = (ClampMin = "0"))
	float MaxRange = 800.f;

	// 착지 지점까지 날아가는 시간(초). 몽타주의 착지 신호 시점과 맞춤
	UPROPERTY(EditDefaultsOnly, Category = "Slam", meta = (ClampMin = "0.05"))
	float LeapDuration = 0.6f;

	// 도약 최고 높이(cm). 가운데에서 가장 높고 출발·착지에서 0
	UPROPERTY(EditDefaultsOnly, Category = "Slam", meta = (ClampMin = "0"))
	float LeapHeight = 150.f;

	// 타격 반경(cm)
	UPROPERTY(EditDefaultsOnly, Category = "Slam", meta = (ClampMin = "0"))
	float Radius = 300.f;

	// 착지(타격) 후 다시 움직일 수 있을 때까지(초). 0이면 몽타주 끝까지 재생
	UPROPERTY(EditDefaultsOnly, Category = "Slam", meta = (ClampMin = "0"))
	float RecoveryTime = 0.45f;

	// 데미지 배율 (공격력 × 배율)
	UPROPERTY(EditDefaultsOnly, Category = "Slam", meta = (ClampMin = "0"))
	float DamageMultiplier = 2.2f;

	// 균열 표식(Mark.Crack)이 남는 시간(초). 0이면 표식 없음
	UPROPERTY(EditDefaultsOnly, Category = "Slam", meta = (ClampMin = "0"))
	float CrackDuration = 8.f;

	// 내려찍기 몽타주. 칼이 바닥에 닿는 프레임에 AN_SendEvent(Event.Montage.Hit)
	UPROPERTY(EditDefaultsOnly, Category = "Slam|Animation")
	TObjectPtr<UAnimMontage> SlamMontage;

	// 몽타주 재생 속도 / 시작 지점(초)
	UPROPERTY(EditDefaultsOnly, Category = "Slam|Animation", meta = (ClampMin = "0.1"))
	float MontagePlayRate = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Slam|Animation", meta = (ClampMin = "0"))
	float MontageStartTime = 0.f;

private:
	// 커서 위치를 최대 거리로 자르고, 높이는 캐릭터와 같게
	FVector GetLandingLocation() const;

	// 착지 신호 → 반경 타격 + 균열 표식
	UFUNCTION()
	void OnImpact(FGameplayEventData Payload);

	// 이동이 끝남. 몽타주가 없을 때만 여기서 타격 + 종료
	UFUNCTION()
	void OnLeapFinished();

	// 몽타주 끝·끊김·취소 → EndAbility
	UFUNCTION()
	void OnMontageEnded();

	// LeapHeight로 포물선 높이 커브를 만듦 (처음 한 번, 높이를 바꾸면 다시)
	UCurveVector* GetLeapArcCurve();

	// 이번 발동에서 이미 타격했는지 (두 번 나가지 않게)
	bool bImpacted = false;

	// 포물선 높이 커브 (에셋 없이 코드로 만듦). X·Y는 0, Z만 0 → LeapHeight → 0
	UPROPERTY(Transient)
	TObjectPtr<UCurveVector> LeapArcCurve;

	// LeapArcCurve를 만들 때의 높이 (높이가 바뀌었는지 확인용)
	float LeapArcCurveHeight = -1.f;
};
