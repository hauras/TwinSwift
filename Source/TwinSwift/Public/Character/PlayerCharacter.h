#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterBase.h"
#include "PlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class USCameraConfig;
class UStaticMeshComponent;
class UGameplayAbility;
class UGameplayEffect;

/**
 * 플레이어 캐릭터.
 * 조각 5에서 몸(메시) 두 개를 가진 상주형 태그 캐릭터가 됨
 */
UCLASS()
class TWINSWIFT_API APlayerCharacter : public ACharacterBase
{
	GENERATED_BODY()

public:
	APlayerCharacter();

	// 서버: 컨트롤러가 이 캐릭터를 잡았을 때 → GAS 초기화 + 시작 능력·스탯 지급
	virtual void PossessedBy(AController* NewController) override;

	// 클라: PlayerState가 복제돼 왔을 때 → GAS 초기화
	virtual void OnRep_PlayerState() override;

	// 카메라 설정 에셋 내용을 스프링암·카메라에 적용 (내 화면에서만 의미 있음)
	void ApplyCameraConfig(const USCameraConfig* Config);

	// 걷기 켜기/끄기. 내 쪽은 바로 바꾸고 서버에도 알림
	void SetWalking(bool bNewWalking);

	UCameraComponent* GetCamera() const { return Camera; }

protected:
	virtual void BeginPlay() override;

	virtual void InitAbilityActorInfo() override;

	// 시작할 때 받는 능력 (예: GA_Warrior_Combo)
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;

	// 시작 스탯 GE (비워 두면 AttributeSet 생성자의 기본값 사용)
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGameplayEffect> DefaultAttributesEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float RunSpeed = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float WalkSpeed = 200.f;

private:
	UFUNCTION(Server, Reliable)
	void Server_SetWalking(bool bNewWalking);

	void ApplyWalkSpeed(bool bNewWalking);

	// 서버에서 한 번만: 시작 스탯 GE 적용 + 시작 능력 지급
	void GiveStartupAbilitiesAndAttributes();

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	// 무기 (전사 대검). 메시·손 안 위치는 BP에서 지정, hand_R 뼈에 붙음
	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;
};
