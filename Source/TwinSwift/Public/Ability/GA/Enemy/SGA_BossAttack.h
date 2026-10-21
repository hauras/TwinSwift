#pragma once

#include "CoreMinimal.h"
#include "Ability/SGameplayAbility.h"
#include "SGA_BossAttack.generated.h"

class ASBossTelegraph;
class UAnimMontage;
class UCameraShakeBase;
class UNiagaraSystem;

// 공격 범위 모양
UENUM()
enum class EBossAttackShape : uint8
{
	// 원: 중심에서 Radius 안
	Circle,
	// 부채꼴: 정면 기준 좌우 FanAngle/2 안, Radius 안
	Fan,
};

/**
 * 보스 공통 근접 공격: 표적을 보고 → 몽타주 재생 → 예고 → HitTimes마다 범위 피해
 * 패턴(내려치기·휩쓸기 …)은 이 클래스의 BP로 만들고 값만 바꿈. Asset Tags에 패턴 태그(Ability.Boss.*)를 넣으면 BT가 그 태그로 발동
 * 맞는 시간을 몽타주 Notify가 아니라 숫자(HitTimes)로 둔 이유: 예고 표시가 "언제 맞는지"를 미리 알아야 함 → 예고와 판정이 같은 숫자를 씀
 * 공격 시계 = 몽타주 재생 위치: 히트스톱(CustomTimeDilation)·재생 속도가 이미 반영돼 모션과 판정·예고가 어긋나지 않음
 *   (월드 시간 타이머로 재면 히트스톱 동안에도 판정 시간이 흘러 주먹보다 피해가 먼저 나감)
 * 작은 경직(State.Stun)은 진행 중인 공격을 끊지 않음 = 슈퍼아머. 큰 경직(State.Groggy)은 ABossCharacter가 취소
 * 단계 9-1: 예고는 디버그 원. 바닥 표시는 9-2
 */
UCLASS()
class TWINSWIFT_API USGA_BossAttack : public USGameplayAbility
{
	GENERATED_BODY()

public:
	USGA_BossAttack();

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
	// 공격 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "Boss Attack|Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	// 몽타주 재생 속도. 무거운 보스는 1보다 느리게 → 예고 시간이 길어짐
	UPROPERTY(EditDefaultsOnly, Category = "Boss Attack|Animation", meta = (ClampMin = "0.1"))
	float MontagePlayRate = 1.f;

	// 맞는 순간들(초, 몽타주 시간 기준 = 재생 속도 1일 때). 2번 치면 2개, 작은 순서대로
	// 몽타주 블렌드 아웃이 시작되면 능력이 끝나므로 (몽타주 길이 - 블렌드 아웃 시간)보다 앞이어야 함
	UPROPERTY(EditDefaultsOnly, Category = "Boss Attack|Hit")
	TArray<float> HitTimes;

	// 범위 모양
	UPROPERTY(EditDefaultsOnly, Category = "Boss Attack|Hit")
	EBossAttackShape Shape = EBossAttackShape::Fan;

	// 범위 반경(cm)
	UPROPERTY(EditDefaultsOnly, Category = "Boss Attack|Hit", meta = (ClampMin = "0"))
	float Radius = 400.f;

	// 부채꼴 전체 각도(도). 180이면 앞쪽 반원
	UPROPERTY(EditDefaultsOnly, Category = "Boss Attack|Hit", meta = (ClampMin = "1", ClampMax = "360", EditCondition = "Shape == EBossAttackShape::Fan"))
	float FanAngle = 90.f;

	// 범위 중심을 보스 앞으로 얼마나 옮길지(cm). 원형 내려치기를 주먹 위치에 맞출 때
	UPROPERTY(EditDefaultsOnly, Category = "Boss Attack|Hit")
	float ForwardOffset = 0.f;

	// 데미지 배율 (보스 공격력 × 배율), 타격마다 같음
	UPROPERTY(EditDefaultsOnly, Category = "Boss Attack|Hit", meta = (ClampMin = "0"))
	float DamageMultiplier = 2.f;

	// 맞은 플레이어 경직 시간(초). 0이면 없음. 경직 중엔 이동·공격·회피 불가, 교체로만 빠져나옴 (경직은 캐릭터마다 따로)
	// 무적(회피·완벽 회피)으로 피한 경우는 걸지 않음
	UPROPERTY(EditDefaultsOnly, Category = "Boss Attack|Hit", meta = (ClampMin = "0"))
	float HitStunDuration = 0.f;

	// 표적에게 뛰어듦: 발동할 때 착지 지점을 정하고(예고도 그 자리에 고정) LeapStartTime~LeapEndTime 동안 캡슐을 옮김
	// 애니는 제자리 버전(앞뒤 이동 0)을 써야 함. 착지 지점 = 표적 - 앞쪽 ForwardOffset (손이 몸 앞에 떨어지므로)
	UPROPERTY(EditDefaultsOnly, Category = "Boss Attack|Leap")
	bool bLeapToTarget = false;

	// 이동 시작·끝 (초, 몽타주 시간 기준). 원본 애니에서 골반이 앞으로 가는 구간을 재서 넣음
	UPROPERTY(EditDefaultsOnly, Category = "Boss Attack|Leap", meta = (EditCondition = "bLeapToTarget", ClampMin = "0"))
	float LeapStartTime = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss Attack|Leap", meta = (EditCondition = "bLeapToTarget", ClampMin = "0"))
	float LeapEndTime = 0.5f;

