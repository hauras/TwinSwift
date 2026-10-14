#include "Ability/SGameplayAbility.h"

USGameplayAbility::USGameplayAbility()
{
	// 능력마다 인스턴스 하나 (멤버 변수에 상태 저장 가능, 콤보 단계 등)
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 클라가 먼저 실행하고 서버가 확인 → 입력 반응이 지연 없이 바로
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}
