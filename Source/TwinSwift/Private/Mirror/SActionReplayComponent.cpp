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

	// 큰 경직 등으로 멈추면 반전도 처음부터 (예고 빛남도 꺼짐). 다음 반전 시각은 그대로
	ReverseState = EReverseState::None;
	ReverseSteps.Reset();
}

void USActionReplayComponent::EnterPhaseTwo()
{
	if (bPhaseTwo) return;

	bPhaseTwo = true;
	const double Now = GetWorld()->GetTimeSeconds();
	Delay = PhaseTwoDelay;

	// 지연이 줄면 그 사이 기록이 한꺼번에 몰림 → 줄어든 구간은 건너뜀
	LastReplayTime = FMath::Max(LastReplayTime, Now - Delay);
	NextReverseTime = Now + ReverseInterval;
	UE_LOG(LogTemp, Display, TEXT("[거울] 2페이즈: 지연 %.1f초, %.0f초마다 거울 반전"), Delay, ReverseInterval);
}

bool USActionReplayComponent::StartIntro()
{
	const double Now = GetWorld()->GetTimeSeconds();
	ReverseSteps.Reset();

	PendingActions.Reset();
	
	if (USActionRecorderComponent* SoureRecorder = FindRecorder())
	{
		SoureRecorder->GetActionsBetween(Now - IntroLookBack, Now, PendingActions);

		PendingActions.RemoveAll([](const FSRecordedAction& Action)
		{
			return Action.Type != ERecordedActionType::Ability || !Action.AbilityClass || IsDodge(Action);
		});

		const int32 RemoveCount = PendingActions.Num() - IntroStepCount;
		if (RemoveCount > 0)
		{
			PendingActions.RemoveAt(0, RemoveCount);
		}
	}

	for (int32 Index = 0; Index < PendingActions.Num(); ++Index)
	{
		const FSRecordedAction& Action = PendingActions[Index];
		const double PlayAt = Index == 0 ? Now + IntroStartDelay : ReverseSteps.Last().PlayAt + FMath::Min(Action.Time - PendingActions[Index - 1].Time, static_cast<double>(IntroMaxGap));

		FReverseStep& Step = ReverseSteps.AddDefaulted_GetRef();
		Step.AbilityClass = Action.AbilityClass;
		Step.Slot = Action.Slot;
		Step.PlayAt = PlayAt;
	}

	const bool bUsedDefault = ReverseSteps.IsEmpty();
	if (bUsedDefault)
	{
		for (const FSMirrorIntroStep& Default : DefaultIntroSteps)
		{
			if (!Default.AbilityClass) continue;

			const double PlayAt = ReverseSteps.IsEmpty() ? Now + IntroStartDelay : ReverseSteps.Last().PlayAt + Default.Gap;

			FReverseStep& Step = ReverseSteps.AddDefaulted_GetRef();
			Step.AbilityClass = Default.AbilityClass;
			Step.Slot = Default.Slot;
			Step.PlayAt = PlayAt;
		}
	}

	// 기록도 기본 순서도 없음 → 등장 없이 바로 전투 (전투 진행 액터가 false를 보고 넘어감)
	if (ReverseSteps.IsEmpty()) return false;

	ReverseIndex = 0;
	bIntroPlaying = true;
	SetComponentTickEnabled(true);
	UE_LOG(LogTemp, Display, TEXT("[거울 등장] 기술 %d개 (%s)"), ReverseSteps.Num(), bUsedDefault ? TEXT("기본 순서") : TEXT("기록"));
	return true;
}

bool USActionReplayComponent::IsIntroPlaying() const
{
	return bIntroPlaying;
}

void USActionReplayComponent::StopIntro()
{
	bIntroPlaying = false;
	ReverseSteps.Reset();
	ReverseIndex = 0;

	if (!bReplaying)
	{
		SetComponentTickEnabled(false);
	}
}

void USActionReplayComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const ASMirrorBoss* Boss = Cast<ASMirrorBoss>(GetOwner());
	const double Now = GetWorld()->GetTimeSeconds();

	if (bIntroPlaying)
	{
		if (!Boss || Boss->IsDead())
		{
			StopIntro();
			return;
		}
		UpdateIntro(Now);
		return;
	}
	
	if (!bReplaying || !Boss || Boss->IsDead())
	{
		StopReplay();
		return;
	}

	// 거울 반전 중이면 따라 하기·채우기는 쉼 (커서는 계속 나아가 끝난 뒤 몰아서 하지 않음)
	const bool bReversing = UpdateReverse(Now);

	// 보스가 나오기 전 기록은 안 따라 함 (그동안은 이동만: 거리를 잡으며 다가옴)
	const double ReplayTime = Now - Delay;
	if (ReplayTime >= FirstStartTime && !bReversing)
	{
		ReplayActions(LastReplayTime, ReplayTime);
		TryFillIdle(Now);
	}
	LastReplayTime = ReplayTime;

	// 예고 중에는 멈춰 섬 (빛나는 동안 "곧 온다"가 보이게)
	if (!Boss->IsDefending() && !IsReverseWarning()) UpdateMovement(DeltaTime);
}

bool USActionReplayComponent::IsBusy() const
{
	const ASMirrorBoss* Boss = Cast<ASMirrorBoss>(GetOwner());
	const UAbilitySystemComponent* ASC = Boss ? Boss->GetAbilitySystemComponent() : nullptr;
	if (!ASC || Boss->IsDead() || Boss->IsDefending()) return true;

	FGameplayTagContainer BusyTags;
	BusyTags.AddTag(SGameplayTags::State_Stun);
	BusyTags.AddTag(SGameplayTags::State_Groggy);
	if (ASC->HasAnyMatchingGameplayTags(BusyTags)) return true;

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.IsActive()) return true;
	}
	return false;
}

bool USActionReplayComponent::UpdateReverse(double Now)
{
	if (!bPhaseTwo) return false;

	switch (ReverseState)
	{
	case EReverseState::None:
		// 때가 됐고 한가하면 예고 시작 (공격 중이면 끝날 때까지 기다림)
		if (Now < NextReverseTime || IsBusy()) return false;
		ReverseState = EReverseState::Warning;
		ReverseStateTime = Now;
		UE_LOG(LogTemp, Display, TEXT("[거울 반전] 예고"));
		return true;

	case EReverseState::Warning:
		if (Now - ReverseStateTime < ReverseWarnTime) return true;
		if (!BuildReverseSteps(Now))
		{
			// 거꾸로 쓸 기술이 없음 (플레이어가 최근에 기술을 안 씀) → 짧게 쉬고 다시
			ReverseState = EReverseState::None;
			NextReverseTime = Now + ReverseWindow;
			return false;
		}
		ReverseState = EReverseState::Playing;
		return true;

	case EReverseState::Playing:
		
		if (!PlaySteps(Now)) return true;

		ReverseState = EReverseState::None;
		NextReverseTime = Now + ReverseInterval;
		return false;
	}
	return false;
}

bool USActionReplayComponent::PlaySteps(double Now)
{
	while (ReverseSteps.IsValidIndex(ReverseIndex) && ReverseSteps[ReverseIndex].PlayAt <= Now)
	{
		FReverseStep& Step = ReverseSteps[ReverseIndex];
		if (bIntroPlaying && ReverseIndex > 0 && ReverseSteps[ReverseIndex - 1].AbilityClass != Step.AbilityClass && IsBusy())
		{
			Step.PlayAt = Now;
			break;
			
		}
		if (PressAction(Step.AbilityClass, Step.Slot))
		{
			UE_LOG(LogTemp, Display, TEXT("[거울 반전] %d/%d 실행 %s"), ReverseIndex + 1, ReverseSteps.Num(), *GetNameSafe(Step.AbilityClass));
		}
		else if (Now - Step.PlayAt < ReverseRetryTime)
		{
			// 거꾸로 놓으면 앞 기술 동작이 안 끝났는데 다음 차례가 올 수 있음 → 다음 틱에 다시 (뒤 순서도 같이 기다림)
			break;
		}
		else
		{
			UE_LOG(LogTemp, Display, TEXT("[거울 반전] %d/%d 건너뜀 %s (%.1f초 동안 못 씀)"), ReverseIndex + 1, ReverseSteps.Num(), *GetNameSafe(Step.AbilityClass), ReverseRetryTime);
		}
		++ReverseIndex;
	}
	return !ReverseSteps.IsValidIndex(ReverseIndex);
	
}

