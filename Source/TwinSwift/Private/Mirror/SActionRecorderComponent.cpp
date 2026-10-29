#include "Mirror/SActionRecorderComponent.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Ability/SAbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"
#include "Character/PlayerCharacter.h"
#include "State/SPlayerState.h"

namespace
{
	// [확인용] 1이면 "N초 전 내 위치"에 보라 구를 그리고, 행동이 기록될 때 로그. 거울 보스가 볼 자리를 미리 보는 것
	TAutoConsoleVariable<int32> CVarShowGhost(
		TEXT("ts.Mirror.ShowGhost"),
		0,
		TEXT("1: 기록 컴포넌트가 GhostDelay초 전 위치에 구를 그리고 행동 기록을 로그로"));

	// [확인용] 구가 몇 초 늦게 따라올지 (거울 보스의 재생 지연과 같게 3초)
	TAutoConsoleVariable<float> CVarGhostDelay(
		TEXT("ts.Mirror.GhostDelay"),
		3.f,
		TEXT("ShowGhost 구의 지연(초)"));
}

USActionRecorderComponent::USActionRecorderComponent()
{
	// 매 프레임이 아니라 기록 간격마다 (틱 한 번 = 위치 한 칸)
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = SampleInterval;
}

void USActionRecorderComponent::BeginPlay()
{
	Super::BeginPlay();
	SetComponentTickInterval(SampleInterval);

	Poses.Init(FMath::CeilToInt(HistorySeconds / SampleInterval) + 1);
	Actions.Init(64);

	if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetOwner()))
	{
		SwapHandle = PlayerCharacter->OnActiveSlotChanged.AddUObject(this, &ThisClass::OnActiveSlotChanged);
	}
	
}

void USActionRecorderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (!Owner) return;

	if (!WarriorAbilityHandle.IsValid())
	{
		const APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetOwner());
		const ASPlayerState* PlayerState = PlayerCharacter ? PlayerCharacter->GetPlayerState<ASPlayerState>() : nullptr;

		USAbilitySystemComponent* WarriorASC = PlayerState ? PlayerState->GetSAbilitySystemComponent(ETwinSlot::Warrior) : nullptr;
		USAbilitySystemComponent* MageASC = PlayerState ? PlayerState->GetSAbilitySystemComponent(ETwinSlot::Mage) : nullptr;

		if (WarriorASC && MageASC)
		{
			WarriorAbilityHandle = WarriorASC->OnAbilityPressed.AddUObject(this, &ThisClass::HandleAbilityPressed, ETwinSlot::Warrior);
			MageAbilityHandle = MageASC->OnAbilityPressed.AddUObject(this, &ThisClass::HandleAbilityPressed, ETwinSlot::Mage);
		}
	}

	FSRecordedPose Pose;
	Pose.Time = GetWorld()->GetTimeSeconds();
	Pose.Location = Owner->GetActorLocation();
	Pose.Yaw = Owner->GetActorRotation().Yaw;
	Poses.Add(Pose);

	// [확인용] N초 전 위치에 구 + 보던 방향. 다음 틱까지 남게 (틱 간격만큼). 플레이어 기록만 (거울 보스도 위치를 기록함)
	if (CVarShowGhost.GetValueOnGameThread() != 0 && Cast<APlayerCharacter>(Owner))
	{
		FSRecordedPose Ghost;
		if (GetPoseAt(Pose.Time - CVarGhostDelay.GetValueOnGameThread(), Ghost))
		{
			const FColor Purple(160, 80, 255);
			DrawDebugSphere(GetWorld(), Ghost.Location, 40.f, 12, Purple, false, SampleInterval + 0.02f);
			DrawDebugDirectionalArrow(
				GetWorld(),
				Ghost.Location,
				Ghost.Location + FRotator(0.f, Ghost.Yaw, 0.f).Vector() * 120.f,
				40.f,
				Purple,
				false,
				SampleInterval + 0.02f);
		}
	}
}

