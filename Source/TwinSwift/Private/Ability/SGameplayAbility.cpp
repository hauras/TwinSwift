#include "Ability/SGameplayAbility.h"

USGameplayAbility::USGameplayAbility()
{
	// 능력마다 인스턴스 하나 (멤버 변수에 상태 저장 가능, 콤보 단계 등)
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 싱글: 내 컴퓨터에서만 실행 (멀티를 붙일 때 LocalPredicted로)
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}
