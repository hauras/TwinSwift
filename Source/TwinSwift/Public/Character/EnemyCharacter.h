#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterBase.h"
#include "EnemyCharacter.generated.h"

class USAbilitySystemComponent;

/**
 * 적. 플레이어와 달리 ASC·AttributeSet을 캐릭터 자신이 가짐 (죽으면 같이 사라져도 됨)
 * 단계 3: 맞으면 체력만 깎이는 허수아비. AI·사망은 이후 단계에서
 * 단계 6: 시험용 주기 공격 (회피·타이밍 교체 시험). 보스 AI가 생기면 끔
 */
UCLASS()
class TWINSWIFT_API AEnemyCharacter : public ACharacterBase
{
	GENERATED_BODY()

public:
	AEnemyCharacter();

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void InitAbilityActorInfo() override;

	// [시험용] 켜면 일정 간격으로 주변을 공격: 바닥에 빨간 원 경고 → 안쪽 원이 다 차는 순간 원 안이면 데미지
	UPROPERTY(EditAnywhere, Category = "Debug Attack")
	bool bDebugAutoAttack = false;

	// 공격 간격(초). 경고 시간 포함
	UPROPERTY(EditAnywhere, Category = "Debug Attack", meta = (EditCondition = "bDebugAutoAttack", ClampMin = "0.5"))
	float DebugAttackInterval = 3.f;

	// 경고 시간(초). 안쪽 원이 커지다가 바깥 원에 닿는 순간 데미지 → "맞기 직전"을 눈으로 잴 수 있음
	UPROPERTY(EditAnywhere, Category = "Debug Attack", meta = (EditCondition = "bDebugAutoAttack", ClampMin = "0.1"))
	float DebugAttackWindup = 0.8f;

	// 공격 반경(cm)
	UPROPERTY(EditAnywhere, Category = "Debug Attack", meta = (EditCondition = "bDebugAutoAttack", ClampMin = "50"))
	float DebugAttackRadius = 350.f;

	// 데미지 배율 (적 공격력 10 × 배율)
	UPROPERTY(EditAnywhere, Category = "Debug Attack", meta = (EditCondition = "bDebugAutoAttack", ClampMin = "0"))
	float DebugAttackMultiplier = 2.f;

private:
	// 경고 시작
	void StartDebugAttack();

	// 경고 중: 안쪽 원을 키워 가며 다시 그림 (0.05초마다)
	void DrawDebugAttackWarning();

	// 경고 끝: 원 안의 플레이어에게 데미지 (실제 공격과 같은 데미지 GE 경로)
	void ExecuteDebugAttack();

	FTimerHandle DebugAttackTimer;
	FTimerHandle DebugAttackWarningTimer;
	FTimerHandle DebugAttackHitTimer;

	// 이번 경고가 시작된 시각 (게임 시간)
	double DebugAttackStartTime = 0.0;
};