void USActionReplayComponent::UpdateIntro(double Now)
{
	if (!PlaySteps(Now)) return;

	if (IsBusy()) return;

	bIntroPlaying = false;
	ReverseSteps.Reset();
	ReverseIndex = 0;
	UE_LOG(LogTemp, Display, TEXT("[거울 등장] 끝"));
}

bool USActionReplayComponent::BuildReverseSteps(double Now)
{
	USActionRecorderComponent* SourceRecorder = FindRecorder();
	if (!SourceRecorder) return false;

	SourceRecorder->GetActionsBetween(Now - ReverseWindow, Now, PendingActions);
	PendingActions.RemoveAll([](const FSRecordedAction& Action)
	{
		return Action.Type != ERecordedActionType::Ability || !Action.AbilityClass || IsDodge(Action);
	});
	if (PendingActions.IsEmpty()) return false;

	// 마지막 기술부터 거꾸로, 간격은 원래대로 (마지막에 쓴 것이 바로, 처음 쓴 것이 마지막에)
	const double LastTime = PendingActions.Last().Time;
	ReverseSteps.Reset();
	for (int32 Index = PendingActions.Num() - 1; Index >= 0; --Index)
	{
		const FSRecordedAction& Action = PendingActions[Index];
		FReverseStep& Step = ReverseSteps.AddDefaulted_GetRef();
		Step.AbilityClass = Action.AbilityClass;
		Step.Slot = Action.Slot;
		Step.PlayAt = Now + (LastTime - Action.Time);
	}
	ReverseIndex = 0;
	return true;
}

bool USActionReplayComponent::PressAction(TSubclassOf<UGameplayAbility> AbilityClass, ETwinSlot Slot)
{
	ASMirrorBoss* Boss = Cast<ASMirrorBoss>(GetOwner());
	USAbilitySystemComponent* ASC = Boss ? Cast<USAbilitySystemComponent>(Boss->GetAbilitySystemComponent()) : nullptr;
	if (!ASC || Boss->IsDead()) return false;

	// 그 능력을 쓴 몸이 아니면 바꿈 (마법사 몸으로 칼을 휘두르지 않게)
	// 바꾸면 진행 중 능력이 끊기므로, 쿨다운 등으로 못 쓸 기술이면 바꾸지 않음
	if (Boss->GetActiveSlot() != Slot)
	{
		if (!ASC->CanActivateAbilityByClass(AbilityClass)) return false;
		Boss->SwapTo(Slot);
	}
	if (!ASC->PressAbilityByClass(AbilityClass)) return false;

	LastActionTime = GetWorld()->GetTimeSeconds();
	return true;
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

		// 교체 기록을 건너뛰었어도 그 능력을 쓴 몸으로
		PressAction(Action.AbilityClass, Action.Slot);
	}
}

void USActionReplayComponent::TryFillIdle(double Now)
{
	if (Now - LastActionTime < IdleFillSeconds) return;

	ASMirrorBoss* Boss = Cast<ASMirrorBoss>(GetOwner());
	USActionRecorderComponent* SourceRecorder = FindRecorder();
	USAbilitySystemComponent* ASC = Boss ? Cast<USAbilitySystemComponent>(Boss->GetAbilitySystemComponent()) : nullptr;

	// 한가할 때만 (공격·시전·경직 중이면 끝난 뒤에)
	if (!SourceRecorder || !ASC || IsBusy()) return;

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
		if (!ASC->CanActivateAbilityByClass(Action.AbilityClass)) continue;

		// 쓸 수 있는 걸 확인한 뒤에 몸을 바꿔서 헛교체가 없게
		if (PressAction(Action.AbilityClass, Action.Slot))
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
