#include "Ability/SAbilityFunctionLibrary.h"

#include "Character/PlayerCharacter.h"

bool USAbilityFunctionLibrary::IsFriend(const AActor* A, const AActor* B)
{
	if (!A || !B)
	{
		return false;
	}

	return A->IsA<APlayerCharacter>() == B->IsA<APlayerCharacter>();
}
