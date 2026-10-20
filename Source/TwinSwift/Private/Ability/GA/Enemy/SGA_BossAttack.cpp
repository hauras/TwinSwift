#include "Ability/GA/Enemy/SGA_BossAttack.h"

#include "AIController.h"
#include "DrawDebugHelpers.h"
#include "SGameplayTags.h"
#include "TimerManager.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Ability/SAbilityFunctionLibrary.h"
#include "Animation/AnimInstance.h"
#include "Camera/SCameraShake_BossImpact.h"
#include "Kismet/GameplayStatics.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"

namespace
{
	// 공격 시계를 확인하는 주기(초). 판정 오차는 최대 이만큼 (60Hz ≈ 17ms)
	constexpr float AttackTickInterval = 1.f / 60.f;
}

USGA_BossAttack::USGA_BossAttack()
{
	// 공격 중 표시: BT 작업이 이 태그가 사라질 때까지 기다림
	ActivationOwnedTags.AddTag(SGameplayTags::State_Attacking);

	ImpactCameraShake = USCameraShake_BossImpact::StaticClass();

	// 경직·그로기·사망 중이거나 다른 공격 중이면 발동 안 함
	ActivationBlockedTags.AddTag(SGameplayTags::State_Stun);
	ActivationBlockedTags.AddTag(SGameplayTags::State_Groggy);
	ActivationBlockedTags.AddTag(SGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(SGameplayTags::State_Attacking);
}

void USGA_BossAttack::ActivateAbility(
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
	HitIndex = 0;
	ActivateTime = GetWorld()->GetTimeSeconds();

	// 시작할 때 표적을 봄 (좌우만). 공격 도중에는 돌지 않음 → 예고된 범위 그대로 맞음
	if (const AActor* Target = GetTargetActor())
	{
		FVector ToTarget = Target->GetActorLocation() - Character->GetActorLocation();
		ToTarget.Z = 0.f;
		if (!ToTarget.IsNearlyZero())
		{
			Character->SetActorRotation(ToTarget.Rotation());
		}
	}

	if (AttackMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			AttackMontage,
			MontagePlayRate);
		// 1차 교훈: 끝나기 시작(BlendOut)·끊김·취소 모두 종료 경로로
		MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageEnded);
		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageEnded);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageEnded);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageEnded);
		MontageTask->ReadyForActivation();

		// 재생에 실패하면 ReadyForActivation 안에서 OnCancelled → 이미 종료됨. 타이머를 켜면 끝난 능력이 피해를 줌
		if (!IsActive())
		{
			return;
		}
	}

	// 판정은 이 타이머가 공격 시계(몽타주 위치)를 읽어서 함. 타이머 자체는 확인 주기일 뿐 시간을 재지 않음
	GetWorld()->GetTimerManager().SetTimer(AttackTickTimer, this, &ThisClass::OnAttackTick, AttackTickInterval, true);
	OnAttackTick();
}

void USGA_BossAttack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 그로기·BT 중단 등으로 중간에 끝나도 남은 타격·예고가 나가지 않게
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackTickTimer);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

AActor* USGA_BossAttack::GetTargetActor() const
{
	const APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	const AAIController* AIController = Pawn ? Cast<AAIController>(Pawn->GetController()) : nullptr;
	const UBlackboardComponent* Blackboard = AIController ? AIController->GetBlackboardComponent() : nullptr;
	return Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(TargetKeyName)) : nullptr;
}

float USGA_BossAttack::GetAttackTime() const
{
	// 몽타주 재생 위치 = 보스 자신의 시간 (히트스톱·재생 속도가 이미 반영됨)
	const UAnimInstance* AnimInstance = AttackMontage ? GetActorInfo().GetAnimInstance() : nullptr;
	if (AnimInstance && AnimInstance->Montage_IsActive(AttackMontage))
	{
		return AnimInstance->Montage_GetPosition(AttackMontage);
	}

	// 몽타주가 없는 공격만 월드 시간으로 (재생 속도를 곱해 몽타주 시간 기준에 맞춤)
	return static_cast<float>(GetWorld()->GetTimeSeconds() - ActivateTime) * MontagePlayRate;
}

