#pragma once

#include "CoreMinimal.h"
#include "Ability/SGameplayAbility.h"
#include "SGA_CastAbility.generated.h"

class UAnimMontage;
class UNiagaraSystem;

/**
 * 마법사 시전 능력의 공통 부모 (화염탄·얼음 쇄도·낙뢰)
 * - 커서 쪽으로 돌고 몽타주 재생 → 손을 뻗는 프레임의 신호(AN_SendEvent, Event.Montage.Hit)에 OnCast
 * - OnCast에는 손(MuzzleBone) 위치와 손 → 커서 방향(수평)을 넘김. 자식은 "신호가 오면 무엇을 하나"만 씀
 * - 시전 후 RecoveryTime이 지나면 다시 움직일 수 있음. 자식의 효과(IsCastFinished)도 끝나면 능력 종료
 *   (얼음 쇄도처럼 효과가 오래가도 캐릭터는 RecoveryTime 뒤 움직임)
 * - 화염탄 중에 다른 시전 능력을 누르면 화염탄을 끊고 시전
 */
UCLASS(Abstract)
class TWINSWIFT_API USGA_CastAbility : public USGameplayAbility
{
	GENERATED_BODY()

public:
	USGA_CastAbility();

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
	// 데미지 배율 (공격력 × 배율)
	UPROPERTY(EditDefaultsOnly, Category = "Cast", meta = (ClampMin = "0"))
	float DamageMultiplier = 1.f;

	// 맞은 적에게 거는 표식 (예: Mark.Frost). 비우면 표식 없음
	UPROPERTY(EditDefaultsOnly, Category = "Cast", meta = (Categories = "Mark"))
	FGameplayTag MarkTag;

	// 표식이 남는 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "Cast", meta = (ClampMin = "0"))
	float MarkDuration = 6.f;

	// 시전이 나가는 뼈 (손)
	UPROPERTY(EditDefaultsOnly, Category = "Cast")
	FName MuzzleBone = "hand_R";

	// 손에서 한 번 터지는 시전 이펙트 (손 → 커서 방향으로 생성)
	UPROPERTY(EditDefaultsOnly, Category = "Cast")
	TObjectPtr<UNiagaraSystem> LaunchEffect;

	// 시전 후 다시 움직이거나 다음 공격을 할 수 있을 때까지(초)
	UPROPERTY(EditDefaultsOnly, Category = "Cast", meta = (ClampMin = "0"))
	float RecoveryTime = 0.2f;

	// 시전 몽타주. 손을 뻗는 프레임에 AN_SendEvent(Event.Montage.Hit). 비우면 누르자마자 시전
	UPROPERTY(EditDefaultsOnly, Category = "Cast|Animation")
	TObjectPtr<UAnimMontage> CastMontage;

	// 몽타주 재생 속도 / 시작 지점(초)
	UPROPERTY(EditDefaultsOnly, Category = "Cast|Animation", meta = (ClampMin = "0.1"))
	float MontagePlayRate = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Cast|Animation", meta = (ClampMin = "0"))
	float MontageStartTime = 0.f;

	// 자식이 쓰는 곳: 발사 신호가 왔을 때. Origin = 손 조금 앞, Direction = 손 → 커서 (수평, 길이 1)
	virtual void OnCast(const FVector& Origin, const FVector& Direction) {}

	// 자식 효과가 아직 진행 중이면 false (예: 얼음 쇄도 판정). 끝나면 TryFinishCast를 불러 줄 것
	virtual bool IsCastFinished() const { return true; }

	// 시전 후 대기와 자식 효과가 모두 끝났으면 능력 종료
	void TryFinishCast();

	// 맞으면 적용할 스펙: 데미지 + (MarkTag가 있으면) 표식. 발사 순간의 공격력이 담김
	TArray<FGameplayEffectSpecHandle> MakeHitSpecs() const;

	// 대상 하나에 표식만 걸기 (MarkTag가 비었으면 아무것도 안 함)
	void ApplyMarkTo(AActor* Target) const;

private:
	// 발사 신호 → 손·방향 계산 → OnCast + RecoveryTime 대기 시작
	UFUNCTION()
	void OnCastEvent(FGameplayEventData Payload);

	// 시전 후 대기 끝 → 이동 막기를 풀고 종료 시도
	UFUNCTION()
	void OnRecoveryFinished();

	// 몽타주가 끝나기 시작 → 시전 전이면 종료(신호를 못 받음), 시전 후면 종료 시도
	UFUNCTION()
	void OnMontageBlendOut();

	// 몽타주가 끊기거나 취소됨 → 종료
	UFUNCTION()
	void OnMontageInterrupted();

	// MarkTag·MarkDuration으로 표식 스펙 (없으면 빈 핸들)
	FGameplayEffectSpecHandle MakeMarkSpec() const;

	// 손 위치와 손 → 커서 방향 계산
	void ComputeAim(FVector& OutOrigin, FVector& OutDirection) const;

	// 시전 중 이동 막기 (붙인 경우에만 한 번 뗌 → Loose 태그가 쌓이지 않음)
	void SetAttacking(bool bAttacking);

	bool bCastFired = false;
	bool bRecoveryDone = false;
	bool bAttackingTagAdded = false;
};
