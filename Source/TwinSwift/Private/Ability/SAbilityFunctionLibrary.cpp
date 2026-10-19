#include "Ability/SAbilityFunctionLibrary.h"

#include "Character/PlayerCharacter.h"
#include "NiagaraFunctionLibrary.h"
#include "Pool/SActorPool.h"

bool USAbilityFunctionLibrary::IsFriend(const AActor* A, const AActor* B)
{
	if (!A || !B)
	{
		return false;
	}

	return A->IsA<APlayerCharacter>() == B->IsA<APlayerCharacter>();
}

void USAbilityFunctionLibrary::SpawnOneShotEffect(
	const UObject* WorldContext,
	UNiagaraSystem* Effect,
	const FVector& Location,
	const FRotator& Rotation,
	const FVector& Scale)
{
	if (!WorldContext || !Effect)
	{
		return;
	}
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		WorldContext,
		Effect,
		Location,
		Rotation,
		Scale,
		true,
		true,
		USActorPool::IsPoolingEnabled() ? ENCPoolMethod::AutoRelease : ENCPoolMethod::None);
}
