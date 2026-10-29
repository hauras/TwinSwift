#include "Mirror/SActionReplayComponent.h"

#include "Ability/SAbilitySystemComponent.h"
#include "Character/SMirrorBoss.h"
#include "Mirror/SMirrorDefenseComponent.h"
#include "Kismet/GameplayStatics.h"
#include "SGameplayTags.h"
#include "Ability/SGameplayAbility.h"

namespace
{
	// 회피는 복사하지 않음 (그때의 위협에 대한 반응 → 지금은 이유 없는 움직임). 보스 회피는 방어 컴포넌트가
	bool IsDodge(const FSRecordedAction& Action)
	{
		const USGameplayAbility* AbilityDefaults = Action.AbilityClass ? Cast<USGameplayAbility>(Action.AbilityClass->GetDefaultObject()) : nullptr;
		return AbilityDefaults && AbilityDefaults->InputTag == SGameplayTags::InputTag_Dodge;
	}
}

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
	LastActionTime = Now;
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
		TryFillIdle(GetWorld()->GetTimeSeconds());
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
			LastActionTime = GetWorld()->GetTimeSeconds();
			continue;
		}

		if (IsDodge(Action)) continue;

		// 그 능력을 쓴 몸이 아니면 먼저 바꿈 (교체 기록을 건너뛰었어도 마법사 몸으로 칼을 휘두르지 않게)
		if (Boss->GetActiveSlot() != Action.Slot)
		{
			Boss->SwapTo(Action.Slot);
		}

		if (ASC->PressAbilityByClass(Action.AbilityClass))
		{
			LastActionTime = GetWorld()->GetTimeSeconds();
		}
	}
}

void USActionReplayComponent::TryFillIdle(double Now)
{
	if (Now - LastActionTime < IdleFillSeconds) return;

	ASMirrorBoss* Boss = Cast<ASMirrorBoss>(GetOwner());
	USActionRecorderComponent* SourceRecorder = FindRecorder();
	USAbilitySystemComponent* ASC = Boss ? Cast<USAbilitySystemComponent>(Boss->GetAbilitySystemComponent()) : nullptr;
	if (!Boss || !SourceRecorder || !ASC || Boss->IsDead() || Boss->IsDefending()) return;

	// 한가할 때만 (공격·시전·경직 중이면 끝난 뒤에)
	FGameplayTagContainer BusyTags;
	BusyTags.AddTag(SGameplayTags::State_Stun);
	BusyTags.AddTag(SGameplayTags::State_Groggy);
	if (ASC->HasAnyMatchingGameplayTags(BusyTags)) return;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.IsActive()) return;
	}

	// 기록에 남은 것 전부 (최근 64개) 중 기술만. 아직 따라 하지 않은 최근 것도 포함 ("거울이 배운 기술")
	SourceRecorder->GetActionsBetween(0.0, Now, FillCandidates);
	FillCandidates.RemoveAll([](const FSRecordedAction& Action)
	{
		return Action.Type != ERecordedActionType::Ability || !Action.AbilityClass || IsDodge(Action);
	});

	// 무작위 순서로 보며 지금 쓸 수 있는(쿨다운 아닌) 첫 기술 하나 (쓸 게 없어도 다음 확인은 IdleFillSeconds 뒤)
	LastActionTime = Now;
	for (int32 Index = FillCandidates.Num() - 1; Index > 0; --Index)
	{
		FillCandidates.Swap(Index, FMath::RandRange(0, Index));
	}
	for (const FSRecordedAction& Action : FillCandidates)
	{
		const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(Action.AbilityClass);
		if (!Spec || !Spec->Ability || !Spec->Ability->CanActivateAbility(Spec->Handle, ASC->AbilityActorInfo.Get())) continue;

		// 그 기술을 쓴 몸으로 (쓸 수 있는 걸 확인한 뒤에 바꿔서 헛교체가 없게)
		if (Boss->GetActiveSlot() != Action.Slot)
		{
			Boss->SwapTo(Action.Slot);
		}
		if (ASC->PressAbilityByClass(Action.AbilityClass))
		{
			UE_LOG(LogTemp, Display, TEXT("[거울 채우기] %s"), *GetNameSafe(Action.AbilityClass));
			return;
		}
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
