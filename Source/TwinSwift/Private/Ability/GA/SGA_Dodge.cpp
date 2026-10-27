#include "Ability/GA/SGA_Dodge.h"

#include "AbilitySystemComponent.h"
#include "SGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/PlayerCharacter.h"
#include "Controller/SPlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

USGA_Dodge::USGA_Dodge()
{
	InputTag = SGameplayTags::InputTag_Dodge;

	// 쓰러진 캐릭터는 회피 못 함
	ActivationBlockedTags.AddTag(SGameplayTags::State_Dead);

	// 경직 중에도 못 함 (보스 점프 내려찍기 등). 경직 태그는 캐릭터(ASC)마다 따로라 교체하면 빠져나옴 → 탈출 수단은 교체
	ActivationBlockedTags.AddTag(SGameplayTags::State_Stun);

	// 기본 쿨다운 1초 (블루프린트에서 바꿀 수 있음)
	CooldownDuration = 1.f;
	CooldownTags.AddTag(SGameplayTags::Cooldown_Dodge);
}

void USGA_Dodge::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bMoveFinished = false;
	bMontageFinished = DodgeMontage == nullptr;

	// 공격 중이면 끊고 회피 (이 능력은 빼고 나머지 취소)
	GetAbilitySystemComponentFromActorInfo()->CancelAbilities(nullptr, nullptr, this);

	StartInvincible();

	// 구르는 방향으로 바로 돌림 (구르기 애니가 앞으로 구르니까)
	const FVector Direction = GetDodgeDirection();
	Character->SetActorRotation(Direction.Rotation());

	if (DodgeMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			DodgeMontage,
			MontagePlayRate,
			NAME_None,
			true,
			1.f,
			MontageStartTime);
		// 1차 교훈: 끝나기 시작(BlendOut)·끊김·취소 모두 종료 경로로
		MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageEnded);
		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageEnded);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageEnded);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageEnded);
		MontageTask->ReadyForActivation();
	}

	if (DodgeDuration > 0.f)
	{
		// 구르기: 정해진 시간 동안 일정한 힘으로 밀기 (입력·중력과 상관없이 같은 거리)
		UAbilityTask_ApplyRootMotionConstantForce* MoveTask = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
			this,
			NAME_None,
			Direction,
			DodgeDistance / DodgeDuration,
			DodgeDuration,
			false,
			nullptr,
			ERootMotionFinishVelocityMode::ClampVelocity,
			FVector::ZeroVector,
			Character->GetCharacterMovement()->MaxWalkSpeed,
			true);
		MoveTask->OnFinish.AddDynamic(this, &ThisClass::OnMoveFinished);
		MoveTask->ReadyForActivation();
	}
	else
	{
		// 순간이동: 벽이 있으면 그 앞에서 멈춤 (Sweep)
		Character->SetActorLocation(Character->GetActorLocation() + Direction * DodgeDistance, true);
		OnMoveFinished();
	}
}

void USGA_Dodge::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 무적 시간보다 빨리 끝나거나 취소돼도 무적이 남지 않게
	StopInvincible();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

FVector USGA_Dodge::GetDodgeDirection() const
{
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return FVector::ForwardVector;
	}

	// 지금 누르고 있는 이동 방향. 없으면 캐릭터가 보는 쪽
	// 플레이어는 컨트롤러가 기록한 방향 우선: 공격 중에는 이동 입력이 캐릭터에 안 넘어가 마지막 이동 입력이 비어 있음
	// (입력 버퍼로 공격 중 눌러 둔 회피가 옆으로 피하려 했는데 앞으로 구르지 않게)
	FVector Direction = FVector::ZeroVector;
	if (const ASPlayerController* PlayerController = Character->GetController<ASPlayerController>())
	{
		Direction = PlayerController->GetDesiredMoveDirection();
	}

	// ② 비어 있으면 캐릭터가 정한 방향 (거울 보스)
	if (Direction.IsNearlyZero())
	{
		if (const ACharacterBase* CharacterBase = Cast<ACharacterBase>(Character))
		{
			CharacterBase->GetAbilityMoveDirection(Direction);
		}
	}
	
	if (Direction.IsNearlyZero())
	{
		Direction = Character->GetLastMovementInputVector();
	}
	Direction.Z = 0.f;
	if (!Direction.Normalize())
	{
		Direction = Character->GetActorForwardVector();
	}
	return Direction;
}

void USGA_Dodge::StartInvincible()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || InvincibleTime <= 0.f)
	{
		return;
	}

	ASC->AddLooseGameplayTag(SGameplayTags::State_Invincible);
	bInvincible = true;

	// 회피 시작 직후에 맞으면 완벽 회피 (판정 시간은 캐릭터 설정, 무적 시간 안쪽)
	if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		PlayerCharacter->OpenPerfectWindow(ASC);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(InvincibleTimer, this, &ThisClass::StopInvincible, InvincibleTime, false);
	}
}

void USGA_Dodge::StopInvincible()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InvincibleTimer);
	}

	if (!bInvincible)
	{
		return;
	}
	bInvincible = false;

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(SGameplayTags::State_Invincible);
	}
}

void USGA_Dodge::OnMoveFinished()
{
	bMoveFinished = true;
	TryEndDodge();
}

void USGA_Dodge::OnMontageEnded()
{
	bMontageFinished = true;
	TryEndDodge();
}

void USGA_Dodge::TryEndDodge()
{
	if (bMoveFinished && bMontageFinished && IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}
