#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterBase.h"
#include "PlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class USCameraConfig;
class UStaticMeshComponent;

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

	// 카메라 설정 에셋 내용을 스프링암·카메라에 적용 (내 화면에서만 의미 있음)
	void ApplyCameraConfig(const USCameraConfig* Config);

	// 걷기 켜기/끄기. 내 쪽은 바로 바꾸고 서버에도 알림
	void SetWalking(bool bNewWalking);

	UCameraComponent* GetCamera() const { return Camera; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float RunSpeed = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float WalkSpeed = 200.f;

private:
	UFUNCTION(Server, Reliable)
	void Server_SetWalking(bool bNewWalking);

	void ApplyWalkSpeed(bool bNewWalking);

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	// 무기 (전사 대검). 메시·손 안 위치는 BP에서 지정, hand_R 뼈에 붙음
	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;
};
