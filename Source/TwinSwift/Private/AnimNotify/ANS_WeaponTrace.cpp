#include "AnimNotify/ANS_WeaponTrace.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "DrawDebugHelpers.h"
#include "SGameplayTags.h"
#include "Character/PlayerCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"

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

	// 칼날 시작~끝을 월드 위치로 (칼 메시의 소켓 두 개)
	FVector BladeStartLocal;
	FVector BladeEndLocal;
	GetBladeLocal(Weapon, BladeStartLocal, BladeEndLocal);
	const FTransform& WeaponTransform = Weapon->GetComponentTransform();
	const FVector BladeStart = WeaponTransform.TransformPosition(BladeStartLocal);
	const FVector BladeEnd = WeaponTransform.TransformPosition(BladeEndLocal);

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

void UANS_WeaponTrace::GetBladeLocal(const UStaticMeshComponent* Weapon, FVector& OutStart, FVector& OutEnd) const
{
	const UStaticMesh* WeaponMesh = Weapon->GetStaticMesh();
	const UStaticMeshSocket* StartSocket = WeaponMesh->FindSocket(BladeStartSocket);
	const UStaticMeshSocket* EndSocket = WeaponMesh->FindSocket(BladeEndSocket);
	if (StartSocket && EndSocket)
	{
		OutStart = StartSocket->RelativeLocation;
		OutEnd = EndSocket->RelativeLocation;
		return;
	}

	// 소켓이 없는 무기: 피벗(손잡이 끝)에서 +Z로 뻗는다고 보고 비율로 계산
	const float WeaponLength = WeaponMesh->GetBoundingBox().Max.Z;
	OutStart = FVector(0.f, 0.f, WeaponLength * BladeStartRatio);
	OutEnd = FVector(0.f, 0.f, WeaponLength);
}

FString UANS_WeaponTrace::GetNotifyName_Implementation() const
{
	return TEXT("Weapon Trace");
}
