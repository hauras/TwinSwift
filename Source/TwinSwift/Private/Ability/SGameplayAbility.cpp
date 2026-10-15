#include "Ability/SGameplayAbility.h"

#include "SGameplayTags.h"
#include "Ability/SGE_Cooldown.h"

USGameplayAbility::USGameplayAbility()
{
	// 능력마다 인스턴스 하나 (멤버 변수에 상태 저장 가능, 콤보 단계 등)
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 싱글: 내 컴퓨터에서만 실행 (멀티를 붙일 때 LocalPredicted로)
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
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
