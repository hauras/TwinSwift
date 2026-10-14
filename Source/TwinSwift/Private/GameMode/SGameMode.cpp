#include "GameMode/SGameMode.h"

#include "Controller/SPlayerController.h"
#include "State/SPlayerState.h"

ASGameMode::ASGameMode()
{
	PlayerControllerClass = ASPlayerController::StaticClass();

	// ASC·AttributeSet을 가진 PlayerState
	PlayerStateClass = ASPlayerState::StaticClass();
}
