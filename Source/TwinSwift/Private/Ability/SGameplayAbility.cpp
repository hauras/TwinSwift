#include "Ability/SGameplayAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "SGameplayTags.h"
#include "Ability/SAbilityFunctionLibrary.h"
#include "Ability/SGE_Cooldown.h"
#include "Ability/SGE_Damage.h"
#include "Controller/SPlayerController.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Pawn.h"

USGameplayAbility::USGameplayAbility()
{
	// 능력마다 인스턴스 하나 (멤버 변수에 상태 저장 가능, 콤보 단계 등)
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 싱글: 내 컴퓨터에서만 실행 (멀티를 붙일 때 LocalPredicted로)
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;

	DamageEffectClass = USGE_Damage::StaticClass();
}

const FGameplayTagContainer* USGameplayAbility::GetCooldownTags() const
{
	// 시간만 정한 쿨다운이면 이 능력의 태그, 아니면 엔진 기본 (Cooldown GE의 태그)
	if (CooldownDuration > 0.f && CooldownTags.Num() > 0)
	{
		return &CooldownTags;
	}
	return Super::GetCooldownTags();
}

void USGameplayAbility::ApplyCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (CooldownDuration <= 0.f || CooldownTags.Num() == 0)
	{
		Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
		return;
	}

	// 공통 쿨다운 GE에 이 능력의 시간·태그를 넣어 적용 → 능력마다 쿨다운 GE 에셋이 필요 없음
	FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(USGE_Cooldown::StaticClass(), GetAbilityLevel());
	if (Spec.IsValid())
	{
		Spec.Data->DynamicGrantedTags.AppendTags(CooldownTags);
		Spec.Data->SetSetByCallerMagnitude(SGameplayTags::SetByCaller_Duration, CooldownDuration);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
	}
}

bool USGameplayAbility::ApplyDamageTo(AActor* Target, float Multiplier) const
{
	if (!Target || USAbilityFunctionLibrary::IsFriend(GetAvatarActorFromActorInfo(), Target))
	{
		return false;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!TargetASC || !SourceASC || !DamageEffectClass)
	{
		return false;
	}

	const FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
	if (!Spec.IsValid())
	{
		return false;
	}

	Spec.Data->SetSetByCallerMagnitude(SGameplayTags::SetByCaller_DamageMultiplier, Multiplier);
	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	return true;
}

int32 USGameplayAbility::ApplyDamageInRadius(
	const FVector& Center,
	float Radius,
	float Multiplier,
	TArray<AActor*>* OutHitActors) const
{
	UWorld* World = GetWorld();
	if (!World || Radius <= 0.f)
	{
		return 0;
	}

	// 칼날 판정(ANS_WeaponTrace)과 같은 기준: Pawn 오브젝트만 찾음
	TArray<FOverlapResult> Overlaps;
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(AbilityRadiusDamage), false, GetAvatarActorFromActorInfo());
	World->OverlapMultiByObjectType(
		Overlaps,
		Center,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(Radius),
		Params);

	// 한 액터의 컴포넌트가 여럿 걸려도 한 번만 맞음
	TSet<AActor*> Visited;
	int32 HitCount = 0;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (!Target || Visited.Contains(Target))
		{
			continue;
		}
		Visited.Add(Target);

		if (ApplyDamageTo(Target, Multiplier))
		{
			++HitCount;
			if (OutHitActors)
			{
				OutHitActors->Add(Target);
			}
		}
	}
	return HitCount;
}

FVector USGameplayAbility::GetAimPoint() const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return FVector::ZeroVector;
	}

	if (const APawn* Pawn = Cast<APawn>(Avatar))
	{
		if (const ASPlayerController* PlayerController = Cast<ASPlayerController>(Pawn->GetController()))
		{
			return PlayerController->GetAimPoint();
		}
	}
	return Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * 100.f;
}

void USGameplayAbility::FaceAimPoint() const
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return;
	}

	const FVector ToAim = GetAimPoint() - Avatar->GetActorLocation();
	if (ToAim.SizeSquared2D() < 1.f)
	{
		return;
	}

	Avatar->SetActorRotation(FRotator(0.f, ToAim.Rotation().Yaw, 0.f));
}
