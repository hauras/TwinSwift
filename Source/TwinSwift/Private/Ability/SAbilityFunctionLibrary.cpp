#include "Ability/SAbilityFunctionLibrary.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "SGameplayTags.h"
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

bool USAbilityFunctionLibrary::IsTargetable(const AActor* Source, const AActor* Target)
{
	if (!Target || IsFriend(Source, Target))
	{
		return false;
	}

	// ASC가 없으면 피해를 받을 수 없음 (벽·소품 등)
	// UAbilitySystemBlueprintLibrary 버전은 블루프린트용이라 const 포인터를 못 받음 → const를 받는 엔진 함수
	const UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (!TargetASC)
	{
		return false;
	}

	// 죽은 대상은 제외 (쓰러진 보스, 쓰러진 플레이어 캐릭터)
	return !TargetASC->HasMatchingGameplayTag(SGameplayTags::State_Dead);
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
