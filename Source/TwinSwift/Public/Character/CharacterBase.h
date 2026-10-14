#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CharacterBase.generated.h"

/**
 * 플레이어·적 공통 부모.
 * 조각 2에서 GAS(IAbilitySystemInterface)를 붙임
 */
UCLASS(Abstract)
class TWINSWIFT_API ACharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	ACharacterBase();
};
