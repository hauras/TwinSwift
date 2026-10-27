#include "Ability/SGameplayAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "SGameplayTags.h"
#include "Ability/SAbilityFunctionLibrary.h"
#include "Ability/SGE_Cooldown.h"
#include "Ability/SGE_Damage.h"
#include "Character/CharacterBase.h"
#include "Controller/SPlayerController.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Pawn.h"
#include "NiagaraFunctionLibrary.h"

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

bool USGameplayAbility::ApplyDamageTo(
	AActor* Target,
	float Multiplier,
	bool* bOutShattered,
	const FGameplayTag& DamageTypeTag) const
{
	if (bOutShattered)
	{
		*bOutShattered = false;
	}

	if (!USAbilityFunctionLibrary::IsTargetable(GetAvatarActorFromActorInfo(), Target))
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
	if (DamageTypeTag.IsValid())
	{
		Spec.Data->AddDynamicAssetTag(DamageTypeTag);
	}
	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);

	// 전사 능력이면 [빙결] 산산조각 (일반 피해 숫자 다음에 추가 피해 숫자)
	const bool bShattered = TryShatterFrost(Target, TargetASC);
	if (bOutShattered)
	{
		*bOutShattered = bShattered;
	}
	return true;
}

int32 USGameplayAbility::ApplyDamageInRadius(
	const FVector& Center,
	float Radius,
	float Multiplier,
	TArray<AActor*>* OutHitActors,
	TArray<AActor*>* OutShatteredActors) const
{
	TArray<AActor*> Enemies;
	FindEnemiesInRadius(Center, Radius, Enemies);

	int32 HitCount = 0;
	for (AActor* Target : Enemies)
	{
		bool bShattered = false;
		if (ApplyDamageTo(Target, Multiplier, &bShattered))
		{
			++HitCount;
			if (OutHitActors)
			{
				OutHitActors->Add(Target);
			}
			if (bShattered && OutShatteredActors)
			{
				OutShatteredActors->Add(Target);
			}
		}
	}
	return HitCount;
}

void USGameplayAbility::FindEnemiesInRadius(const FVector& Center, float Radius, TArray<AActor*>& OutEnemies) const
{
	UWorld* World = GetWorld();
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!World || Radius <= 0.f)
	{
		return;
	}

	// 칼날 판정(ANS_WeaponTrace)과 같은 기준: Pawn 오브젝트만 찾음
	TArray<FOverlapResult> Overlaps;
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(AbilityRadiusSearch), false, Avatar);
	World->OverlapMultiByObjectType(
		Overlaps,
		Center,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(Radius),
		Params);

	// 한 액터의 컴포넌트가 여럿 걸려도 한 번만. 같은 편과 ASC가 없는 것(맞을 수 없음)은 뺌
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (!Target || OutEnemies.Contains(Target))
		{
			continue;
		}
		if (!USAbilityFunctionLibrary::IsTargetable(Avatar, Target))
		{
			continue;
		}
		OutEnemies.Add(Target);
	}
}

void USGameplayAbility::ApplyTimedTagTo(AActor* Target, const FGameplayTag& Tag, float Duration) const
{
	if (!Target)
	{
		return;
	}
	ApplyTimedTagToASC(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target), Tag, Duration);
}

void USGameplayAbility::ApplyTimedTagToASC(UAbilitySystemComponent* TargetASC, const FGameplayTag& Tag, float Duration) const
{
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!TargetASC || !SourceASC || !Tag.IsValid() || Duration <= 0.f)
	{
		return;
	}

	const FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(USGE_Cooldown::StaticClass(), GetAbilityLevel());
	if (!Spec.IsValid())
	{
		return;
	}

	Spec.Data->DynamicGrantedTags.AddTag(Tag);
	Spec.Data->SetSetByCallerMagnitude(SGameplayTags::SetByCaller_Duration, Duration);
	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
}

bool USGameplayAbility::TryShatterFrost(AActor* Target, UAbilitySystemComponent* TargetASC) const
{
	// 죽었는지 다시 확인: 바로 앞의 일반 피해 GE가 즉시 적용돼 그 사이에 죽었을 수 있음 (시체에 추가 피해·경직·파편 방지)
	if (!bShattersFrost
		|| !USAbilityFunctionLibrary::IsTargetable(GetAvatarActorFromActorInfo(), Target)
		|| !TargetASC
		|| !TargetASC->HasMatchingGameplayTag(SGameplayTags::Mark_Frost))
	{
		return false;
	}

	// [빙결]을 먼저 지움 → 같은 프레임·다음 판정(돌진 베기 0.02초 간격)에서 두 번 터지지 않음
	TargetASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(SGameplayTags::Mark_Frost));

	// 추가 피해: 같은 데미지 GE에 산산조각 배율 + Damage.Shatter 표시 (숫자 색 구분용)
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	const FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
	if (SourceASC && Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(SGameplayTags::SetByCaller_DamageMultiplier, ShatterMultiplier);
		Spec.Data->AddDynamicAssetTag(SGameplayTags::Damage_Shatter);
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}

	// 경직
	ApplyTimedTagTo(Target, SGameplayTags::State_Stun, ShatterStunDuration);

	// 히트스톱: 때린 캐릭터와 맞은 적만 잠깐 멈춤 (다른 적·이펙트·카메라는 그대로)
	for (AActor* HitStopActor : { GetAvatarActorFromActorInfo(), Target })
	{
		if (ACharacterBase* Character = Cast<ACharacterBase>(HitStopActor))
		{
			Character->ApplyHitStop(ShatterHitStopDuration);
		}
	}

	// 파편 이펙트 (맞은 자리)
	if (ShatterEffect)
	{
		// 캡슐 중심에 만들면 몸체에 가려짐: 공격자 쪽 표면으로 빼서 생성
		FVector EffectLocation = Target->GetActorLocation() + FVector(0.f, 0.f, 30.f);
		if (const AActor* Avatar = GetAvatarActorFromActorInfo())
		{
			EffectLocation += (Avatar->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D() * 45.f;
		}
		USAbilityFunctionLibrary::SpawnOneShotEffect(
			GetWorld(),
			ShatterEffect,
			EffectLocation,
			FRotator::ZeroRotator,
			FVector(1.6f));
	}
	return true;
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

		FVector AimPoint;
		const ACharacterBase* Character = Cast<ACharacterBase>(Avatar);
		if (Character && Character->GetAbilityAimPoint(AimPoint))
		{
			return AimPoint;
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
