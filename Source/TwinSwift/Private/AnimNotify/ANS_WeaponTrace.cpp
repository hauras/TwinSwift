#include "AnimNotify/ANS_WeaponTrace.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "DrawDebugHelpers.h"
#include "SGameplayTags.h"
#include "Character/PlayerCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

void UANS_WeaponTrace::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	// 새로 휘두르기 시작 → 이 캐릭터가 이미 맞힌 목록 비움
	HitActorsByMesh.FindOrAdd(MeshComp).Reset();
}

void UANS_WeaponTrace::NotifyTick(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	UWorld* World = MeshComp ? MeshComp->GetWorld() : nullptr;

	// 에디터 미리보기(애니 에디터)에서는 판정 안 함
	if (!Owner || !World || !World->IsGameWorld())
	{
		return;
	}

	const APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(Owner);
	const UStaticMeshComponent* Weapon = PlayerCharacter ? PlayerCharacter->GetWeaponMesh() : nullptr;
	if (!Weapon || !Weapon->GetStaticMesh())
	{
		return;
	}

	// 무기 메시는 피벗(손잡이 끝)에서 +Z 방향으로 뻗음 → 칼날 시작~끝을 월드 위치로
	const float BladeLength = Weapon->GetStaticMesh()->GetBoundingBox().Max.Z;
	const FTransform& WeaponTransform = Weapon->GetComponentTransform();
	const FVector BladeStart = WeaponTransform.TransformPosition(FVector(0.f, 0.f, BladeLength * BladeStartRatio));
	const FVector BladeEnd = WeaponTransform.TransformPosition(FVector(0.f, 0.f, BladeLength));

	FCollisionQueryParams Params(SCENE_QUERY_STAT(WeaponTrace), false, Owner);
	TArray<FHitResult> Hits;
	World->SweepMultiByObjectType(
		Hits,
		BladeStart,
		BladeEnd,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(TraceRadius),
		Params);

	if (bDrawDebug)
	{
		DrawDebugCapsule(
			World,
			(BladeStart + BladeEnd) * 0.5f,
			FVector::Dist(BladeStart, BladeEnd) * 0.5f + TraceRadius,
			TraceRadius,
			FRotationMatrix::MakeFromZ(BladeEnd - BladeStart).ToQuat(),
			Hits.Num() > 0 ? FColor::Red : FColor::Green,
			false,
			0.5f);
	}

	TSet<TWeakObjectPtr<AActor>>& AlreadyHit = HitActorsByMesh.FindOrAdd(MeshComp);
	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || AlreadyHit.Contains(HitActor))
		{
			continue;
		}
		AlreadyHit.Add(HitActor);

		FGameplayEventData Payload;
		Payload.EventTag = SGameplayTags::Event_Montage_Hit;
		Payload.Instigator = Owner;
		Payload.Target = HitActor;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, SGameplayTags::Event_Montage_Hit, Payload);
	}
}

void UANS_WeaponTrace::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	HitActorsByMesh.Remove(MeshComp);

	Super::NotifyEnd(MeshComp, Animation, EventReference);
}

FString UANS_WeaponTrace::GetNotifyName_Implementation() const
{
	return TEXT("Weapon Trace");
}
