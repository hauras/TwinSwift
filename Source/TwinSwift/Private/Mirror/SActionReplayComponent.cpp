#include "Mirror/SActionReplayComponent.h"

#include "Ability/SAbilitySystemComponent.h"
#include "Character/SMirrorBoss.h"
#include "Mirror/SMirrorDefenseComponent.h"
#include "Kismet/GameplayStatics.h"
#include "SGameplayTags.h"
#include "Ability/SGameplayAbility.h"

USActionReplayComponent::USActionReplayComponent()
{
	// 전투 중에만 틱 (StartReplay에서 켬)
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void USActionReplayComponent::StartReplay()
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (FirstStartTime < 0.0)
	{
		FirstStartTime = Now;
	}

	// 멈춘 사이(큰 경직)의 행동은 몰아서 하지 않고 건너뜀 (몸은 다음 능력이 맞춤)
	LastReplayTime = Now - Delay;
	bReplaying = true;
	SetComponentTickEnabled(true);
}

void USActionReplayComponent::StopReplay()
{
	bReplaying = false;
	SetComponentTickEnabled(false);
}

void USActionReplayComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const ASMirrorBoss* Boss = Cast<ASMirrorBoss>(GetOwner());
	if (!bReplaying || !Boss || Boss->IsDead())
	{
		StopReplay();
		return;
	}

	// 보스가 나오기 전 기록은 안 따라 함 (그동안은 이동만: 거리를 잡으며 다가옴)
	const double ReplayTime = GetWorld()->GetTimeSeconds() - Delay;
	if (ReplayTime >= FirstStartTime)
	{
		ReplayActions(LastReplayTime, ReplayTime);
	}
	LastReplayTime = ReplayTime;

	if (!Boss->IsDefending()) UpdateMovement(DeltaTime);
}

void USActionReplayComponent::UpdateMovement(float DeltaTime)
{
	ASMirrorBoss* Boss = Cast<ASMirrorBoss>(GetOwner());
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const UAbilitySystemComponent* ASC = Boss ? Boss->GetAbilitySystemComponent() : nullptr;
	if (!Boss || !Player || !ASC) return;

	FGameplayTagContainer BlockTags;
	BlockTags.AddTag(SGameplayTags::State_Groggy);
	BlockTags.AddTag(SGameplayTags::State_Stun);
	BlockTags.AddTag(SGameplayTags::State_Dead);
	if (ASC->HasAnyMatchingGameplayTags(BlockTags)) return;

	// 능력이 켜져 있으면 능력이 움직임을 맡음 (회피 구르기·돌진 베기의 루트 모션, 시전 중 멈춤)
	// 회피·마법은 State.Attacking을 안 붙여서 태그 대신 "켜진 능력이 있나"로 봄
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.IsActive()) return;
	}

	const FVector ToPlayer = Player->GetActorLocation() - Boss->GetActorLocation();
	const float Distance = ToPlayer.Size2D();
	const FVector Direction = ToPlayer.GetSafeNormal2D();

	// 걸을 때는 이동 방향을 봄 (bOrientRotationToMovement)
	if (Boss->GetActiveSlot() == ETwinSlot::Warrior)
	{
		// 붙을 때까지 다가감
		if (Distance > WarriorRange)
		{
			Boss->AddMovementInput(Direction, 1.f);
			return;
		}
	}
	else if (Distance < MageMinRange)
	{
		// 마법사: 붙으면 점멸로 벌림 (뒤로 걷는 애니가 없어 걸어서 물러나면 등을 보임). 점멸 쿨다운이면 걸어서
		USMirrorDefenseComponent* Defense = Boss->GetDefenseComponent();
		if (!Defense || !Defense->TryEvadeFrom(Player->GetActorLocation()))
		{
			Boss->AddMovementInput(-Direction, 1.f);
		}
		return;
	}
	else if (Distance > MageMaxRange)
	{
		Boss->AddMovementInput(Direction, 1.f);
		return;
	}

	// 거리 안: 서서 플레이어 쪽으로 몸을 돌림 (다음 공격 때 갑자기 돌지 않게)
	const FRotator Facing(0.f, Direction.Rotation().Yaw, 0.f);
	Boss->SetActorRotation(FMath::RInterpConstantTo(Boss->GetActorRotation(), Facing, DeltaTime, 720.f));
}

void USActionReplayComponent::ReplayActions(double FromTime, double ToTime)
{
	ASMirrorBoss* Boss = Cast<ASMirrorBoss>(GetOwner());
	USActionRecorderComponent* SourceRecorder = FindRecorder();
	// 방어 중 지나간 복사 행동은 건너뜀. 커서는 Tick에서 계속 전진.
	if (!Boss || !SourceRecorder || Boss->IsDefending()) return;

	USAbilitySystemComponent* ASC = Cast<USAbilitySystemComponent>(Boss->GetAbilitySystemComponent());
	if (!ASC) return;

	SourceRecorder->GetActionsBetween(FromTime, ToTime, PendingActions);
	for (const FSRecordedAction& Action : PendingActions)
	{
		// 앞 행동으로 죽었을 수 있음 (반사 피해 등) → 매번 다시 확인
		if (Boss->IsDead()) return;

		if (Action.Type == ERecordedActionType::Swap)
		{
			Boss->SwapTo(Action.Slot);
			continue;
		}

		// 회피는 복사하지 않음 (그때의 위협에 대한 반응 → 지금은 이유 없는 움직임). 보스 회피는 방어 컴포넌트가
		const USGameplayAbility* AbilityDefaults = Action.AbilityClass ? Cast<USGameplayAbility>(Action.AbilityClass->GetDefaultObject()) : nullptr;
		if (AbilityDefaults && AbilityDefaults->InputTag == SGameplayTags::InputTag_Dodge)
		{
			continue;
		}

		// 그 능력을 쓴 몸이 아니면 먼저 바꿈 (교체 기록을 건너뛰었어도 마법사 몸으로 칼을 휘두르지 않게)
		if (Boss->GetActiveSlot() != Action.Slot)
		{
			Boss->SwapTo(Action.Slot);
		}

		ASC->PressAbilityByClass(Action.AbilityClass);
	}
}

USActionRecorderComponent* USActionReplayComponent::FindRecorder()
{
	if (USActionRecorderComponent* Found = Recorder.Get())
	{
		return Found;
	}

	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	Recorder = Player ? Player->FindComponentByClass<USActionRecorderComponent>() : nullptr;
	return Recorder.Get();
}
