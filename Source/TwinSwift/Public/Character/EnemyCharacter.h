#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterBase.h"
#include "EnemyCharacter.generated.h"

class USAbilitySystemComponent;

/**
 * 적. 플레이어와 달리 ASC·AttributeSet을 캐릭터 자신이 가짐 (죽으면 같이 사라져도 됨)
 * 단계 3: 맞으면 체력만 깎이는 허수아비. AI·사망은 이후 단계에서
 */
UCLASS()
class TWINSWIFT_API AEnemyCharacter : public ACharacterBase
{
	GENERATED_BODY()

public:
	AEnemyCharacter();

protected:
	virtual void BeginPlay() override;

	virtual void InitAbilityActorInfo() override;
};
