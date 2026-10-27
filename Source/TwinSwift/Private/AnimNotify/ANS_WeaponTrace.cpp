#include "AnimNotify/ANS_WeaponTrace.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "DrawDebugHelpers.h"
#include "SGameplayTags.h"
#include "Character/CharacterBase.h"
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

	// 새로 휘두르기 시작 → 이 캐릭터가 이미 맞힌 목록·지난 칼날 위치 비움 (첫 프레임은 지금 칼날만 검사)
	HitActorsByMesh.FindOrAdd(MeshComp).Reset();
	LastBladeByMesh.Remove(MeshComp);
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

	// 칼은 캐릭터에게 물음 (플레이어·거울 보스가 같은 공격 몽타주를 씀)
	const ACharacterBase* Character = Cast<ACharacterBase>(Owner);
	const UStaticMeshComponent* Weapon = Character ? Character->GetWeaponMesh() : nullptr;
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

	// 1) 지금 칼날
	SweepBlade(World, BladeStart, BladeEnd, Params, Hits);

	// 2) 지난 프레임 → 지금까지 칼날이 쓸고 지나간 면: 칼날 위 여러 점을 지난 위치에서 지금 위치로 쓸어 봄
	//    지금 칼날만 보면 빠른 휘두르기·낮은 프레임에서 프레임 사이에 지나친 적을 놓침 (눈으로는 베었는데 빗나감)
	//    점 간격은 판정 구 지름 이하 → 점 사이 틈으로 빠지지 않게
	if (const TPair<FVector, FVector>* LastBlade = LastBladeByMesh.Find(MeshComp))
	{
		const int32 Samples = FMath::Clamp(FMath::CeilToInt(FVector::Dist(BladeStart, BladeEnd) / (TraceRadius * 2.f)), 1, 8);
		for (int32 Index = 0; Index <= Samples; ++Index)
		{
			const float Alpha = static_cast<float>(Index) / Samples;
			const FVector From = FMath::Lerp(LastBlade->Key, LastBlade->Value, Alpha);
			const FVector To = FMath::Lerp(BladeStart, BladeEnd, Alpha);
			SweepBlade(World, From, To, Params, Hits);
		}
	}
	LastBladeByMesh.Add(MeshComp, TPair<FVector, FVector>(BladeStart, BladeEnd));

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
	LastBladeByMesh.Remove(MeshComp);

	Super::NotifyEnd(MeshComp, Animation, EventReference);
}

void UANS_WeaponTrace::SweepBlade(UWorld* World, const FVector& From, const FVector& To, const FCollisionQueryParams& Params, TArray<FHitResult>& OutHits) const
{
	TArray<FHitResult> Hits;
	World->SweepMultiByObjectType(
		Hits,
		From,
		To,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(TraceRadius),
		Params);
	OutHits.Append(Hits);
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
