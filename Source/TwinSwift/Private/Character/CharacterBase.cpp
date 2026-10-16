#include "Character/CharacterBase.h"

#include "Ability/SAttributeSet.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

ACharacterBase::ACharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// 캐릭터(플레이어·적)는 카메라를 막지 않음
	// 막으면 적과 겹치거나 적이 카메라와 캐릭터 사이에 들어올 때 스프링암이 벽으로 보고 카메라를 확 당김
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
}

UAbilitySystemComponent* ACharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
