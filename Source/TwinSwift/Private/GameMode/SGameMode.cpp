#include "GameMode/SGameMode.h"

#include "Controller/SPlayerController.h"

ASGameMode::ASGameMode()
{
	PlayerControllerClass = ASPlayerController::StaticClass();
}
