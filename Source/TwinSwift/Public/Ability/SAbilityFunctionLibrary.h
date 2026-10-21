#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SAbilityFunctionLibrary.generated.h"

class UNiagaraSystem;

UCLASS()
class TWINSWIFT_API USAbilityFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 같은 편인지. 팀 판정은 이 함수 하나로만 (1차에서 여기저기 따로 판정해서 어긋났음)
	// 지금 규칙: 플레이어끼리, 적끼리는 같은 편
	UFUNCTION(BlueprintPure, Category = "Combat")
	static bool IsFriend(const AActor* A, const AActor* B);

	// 때려도 되는 대상인지: 있음 + 같은 편 아님 + ASC 있음 + 죽지 않음(State.Dead 없음)
	// 표적 고르기·범위 피해·투사체 명중이 모두 이 함수 하나로 (규칙이 늘어도 여기만 고침)
	static bool IsTargetable(const AActor* Source, const AActor* Target);

	// 한 번 터지고 끝나는 이펙트를 그 자리에 (명중·폭발·시전 섬광 등)
	// 엔진 나이아가라 풀(AutoRelease)에서 꺼내 쓰고, 다 재생되면 풀로 돌아감 → 매번 컴포넌트를 만들고 지우지 않음 (GC 부담 감소)
	// 붙어서 따라가는 이펙트(투사체 비행 등)에는 쓰지 않음: 풀로 돌아간 뒤에도 붙은 채로 다른 곳에 재사용될 수 있어서
	static void SpawnOneShotEffect(
		const UObject* WorldContext,
		UNiagaraSystem* Effect,
		const FVector& Location,
		const FRotator& Rotation = FRotator::ZeroRotator,
		const FVector& Scale = FVector::OneVector);
};