void USActionRecorderComponent::HandleAbilityPressed(TSubclassOf<UGameplayAbility> AbilityClass, ETwinSlot Slot)
{
	// 발동이 아니라 키 누름을 적음 → 콤보 2타도 남고, 넉백처럼 맞아서 켜지는 능력은 누름이 없어 저절로 빠짐
	FSRecordedAction Action;
	Action.Time = GetWorld()->GetTimeSeconds();
	Action.Type = ERecordedActionType::Ability;
	Action.AbilityClass = AbilityClass;
	Action.Slot = Slot;
	Actions.Add(Action);

	if (CVarShowGhost.GetValueOnGameThread() != 0)
	{
		UE_LOG(LogTemp, Display, TEXT("[거울 기록] %.2f초 능력: %s"), Action.Time, *GetNameSafe(Action.AbilityClass));
	}
}

void USActionRecorderComponent::OnActiveSlotChanged(ETwinSlot NewSlot)
{
	FSRecordedAction Action;
	Action.Time = GetWorld()->GetTimeSeconds();
	Action.Type = ERecordedActionType::Swap;
	Action.Slot = NewSlot;
	Actions.Add(Action);

	if (CVarShowGhost.GetValueOnGameThread() != 0)
	{
		UE_LOG(LogTemp, Display, TEXT("[거울 기록] %.2f초 교체 → %s"), Action.Time, NewSlot == ETwinSlot::Warrior ? TEXT("전사") : TEXT("마법사"));
	}
}

bool USActionRecorderComponent::GetPoseAt(double Time, FSRecordedPose& OutPose) const
{
	if (Poses.Num() < 2) return false;

	if (Time < Poses.Get(0).Time)
	{
		OutPose = Poses.Get(0);
		return true;
	}
	if (Time > Poses.Get(Poses.Num()-1).Time)
	{
		OutPose = Poses.Get(Poses.Num()-1);
		return true;
	}
	for (int32 Index = 0; Index<=Poses.Num() - 2; ++Index)
	{
		const FSRecordedPose& Before  = Poses.Get(Index);
		const FSRecordedPose& After  = Poses.Get(Index + 1);
		if (Before.Time <= Time && Time <= After.Time)
		{
			const double Span = After.Time - Before.Time;
			const double Alpha = Span > 0 ? (Time - Before.Time) / Span : 0.0;

			OutPose.Time = Time;
			OutPose.Location = FMath::Lerp(Before.Location, After.Location, Alpha);
			const float Delta = FMath::FindDeltaAngleDegrees(Before.Yaw, After.Yaw);
			OutPose.Yaw = Before.Yaw + Delta * Alpha;
			
			return true;
		}
	}
	return false;
}

void USActionRecorderComponent::GetActionsBetween(double FromTime, double ToTime,
	TArray<FSRecordedAction>& OutActions) const
{
	OutActions.Reset();

	for (int32 Index = 0; Index <Actions.Num(); ++Index)
	{
		const FSRecordedAction& Action = Actions.Get(Index);

		if (FromTime < Action.Time && Action.Time <= ToTime)
		{
			OutActions.Add(Action);
		}
	}
}

void USActionRecorderComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetOwner()))
	{
		PlayerCharacter->OnActiveSlotChanged.Remove(SwapHandle);

		const ASPlayerState* PlayerState = PlayerCharacter->GetPlayerState<ASPlayerState>();
		USAbilitySystemComponent* WarriorASC = PlayerState ? PlayerState->GetSAbilitySystemComponent(ETwinSlot::Warrior) : nullptr;
		USAbilitySystemComponent* MageASC = PlayerState ? PlayerState->GetSAbilitySystemComponent(ETwinSlot::Mage) : nullptr;
		if (WarriorASC)
		{
			WarriorASC->OnAbilityPressed.Remove(WarriorAbilityHandle);
		}
		if (MageASC)
		{
			MageASC->OnAbilityPressed.Remove(MageAbilityHandle);
		}
	}

	Super::EndPlay(EndPlayReason);
}
