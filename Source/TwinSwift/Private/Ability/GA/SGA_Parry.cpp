#include "Ability/GA/SGA_Parry.h"

#include "AbilitySystemComponent.h"
#include "SGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "Ability/SAbilityFunctionLibrary.h"
#include "Camera/CameraShakeBase.h"
#include "Character/BossCharacter.h"
#include "Character/PlayerCharacter.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

USGA_Parry::USGA_Parry()
{
	InputTag = SGameplayTags::InputTag_SecondaryAttack;

	// 자세·반격 동안 이동 입력을 막음 (서서 받아침)
	ActivationOwnedTags.AddTag(SGameplayTags::State_Attacking);

	ActivationBlockedTags.AddTag(SGameplayTags::State_Stun);
	ActivationBlockedTags.AddTag(SGameplayTags::State_Dead);

	// 콤보 중에 누르면 콤보를 끊고 자세 (반응 기술)
	CancelAbilitiesWithTag.AddTag(SGameplayTags::Ability_Warrior_Combo);

	CooldownDuration = 6.f;
	CooldownTags.AddTag(SGameplayTags::Cooldown_Warrior_Parry);
}

void USGA_Parry::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!GetAvatarActorFromActorInfo() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 능력 인스턴스는 재사용되므로 매번 처음 상태로
	bCountered = false;

	// 맞음 알림을 자세보다 먼저 기다림 (같은 프레임에 맞아도 놓치지 않게). 한 번만
	UAbilityTask_WaitGameplayEvent* ParryTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		SGameplayTags::Event_Parry_Success,
		nullptr,
		true);
	ParryTask->EventReceived.AddDynamic(this, &ThisClass::OnParried);
	ParryTask->ReadyForActivation();

	SetParrying(true);

	// 자세 몽타주는 끝 신호를 안 받음: 능력이 끝나면 멈추고(bStopWhenAbilityEnds), 반격이 나오면 반격 몽타주가 덮음
	if (StanceMontage)
	{
		// 자세 + 빈틈 시간 동안 애니가 끝나지 않게 느리게 (시간을 바꾸면 속도도 같이 맞음)
		const float StanceLength = FMath::Max(StanceMontage->GetPlayLength() - StanceStartTime, 0.01f);
		const float StanceRate = StanceLength / FMath::Max(ParryWindow + RecoveryTime, 0.01f);

		UAbilityTask_PlayMontageAndWait* StanceTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			StanceMontage,
			StanceRate,
			NAME_None,
			true,
			1.f,
			StanceStartTime);
		StanceTask->ReadyForActivation();
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(WindowTimer, this, &ThisClass::OnWindowEnded, ParryWindow, false);
	}
}

void USGA_Parry::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 취소(교체·피격 경직)돼도 태그·타이머가 남지 않게
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WindowTimer);
		World->GetTimerManager().ClearTimer(RecoveryTimer);
	}
	SetParrying(false);
	SetInvincible(false);
	CounterTarget.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USGA_Parry::OnParried(FGameplayEventData Payload)
{
	if (bCountered || !bParrying) return;

	bCountered = true;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WindowTimer);
	}
	SetParrying(false);

	// 반격 동안은 무적 (받아친 직후 다른 공격에 맞아 끊기지 않게)
	SetInvincible(true);

	AActor* Attacker = const_cast<AActor*>(Payload.Instigator.Get());
	PlayParryFeedback(Attacker);

	// 때린 쪽을 봄 (멀리서 쏜 탄이어도 그쪽으로). 반격 거리 안이었으면 칼이 지나갈 때 때릴 상대로 기억
	CounterTarget.Reset();
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (Character && IsValid(Attacker))
	{
		FVector ToAttacker = Attacker->GetActorLocation() - Character->GetActorLocation();
		ToAttacker.Z = 0.f;
		if (!ToAttacker.IsNearlyZero())
		{
			Character->SetActorRotation(ToAttacker.Rotation());
		}
		if (ToAttacker.Size() <= CounterRange)
		{
			CounterTarget = Attacker;
		}

	}

	// 몽타주가 없으면 칼이 지나갈 때를 알 수 없음 → 바로 때리고 끝
	if (!CounterMontage)
	{
		CounterAttack(CounterTarget.Get());
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// 칼이 지나가는 순간의 신호를 몽타주보다 먼저 기다림. 한 번만
	UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		SGameplayTags::Event_Montage_Hit,
		nullptr,
		true);
	HitTask->EventReceived.AddDynamic(this, &ThisClass::OnCounterHit);
	HitTask->ReadyForActivation();

	UAbilityTask_PlayMontageAndWait* CounterTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		CounterMontage,
		CounterPlayRate);
	// 1차 교훈: 끝나기 시작(BlendOut)·끊김·취소 모두 종료 경로로
	CounterTask->OnBlendOut.AddDynamic(this, &ThisClass::OnCounterMontageEnded);
	CounterTask->OnCompleted.AddDynamic(this, &ThisClass::OnCounterMontageEnded);
	CounterTask->OnInterrupted.AddDynamic(this, &ThisClass::OnCounterMontageEnded);
	CounterTask->OnCancelled.AddDynamic(this, &ThisClass::OnCounterMontageEnded);
	CounterTask->ReadyForActivation();
}

