#include "Ability/GA/Enemy/SGA_ConductorPattern.h"

#include "AIController.h"
#include "SGameplayTags.h"
#include "TimerManager.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Ability/SAbilityFunctionLibrary.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Bullet/SBulletManager.h"
#include "GameFramework/Pawn.h"

namespace
{
	// 몽타주 위치를 확인하는 주기(초). 발사 오차는 최대 이만큼
	constexpr float PatternTickInterval = 1.f / 60.f;

	// 한 번에 쏘는 탄 수 상한 (설정 실수로 수천 발이 한 프레임에 나가지 않게)
	constexpr int32 MaxBulletsPerVolley = 128;

	// 탄이 보스 몸 안에서 생기지 않게 앞으로 띄우는 거리(cm)
	constexpr float MuzzleOffset = 100.f;
}

USGA_ConductorPattern::USGA_ConductorPattern()
{
	// 공격 중 표시: BT 작업이 이 태그가 사라질 때까지 기다림
	ActivationOwnedTags.AddTag(SGameplayTags::State_Attacking);

	// 다른 공격 중·사망·경직·그로기 중이면 발동 안 함
	ActivationBlockedTags.AddTag(SGameplayTags::State_Attacking);
	ActivationBlockedTags.AddTag(SGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(SGameplayTags::State_Stun);
	ActivationBlockedTags.AddTag(SGameplayTags::State_Groggy);
}

void USGA_ConductorPattern::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 표적: 블랙보드 (BTService_FindNearestPlayer가 씀)
	const APawn* Boss = Cast<APawn>(GetAvatarActorFromActorInfo());
	const AAIController* AIController = Boss ? Cast<AAIController>(Boss->GetController()) : nullptr;
	const UBlackboardComponent* Blackboard = AIController ? AIController->GetBlackboardComponent() : nullptr;
	const AActor* Target = Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor"))) : nullptr;

	// 마지막 발사 전에 모션이 끝나는 설정은 발동 전에 거부 (발사가 잘려 나가는 대신 바로 알 수 있게)
	const float LastShotTime = FirstShotTime + (FMath::Max(VolleyCount, 1) - 1) * VolleyInterval;
	if (!Boss
		|| !CastMontage
		|| !ASBulletManager::Get(this)
		|| !USAbilityFunctionLibrary::IsTargetable(Boss, Target)
		|| LastShotTime >= CastMontage->GetPlayLength() - CastMontage->GetDefaultBlendOutTime()
		|| !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 능력 인스턴스는 재사용되므로 매번 처음 상태로
	NextVolley = 0;

	// 시작할 때 표적을 봄 (좌우만). 패턴 도중에는 돌지 않음 → 예고된 방향 그대로
	StartYaw = (Target->GetActorLocation() - Boss->GetActorLocation()).Rotation().Yaw;
	BulletHeight = Target->GetActorLocation().Z;
	GetAvatarActorFromActorInfo()->SetActorRotation(FRotator(0.f, StartYaw, 0.f));

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		CastMontage);
	// 1차 교훈: 끝나기 시작(BlendOut)·끊김·취소 모두 종료 경로로
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageEnded);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageEnded);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageEnded);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageEnded);
	MontageTask->ReadyForActivation();

	// 재생에 실패하면 ReadyForActivation 안에서 OnCancelled → 이미 종료됨. 타이머를 켜면 끝난 능력이 쏨
	if (!IsActive())
	{
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(PatternTimer, this, &ThisClass::UpdatePattern, PatternTickInterval, true);
	UpdatePattern();
}

void USGA_ConductorPattern::UpdatePattern()
{
	if (!IsActive())
	{
		return;
	}

	const UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance();
	if (!AnimInstance || !AnimInstance->Montage_IsActive(CastMontage))
	{
		OnMontageEnded();
		return;
	}

	// 지난 발사 시각을 모두 처리 (확인 주기 사이에 두 번이 지나가도 놓치지 않게 while)
	const float Position = AnimInstance->Montage_GetPosition(CastMontage);
	while (NextVolley < FMath::Max(VolleyCount, 1) && Position >= FirstShotTime + NextVolley * VolleyInterval)
	{
		FireVolley();
		++NextVolley;
	}
}

void USGA_ConductorPattern::FireVolley()
{
	ASBulletManager* Manager = ASBulletManager::Get(this);
	const AActor* Boss = GetAvatarActorFromActorInfo();
	if (!Manager || !Boss || !HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	FVector Origin = Boss->GetActorLocation();
	Origin.Z = BulletHeight;

	const int32 Count = FMath::Clamp(BulletsPerVolley, 1, MaxBulletsPerVolley);
	const float BaseYaw = StartYaw + (Pattern == EConductorPattern::Spiral ? NextVolley * SpiralStep : 0.f);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		// 부채꼴: -절반 ~ +절반을 고르게 / 방사형·나선: 한 바퀴를 고르게
		float Offset = Index * (360.f / Count);
		if (Pattern == EConductorPattern::AimedFan)
		{
			Offset = Count == 1 ? 0.f : FMath::Lerp(-FanAngle * 0.5f, FanAngle * 0.5f, static_cast<float>(Index) / (Count - 1));
		}

		const FVector Direction = FRotator(0.f, BaseYaw + Offset, 0.f).Vector();
		Manager->SpawnBullet(
			Origin + Direction * MuzzleOffset,
			Direction * BulletSpeed,
			bBlue ? EBulletColor::Blue : EBulletColor::Orange,
			GetAbilitySystemComponentFromActorInfo(),
			DamageMultiplier);
	}
}

void USGA_ConductorPattern::OnMontageEnded()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void USGA_ConductorPattern::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 그로기·BT 중단 등으로 중간에 끝나도 남은 발사가 나가지 않게
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PatternTimer);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