void USGA_BossAttack::OnAttackTick()
{
	if (!IsActive())
	{
		return;
	}

	// 지난 HitTimes를 모두 처리 (확인 주기 사이에 두 타격이 지나가도 놓치지 않게 while)
	const float AttackTime = GetAttackTime();
	while (HitTimes.IsValidIndex(HitIndex) && AttackTime >= HitTimes[HitIndex])
	{
		DoHit();
		++HitIndex;
	}

	if (bDrawDebugTelegraph)
	{
		DrawTelegraph(AttackTime);
	}

	// 다 쳤음. 몽타주가 없으면 여기서 끝 (있으면 몽타주가 끝날 때 끝냄)
	if (!HitTimes.IsValidIndex(HitIndex) && !AttackMontage)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void USGA_BossAttack::DoHit()
{
	// 원 안의 적을 먼저 넓게 찾고(같은 편 제외), 부채꼴이면 각도로 한 번 더 거름
	TArray<AActor*> Candidates;
	FindEnemiesInRadius(GetShapeCenter(), Radius, Candidates);
	for (AActor* Target : Candidates)
	{
		if (IsInShape(Target))
		{
			// 플레이어 공격과 같은 경로 → AttributeSet에서 무적·완벽 회피 판정을 거침
			ApplyDamageTo(Target, DamageMultiplier);
		}
	}

	// 맞는 순간을 알려 줌: 바닥에서 바위가 튐 + 화면 흔들림 (피했어도)
	const FVector Center = GetShapeCenter();
	USAbilityFunctionLibrary::SpawnOneShotEffect(
		this,
		ImpactEffect,
		Center,
		FRotator::ZeroRotator,
		FVector(ImpactEffectScale));

	if (ImpactCameraShake)
	{
		// 범위 안은 최대 세기, ImpactShakeOuterRadius까지 점점 약해짐. 모든 로컬 플레이어 화면에 적용
		UGameplayStatics::PlayWorldCameraShake(
			this,
			ImpactCameraShake,
			Center,
			Radius,
			ImpactShakeOuterRadius);
	}

	// 맞는 순간 표시 (굵은 빨간 선, 0.2초)
	if (bDrawDebugTelegraph)
	{
		DrawArea(Radius, FColor::Red, 10.f, 0.2f);
	}
}

bool USGA_BossAttack::IsInShape(const AActor* Target) const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar || !Target)
	{
		return false;
	}

	// 높이는 무시하고 바닥 평면에서만 판정
	FVector ToTarget = Target->GetActorLocation() - GetShapeCenter();
	ToTarget.Z = 0.f;
	if (ToTarget.SizeSquared() > FMath::Square(Radius))
	{
		return false;
	}

	// 원이거나 중심에 딱 붙어 있으면 맞음
	if (Shape == EBossAttackShape::Circle || ToTarget.IsNearlyZero())
	{
		return true;
	}

	// 부채꼴: 정면과 이루는 각이 절반 각도 이내 (두 단위 벡터의 내적 = cos(사이 각))
	const FVector Forward = Avatar->GetActorForwardVector().GetSafeNormal2D();
	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(FanAngle * 0.5f));
	return FVector::DotProduct(Forward, ToTarget.GetSafeNormal()) >= CosHalfAngle;
}

FVector USGA_BossAttack::GetShapeCenter() const
{
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return FVector::ZeroVector;
	}

	// 발밑 높이(바닥보다 3cm 위) + 앞으로 ForwardOffset
	const float HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	return Character->GetActorLocation()
		+ Character->GetActorForwardVector() * ForwardOffset
		- FVector(0.f, 0.f, HalfHeight - 3.f);
}

void USGA_BossAttack::DrawTelegraph(float AttackTime) const
{
	if (!HitTimes.IsValidIndex(HitIndex))
	{
		return;
	}

	// 지난 타격(첫 타격이면 0초)부터 이번 타격까지 몇 % 왔는지. 판정과 같은 공격 시계
	const float Start = HitIndex > 0 ? HitTimes[HitIndex - 1] : 0.f;
	const float End = HitTimes[HitIndex];
	const float Alpha = FMath::Clamp((AttackTime - Start) / FMath::Max(End - Start, KINDA_SMALL_NUMBER), 0.f, 1.f);

	// 바깥 = 공격 범위, 안쪽 = 남은 시간 (바깥에 닿는 순간 맞음). 다음 확인 때까지 보이게 주기보다 조금 길게
	const float Duration = AttackTickInterval * 1.5f;
	DrawArea(Radius, FColor::Red, 4.f, Duration);
	DrawArea(Radius * Alpha, FColor::Orange, 3.f, Duration);
}

void USGA_BossAttack::DrawArea(float InRadius, const FColor& Color, float Thickness, float Duration) const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return;
	}

	const FVector Center = GetShapeCenter();
	const FVector Forward = Avatar->GetActorForwardVector().GetSafeNormal2D();
	const float HalfAngle = Shape == EBossAttackShape::Fan ? FanAngle * 0.5f : 180.f;
	const bool bDrawEdges = HalfAngle < 180.f;

	// 호를 짧은 선분 32개로. 부채꼴이면 중심에서 양 끝까지 선도
	constexpr int32 Segments = 32;
	FVector Prev = Center + Forward.RotateAngleAxis(-HalfAngle, FVector::UpVector) * InRadius;
	if (bDrawEdges)
	{
		DrawDebugLine(GetWorld(), Center, Prev, Color, false, Duration, 0, Thickness);
	}
	for (int32 Index = 1; Index <= Segments; ++Index)
	{
		const float Angle = -HalfAngle + 2.f * HalfAngle * Index / Segments;
		const FVector Point = Center + Forward.RotateAngleAxis(Angle, FVector::UpVector) * InRadius;
		DrawDebugLine(GetWorld(), Prev, Point, Color, false, Duration, 0, Thickness);
		Prev = Point;
	}
	if (bDrawEdges)
	{
		DrawDebugLine(GetWorld(), Center, Prev, Color, false, Duration, 0, Thickness);
	}
}

void USGA_BossAttack::OnMontageEnded()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}