void USGA_Parry::PlayParryFeedback(AActor* Attacker)
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character) return;

	// 때린 쪽으로 조금 나간 가슴 높이 (칼이 부딪힌 자리처럼). 상대를 모르면 앞쪽
	FVector ToAttacker = IsValid(Attacker) ? Attacker->GetActorLocation() - Character->GetActorLocation() : Character->GetActorForwardVector();
	ToAttacker.Z = 0.f;
	const FVector EffectLocation = Character->GetActorLocation() + ToAttacker.GetSafeNormal() * 70.f + FVector(0.f, 0.f, 30.f);
	USAbilityFunctionLibrary::SpawnOneShotEffect(
		Character,
		ParryEffect,
		EffectLocation,
		FRotator::ZeroRotator,
		FVector(ParryEffectScale));

	if (ACharacterBase* Self = Cast<ACharacterBase>(Character))
	{
		Self->ApplyHitStop(ParryHitStop);
	}

	// 멀리서 쏜 탄이면 쏜 쪽은 안 멈춤 (막은 쪽만)
	ACharacterBase* AttackerCharacter = Cast<ACharacterBase>(Attacker);
	if (AttackerCharacter && ToAttacker.Size() <= CounterRange)
	{
		AttackerCharacter->ApplyHitStop(ParryHitStop);
	}

	if (ParryShake)
	{
		UGameplayStatics::PlayWorldCameraShake(
			Character,
			ParryShake,
			EffectLocation,
			1000.f,
			4000.f);
	}

	// 플레이어가 막았을 때만 글자 (거울 보스가 막았을 때는 이펙트·멈춤으로)
	if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(Character))
	{
		PlayerCharacter->OnParried.Broadcast();
	}
}

void USGA_Parry::OnCounterHit(FGameplayEventData Payload)
{
	CounterAttack(CounterTarget.Get());
	CounterTarget.Reset();
}

void USGA_Parry::CounterAttack(AActor* Target)
{
	// 막은 뒤 칼이 지나가기까지 시간이 있음 → 때리는 순간 다시 확인 (거리는 여기서, 쓰러졌는지·같은 편인지는 ApplyDamageTo가)
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!IsValid(Target) || !Avatar) return;

	const float Distance = FVector::Dist2D(Avatar->GetActorLocation(), Target->GetActorLocation());
	if (Distance > CounterRange * CounterHitRangeScale)
	{
		return;
	}
	// 막혔으면(거울 보스의 반격 자세·회피) false → 큰 경직도 없음
	if (!ApplyDamageTo(Target, CounterMultiplier)) return;

	// 피해로 쓰러졌을 수 있음 → 보스 쪽에서 사망·갑옷을 다시 봄
	if (ABossCharacter* Boss = Cast<ABossCharacter>(Target))
	{
		Boss->NotifyCountered();
	}
}

void USGA_Parry::OnWindowEnded()
{
	SetParrying(false);

	// 헛자세: 빈틈 뒤 종료
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			RecoveryTimer,
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
			}),
			FMath::Max(RecoveryTime, 0.01f),
			false);
	}
}

void USGA_Parry::OnCounterMontageEnded()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USGA_Parry::SetParrying(bool bEnable)
{
	if (bParrying == bEnable) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	bParrying = bEnable;
	if (bEnable)
	{
		ASC->AddLooseGameplayTag(SGameplayTags::State_Parrying);
	}
	else
	{
		ASC->RemoveLooseGameplayTag(SGameplayTags::State_Parrying);
	}
}

void USGA_Parry::SetInvincible(bool bEnable)
{
	if (bInvincible == bEnable) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	bInvincible = bEnable;
	if (bEnable)
	{
		ASC->AddLooseGameplayTag(SGameplayTags::State_Invincible);
	}
	else
	{
		ASC->RemoveLooseGameplayTag(SGameplayTags::State_Invincible);
	}
}
