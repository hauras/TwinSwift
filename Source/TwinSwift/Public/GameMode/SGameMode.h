#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SGameMode.generated.h"

/**
 * 기본 게임 모드. 폰은 BP_GameMode에서 BP_Player로 지정
 */
UCLASS()
class TWINSWIFT_API ASGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASGameMode();
};
