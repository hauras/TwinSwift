#include "Ability/GA/SGA_GroundSlam.h"

#include "SGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Curves/CurveVector.h"
#include "GameFramework/Character.h"

USGA_GroundSlam::USGA_GroundSlam()
{
	InputTag = SGameplayTags::InputTag_Warrior_Skill2;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(SGameplayTags::Ability_Warrior_GroundSlam);
	SetAssetTags(AssetTags);

	// 내려찍는 동안 이동 입력을 막음
	ActivationOwnedTags.AddTag(SGameplayTags::State_Attacking);

	ActivationBlockedTags.AddTag(SGameplayTags::State_Stun);
	ActivationBlockedTags.AddTag(SGameplayTags::State_Dead);

	// 콤보 중에 누르면 콤보를 끊고 시전
	CancelAbilitiesWithTag.AddTag(SGameplayTags::Ability_Warrior_Combo);

	CooldownDuration = 10.f;
	CooldownTags.AddTag(SGameplayTags::Cooldown_Warrior_GroundSlam);
}

void USGA_GroundSlam::ActivateAbility(
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

	// 능력 인스턴스는 재사용되므로 매번 처음 상태로
	bImpacted = false;

	const FVector LandingLocation = GetLandingLocation();
	FaceAimPoint();

	// 몽타주의 착지 프레임(AN_SendEvent)에서 오는 신호. 몽타주보다 먼저 기다리기 시작해야 놓치지 않음
	UAbilityTask_WaitGameplayEvent* ImpactTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		SGameplayTags::Event_Montage_Hit);
	ImpactTask->EventReceived.AddDynamic(this, &ThisClass::OnImpact);
	ImpactTask->ReadyForActivation();

	if (SlamMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			SlamMontage,
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

	// 착지 지점까지 LeapDuration 동안 포물선으로 이동. 도착하면 속도 0으로 멈춤
	// 걷기 모드는 바닥에 붙어서 위로 못 뜸 → 나는 동안만 Flying (태스크가 끝나면 원래 모드로 되돌림)
	UAbilityTask_ApplyRootMotionMoveToForce* LeapTask = UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
		this,
		NAME_None,
		LandingLocation,
		LeapDuration,
		true,
		MOVE_Flying,
		false,
		GetLeapArcCurve(),
		ERootMotionFinishVelocityMode::SetVelocity,
		FVector::ZeroVector,
		0.f);
	// 몽타주가 없으면 착지 신호도 없음 → 도착 순간을 착지로 보고 타격 후 종료
	LeapTask->OnTimedOut.AddDynamic(this, &ThisClass::OnLeapFinished);
	LeapTask->OnTimedOutAndDestinationReached.AddDynamic(this, &ThisClass::OnLeapFinished);
	LeapTask->ReadyForActivation();
}

FVector USGA_GroundSlam::GetLandingLocation() const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return FVector::ZeroVector;
	}

	const FVector Start = Avatar->GetActorLocation();

	// 커서까지의 방향·거리 (높이 차이는 버림 → 같은 높이에 착지)
	FVector ToAim = GetAimPoint() - Start;
	ToAim.Z = 0.f;

	// 최대 거리보다 멀면 그 방향으로 MaxRange까지만
	ToAim = ToAim.GetClampedToMaxSize(MaxRange);
	return Start + ToAim;
}

UCurveVector* USGA_GroundSlam::GetLeapArcCurve()
{
	if (LeapHeight <= 0.f)
	{
		return nullptr;
	}

	if (!LeapArcCurve || LeapArcCurveHeight != LeapHeight)
	{
		// 가로축 = 진행 비율(0~1), 값 = 직선 경로에서 더할 위치(cm). Z만 써서 위로 볼록하게
		LeapArcCurve = NewObject<UCurveVector>(this);
		FRichCurve& Height = LeapArcCurve->FloatCurves[2];
		Height.AddKey(0.f, 0.f);
		Height.AddKey(0.5f, LeapHeight);
		Height.AddKey(1.f, 0.f);
		Height.AutoSetTangents();
		LeapArcCurveHeight = LeapHeight;
	}
	return LeapArcCurve;
}

void USGA_GroundSlam::OnImpact(FGameplayEventData Payload)
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (bImpacted || !Avatar)
	{
		return;
	}
	bImpacted = true;

	// 마무리 동작을 다 기다리지 않고 RecoveryTime 뒤 종료 (몽타주는 능력이 끝나면 블렌드 아웃)
	if (RecoveryTime > 0.f)
	{
		UAbilityTask_WaitDelay* RecoveryTask = UAbilityTask_WaitDelay::WaitDelay(this, RecoveryTime);
		RecoveryTask->OnFinish.AddDynamic(this, &ThisClass::OnMontageEnded);
		RecoveryTask->ReadyForActivation();
	}

	// 발밑(캡슐 가운데) 기준 반경 타격
	TArray<AActor*> HitActors;
	TArray<AActor*> ShatteredActors;
	ApplyDamageInRadius(
		Avatar->GetActorLocation(),
		Radius,
		DamageMultiplier,
		&HitActors,
		&ShatteredActors);

	// 맞은 적에게 균열 표식 (공통 시간 태그 GE, 시간이 지나면 저절로 빠짐)
	// 표식을 터뜨린 타격은 새 표식을 남기지 않음 → 이번에 [빙결]을 깬 적은 건너뜀
	for (AActor* Target : HitActors)
	{
		if (!ShatteredActors.Contains(Target))
		{
			ApplyTimedTagTo(Target, SGameplayTags::Mark_Crack, CrackDuration);
		}
	}
}

void USGA_GroundSlam::OnLeapFinished()
{
	// 몽타주가 있으면 착지 신호와 몽타주 끝이 처리함
	if (SlamMontage)
	{
		return;
	}

	OnImpact(FGameplayEventData());
	OnMontageEnded();
}

void USGA_GroundSlam::OnMontageEnded()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}
