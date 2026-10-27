#include "Ability/GA/Enemy/SGA_ConductorIntro.h"

#include "TimerManager.h"
#include "Character/BossCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	// 하강 위치를 갱신하는 주기(초)
	constexpr float DescendTickInterval = 1.f / 60.f;
}

void USGA_ConductorIntro::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ABossCharacter* Boss = Cast<ABossCharacter>(GetAvatarActorFromActorInfo());
	if (!Boss)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 착지 지점: 전투 진행 액터가 PlayIntro로 넘긴 경기장 가운데 바닥 (없으면 지금 자리). 캡슐 가운데라 반 높이만큼 위
	DescendEnd = Boss->GetActorLocation();
	FVector Ground;
	if (Boss->ConsumeLeapTargetOverride(Ground))
	{
		DescendEnd = Ground + FVector(0.f, 0.f, Boss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	}
	DescendStart = DescendEnd + FVector(0.f, 0.f, DescendHeight);

	// 내려오는 동안 플레이어를 봄 (좌우만)
	if (const AActor* Player = FindTarget(Boss))
	{
		const float Yaw = (Player->GetActorLocation() - DescendEnd).Rotation().Yaw;
		Boss->SetActorRotation(FRotator(0.f, Yaw, 0.f));
	}

	// 비행 모드: 걷기 모드면 공중에 둔 순간 중력으로 떨어짐. 위치는 아래 타이머가 직접 정함 (엔진 이동 계산을 거치지 않아 경로가 휘지 않음)
	UCharacterMovementComponent* Movement = Boss->GetCharacterMovement();
	Movement->SetMovementMode(MOVE_Flying);
	Movement->Velocity = FVector::ZeroVector;
	Boss->SetActorLocation(DescendStart, false, nullptr, ETeleportType::TeleportPhysics);

	bDescending = true;
	DescendStartTime = GetWorld()->GetTimeSeconds();
	GetWorld()->GetTimerManager().SetTimer(DescendTimer, this, &ThisClass::UpdateDescend, DescendTickInterval, true);
}

void USGA_ConductorIntro::UpdateDescend()
{
	AActor* Boss = GetAvatarActorFromActorInfo();
	if (!IsActive() || !Boss)
	{
		return;
	}

	// 시간 비율 0 → 1을 부드러운 곡선으로 (SmoothStep: 3t² - 2t³): 시작·끝에서 속도 0, 가장 빠를 때도 평균의 1.5배
	// (감속만 하는 곡선은 시작 속도가 평균의 3배라 "툭 떨어지는" 느낌)
	const float Alpha = FMath::Clamp(static_cast<float>((GetWorld()->GetTimeSeconds() - DescendStartTime) / DescendDuration), 0.f, 1.f);
	const float Eased = FMath::SmoothStep(0.f, 1.f, Alpha);
	Boss->SetActorLocation(FMath::Lerp(DescendStart, DescendEnd, Eased), false, nullptr, ETeleportType::TeleportPhysics);

	if (Alpha >= 1.f)
	{
		FinishDescend();
	}
}

void USGA_ConductorIntro::FinishDescend()
{
	GetWorld()->GetTimerManager().ClearTimer(DescendTimer);
	bDescending = false;

	if (ACharacter* Boss = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		Boss->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}

	// 땅에 닿았으니 패턴 시작 (몽타주 → 몸 색 → 첫 박). 거부되면 부모가 EndAbility
	USGA_ConductorPattern::ActivateAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, nullptr);
}

const AActor* USGA_ConductorIntro::FindTarget(const APawn* Boss) const
{
	// 등장 중에는 AI(블랙보드)가 아직 안 돔 → 첫 번째 플레이어를 봄 (지금은 1명. 2인이면 가까운 쪽)
	return UGameplayStatics::GetPlayerPawn(Boss, 0);
}

void USGA_ConductorIntro::OnPatternStarted()
{
	// 몸 색 소개: 주황 → 파랑 → 끔 (첫 박의 고리가 두 색 → 충격파의 몸 색 예고를 처음 봐도 읽을 수 있게)
	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	TimerManager.SetTimer(OrangeTimer, this, &ThisClass::ShowOrange, FMath::Max(OrangeShowTime, 0.01f), false);
	TimerManager.SetTimer(BlueTimer, this, &ThisClass::ShowBlue, FMath::Max(BlueShowTime, 0.01f), false);
	TimerManager.SetTimer(ColorEndTimer, this, &ThisClass::EndColorShow, FMath::Max(ColorEndTime, 0.01f), false);
}

void USGA_ConductorIntro::ShowOrange()
{
	RolledColor = EBulletColor::Orange;
	ShowColorTelegraph();
}

void USGA_ConductorIntro::ShowBlue()
{
	RolledColor = EBulletColor::Blue;
	ShowColorTelegraph();
}

void USGA_ConductorIntro::EndColorShow()
{
	HideColorTelegraph();
}

void USGA_ConductorIntro::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 건너뛰기(능력 취소)로 끝나도 하강·색이 나중에 다시 움직이지 않게. 색 끄기는 부모 EndAbility가 함
	if (const UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		TimerManager.ClearTimer(DescendTimer);
		TimerManager.ClearTimer(OrangeTimer);
		TimerManager.ClearTimer(BlueTimer);
		TimerManager.ClearTimer(ColorEndTimer);
	}

	// 하강 중에 끊기면 비행 모드가 남아 공중에 뜬 채 멈춤 → 걷기로 (건너뛰기는 바로 뒤 착지 지점에 세움)
	if (bDescending)
	{
		bDescending = false;
		if (ACharacter* Boss = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			Boss->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
