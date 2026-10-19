#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "CharacterBase.generated.h"

class UAbilitySystemComponent;
class USAttributeSet;

/**
 * 플레이어·적 공통 부모
 * ASC는 플레이어면 PlayerState, 적이면 캐릭터 자신이 가짐. 여기서는 포인터만 들고 있음
 */
UCLASS(Abstract)
class TWINSWIFT_API ACharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACharacterBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	USAttributeSet* GetAttributeSet() const { return AttributeSet; }

	// 히트스톱: Duration(실제 시간 초) 동안 이 캐릭터만 거의 멈춤. 도중에 또 걸면 시간이 처음부터 다시 시작
	void ApplyHitStop(float Duration);

	bool IsInHitStop() const { return bInHitStop; }

protected:
	// ASC에 "주인(Owner)과 몸(Avatar)"을 알려 줌. 자식이 구현
	virtual void InitAbilityActorInfo() {}

	// 히트스톱이 아닐 때의 시간 배율. 다른 연출(완벽 회피 슬로)이 배율을 바꾸면 자식이 재정의
	virtual float GetNormalTimeDilation() const { return 1.f; }

	// 지금 상태(히트스톱 / 평소)에 맞게 CustomTimeDilation을 다시 정함. 배율을 바꾸는 곳은 모두 이걸 부름
	void RefreshTimeDilation();

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<USAttributeSet> AttributeSet;

private:
	void EndHitStop();

	FTimerHandle HitStopTimer;

	bool bInHitStop = false;
};