	// 최대 이동 거리(cm). 표적이 더 멀면 그 방향으로 여기까지만
	UPROPERTY(EditDefaultsOnly, Category = "Boss Attack|Leap", meta = (EditCondition = "bLeapToTarget", ClampMin = "0"))
	float MaxLeapDistance = 800.f;

	// 맞는 순간 범위 중심에 생기는 이펙트 (나이아가라 풀 사용). 비우면 없음
	UPROPERTY(EditDefaultsOnly, Category = "Boss Attack|Feedback")
	TObjectPtr<UNiagaraSystem> ImpactEffect;

	// 이펙트 크기 배율 (범위 크기에 맞출 때)
	UPROPERTY(EditDefaultsOnly, Category = "Boss Attack|Feedback", meta = (ClampMin = "0.1"))
	float ImpactEffectScale = 1.f;

	// 맞는 순간 화면 흔들림 (기본 USCameraShake_BossImpact). 피했어도 흔들림, 범위 밖은 거리에 따라 약해짐
	UPROPERTY(EditDefaultsOnly, Category = "Boss Attack|Feedback")
	TSubclassOf<UCameraShakeBase> ImpactCameraShake;

	// 이 거리(cm)부터 흔들림이 0. 범위 안은 최대 세기
	UPROPERTY(EditDefaultsOnly, Category = "Boss Attack|Feedback", meta = (ClampMin = "0"))
	float ImpactShakeOuterRadius = 2000.f;

	// 표적을 적어 둔 블랙보드 키 (BTService_FindNearestPlayer가 씀)
	UPROPERTY(EditDefaultsOnly, Category = "Boss Attack|Target")
	FName TargetKeyName = TEXT("TargetActor");

	// 바닥 예고 (BP_BossTelegraph). 발동할 때 풀에서 꺼내 범위 모양으로 → 공격 시계로 차오름 → 마지막 타격에 번쩍하고 반납
	UPROPERTY(EditDefaultsOnly, Category = "Boss Attack|Telegraph")
	TSubclassOf<ASBossTelegraph> TelegraphClass;

	// [디버그] 판정 범위를 디버그 선으로도 그림 (바닥 예고와 실제 판정이 맞는지 비교할 때)
	UPROPERTY(EditDefaultsOnly, Category = "Boss Attack|Debug")
	bool bDrawDebugTelegraph = false;

private:
	// 블랙보드의 표적. 없으면 nullptr
	AActor* GetTargetActor() const;

	// 공격 시계(초, 몽타주 시간 기준). 몽타주 재생 위치. 몽타주가 없으면 발동 후 월드 시간 × 재생 속도
	float GetAttackTime() const;

	// 짧은 주기로 호출: 공격 시계가 HitTimes를 지났으면 타격, 예고 그리기, 다 쳤으면(몽타주 없을 때) 종료
	void OnAttackTick();

	// 범위 안의 적에게 피해 + 이펙트·흔들림
	void DoHit();

	// Target이 지금 범위 안인지 (높이 무시)
	bool IsInShape(const AActor* Target) const;

	// 범위 중심 (보스 위치 + 앞으로 ForwardOffset, 발밑 높이). 도약 공격은 발동할 때 정한 착지 쪽 지점
	FVector GetShapeCenter() const;

	// 도약: 착지 지점·범위 중심을 정함 (발동할 때) / 이동 시작 (공격 시계가 LeapStartTime을 지날 때)
	void PlanLeap(const AActor* Target);
	void StartLeap();

	// 지난 타격(첫 타격이면 0초)부터 다음 타격까지 몇 % 왔는지 (예고가 가득 = 맞는 순간). 바닥 예고와 디버그 선이 같이 씀
	float GetTelegraphProgress(float AttackTime) const;

	// 바닥 예고 꺼내기 (발동할 때) / 넣기 (마지막 타격이면 번쩍 후, 취소면 바로)
	void ShowTelegraph();
	void HideTelegraph(bool bFlash);

	// [디버그] 예고 선 그리기. 안쪽 원이 바깥에 닿는 순간 맞음
	void DrawTelegraph(float AttackTime) const;

	// 범위 모양을 디버그 선으로 그림 (원 또는 부채꼴). 예고와 맞는 순간 표시가 같이 씀
	void DrawArea(float InRadius, const FColor& Color, float Thickness, float Duration) const;

	// 몽타주 끝·끊김·취소 → EndAbility
	UFUNCTION()
	void OnMontageEnded();

	// 이번 발동에서 다음에 칠 HitTimes 번호
	int32 HitIndex = 0;

	// 발동 시각 (월드 시간). 몽타주가 없을 때만 공격 시계로 씀
	double ActivateTime = 0.0;

	// OnAttackTick 반복 타이머
	FTimerHandle AttackTickTimer;

	// 이번 발동의 바닥 예고 (풀이 주인. 반납 뒤 다른 공격이 쓸 수 있어 약한 포인터 + 반납하면 바로 비움)
	TWeakObjectPtr<ASBossTelegraph> Telegraph;

	// 도약 상태 (발동마다 처음부터)
	bool bLeapStarted = false;
	bool bHasLockedCenter = false;
	FVector LeapDestination = FVector::ZeroVector;
	FVector LockedCenter = FVector::ZeroVector;
};