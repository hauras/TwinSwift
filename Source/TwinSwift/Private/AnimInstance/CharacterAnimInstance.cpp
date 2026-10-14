#include "AnimInstance/CharacterAnimInstance.h"

#include "Character/CharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"

void UCharacterAnimInstance::NativeInitializeAnimation()
{
	OwningCharacter = Cast<ACharacterBase>(TryGetPawnOwner());

	if (OwningCharacter)
	{
		OwningMovementComponent = OwningCharacter->GetCharacterMovement();
	}
}

void UCharacterAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	if (!OwningCharacter || !OwningMovementComponent)
	{
		return;
	}

	GroundSpeed = OwningCharacter->GetVelocity().Size2D();

	bHasAcceleration = OwningMovementComponent->GetCurrentAcceleration().SizeSquared() > 0.f;

	// 아주 느린 미끄러짐은 이동으로 치지 않음
	bShouldMove = GroundSpeed > 3.f && bHasAcceleration;

	bIsFalling = OwningMovementComponent->IsFalling();
}
