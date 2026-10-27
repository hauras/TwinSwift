#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "CharacterBase.generated.h"

class UAbilitySystemComponent;
class UStaticMeshComponent;
class USAttributeSet;

UCLASS(Abstract)
class TWINSWIFT_API ACharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACharacterBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// 능력이 겨냥할 곳을 캐릭터가 정할 때 (입력이 없는 거울 보스). true면 OutAimPoint를 씀
	// 플레이어는 컨트롤러가 정해서 재정의 안 함 (능력이 컨트롤러에게 먼저 물음)
	virtual bool GetAbilityAimPoint(FVector& OutAimPoint) const { return false; }

	// 회피처럼 누르고 있는 방향이 필요한 능력용. 입력이 없는 캐릭터가 재정의. true면 OutDirection을 씀
	virtual bool GetAbilityMoveDirection(FVector& OutDirection) const { return false; }

	// 칼 판정(ANS_WeaponTrace)이 쓰는 무기. 없으면 nullptr (칼을 든 캐릭터가 재정의: 플레이어, 거울 보스)
	virtual UStaticMeshComponent* GetWeaponMesh() const { return nullptr; }

	USAttributeSet* GetAttributeSet() const { return AttributeSet; }

	void ApplyHitStop(float Duration);

	bool IsInHitStop() const { return bInHitStop; }

protected:
	virtual void InitAbilityActorInfo() {}

	virtual float GetNormalTimeDilation() const { return 1.f; }

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
