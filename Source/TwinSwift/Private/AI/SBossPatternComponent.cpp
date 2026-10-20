#include "AI/SBossPatternComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarShowPatternScores(
		TEXT("ts.Boss.ShowPatternScores"),
		0,
		TEXT("1이면 보스가 패턴을 고를 때마다 후보별 점수와 고른 패턴을 머리 위에 표시"));

	// 같은 값이 가장 길게 이어진 횟수
	int32 LongestRun(const TArray<int32>& Picks)
	{
		int32 Longest = 0;
		int32 Run = 0;
		for (int32 Index = 0; Index < Picks.Num(); ++Index)
		{
			Run = (Index > 0 && Picks[Index] == Picks[Index - 1]) ? Run + 1 : 1;
			Longest = FMath::Max(Longest, Run);
		}
		return Longest;
	}

	// 같은 값이 3번 이상 이어진 구간 수 (플레이어가 "또?"라고 느끼는 횟수)
	int32 CountRunsOf3OrMore(const TArray<int32>& Picks)
	{
		int32 Count = 0;
		int32 Run = 0;
		for (int32 Index = 0; Index < Picks.Num(); ++Index)
		{
			Run = (Index > 0 && Picks[Index] == Picks[Index - 1]) ? Run + 1 : 1;
			if (Run == 3)
			{
				++Count;
			}
		}
		return Count;
	}
}

USBossPatternComponent::USBossPatternComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USBossPatternComponent::BeginPlay()
{
	Super::BeginPlay();

	// 판마다 다른 순서 (시뮬레이션은 고정 시드로 따로)
	State.Random.Initialize(FMath::Rand());
	RefillBag(State);
}

FGameplayTag USBossPatternComponent::SelectPattern(float DistanceToTarget)
{
	const bool bShowScores = CVarShowPatternScores.GetValueOnGameThread() != 0;

	TArray<float> Scores;
	const int32 Chosen = SelectIndex(State, DistanceToTarget, true, bShowScores ? &Scores : nullptr);

	if (bShowScores)
	{
		DrawScores(Scores, Chosen, DistanceToTarget);
	}

	return Patterns.IsValidIndex(Chosen) ? Patterns[Chosen].AbilityTag : FGameplayTag();
}

int32 USBossPatternComponent::SelectIndex(FSelectState& InState, float DistanceToTarget, bool bCheckAbility, TArray<float>* OutScores) const
{
	// 1~2) 거르기 + 점수
	TArray<float> Scores;
	Scores.SetNumZeroed(Patterns.Num());
	float BestScore = 0.f;
	int32 BestIndex = INDEX_NONE;
	for (int32 Index = 0; Index < Patterns.Num(); ++Index)
	{
		Scores[Index] = ScorePattern(Index, InState, DistanceToTarget, bCheckAbility);
		if (Scores[Index] > BestScore)
		{
			BestScore = Scores[Index];
			BestIndex = Index;
		}
	}

	if (OutScores)
	{
		*OutScores = Scores;
	}

	// 쓸 수 있는 패턴이 없음 → BT가 Move To로 넘어감
	if (BestIndex == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	// 3) 최고점 근처만 후보
	const float Threshold = BestScore * CandidateRatio;
	auto IsCandidate = [&Scores, Threshold](int32 Index)
	{
		return Scores[Index] > 0.f && Scores[Index] >= Threshold;
	};

	// 4) 가방에서 앞쪽부터 꺼내다가 처음 나오는 후보. 가방에 후보가 없으면 다시 채워서 한 번 더
	int32 Chosen = INDEX_NONE;
	for (int32 Attempt = 0; Attempt < 2 && Chosen == INDEX_NONE; ++Attempt)
	{
		const int32 BagPosition = InState.Bag.IndexOfByPredicate(IsCandidate);
		if (BagPosition != INDEX_NONE)
		{
			Chosen = InState.Bag[BagPosition];
			InState.Bag.RemoveAt(BagPosition);
		}
		else
		{
			RefillBag(InState);
		}
	}

	// 가방을 채워도 없으면(있을 수 없지만 안전하게) 최고점
	if (Chosen == INDEX_NONE)
	{
		Chosen = BestIndex;
	}

	if (InState.Bag.IsEmpty())
	{
		RefillBag(InState);
	}

	// 최근 사용 기록 (감점 개수만큼만 기억)
	InState.History.Insert(Chosen, 0);
	if (InState.History.Num() > RecentPenalties.Num())
	{
		InState.History.SetNum(RecentPenalties.Num());
	}

	return Chosen;
}

float USBossPatternComponent::ScorePattern(int32 Index, const FSelectState& InState, float DistanceToTarget, bool bCheckAbility) const
{
	const FSBossPattern& Pattern = Patterns[Index];
	if (!Pattern.AbilityTag.IsValid() || DistanceToTarget < Pattern.MinRange || DistanceToTarget > Pattern.MaxRange)
	{
		return 0.f;
	}
	if (bCheckAbility && !CanActivatePattern(Pattern))
	{
		return 0.f;
	}

	// 거리 점수: 선호 거리에서 1 → 범위 끝에서 MinDistanceScore (선호 거리 앞뒤를 각각 0~1로)
	const float Side = DistanceToTarget <= Pattern.PreferredRange
		? Pattern.PreferredRange - Pattern.MinRange
		: Pattern.MaxRange - Pattern.PreferredRange;
	const float Deviation = FMath::Clamp(FMath::Abs(DistanceToTarget - Pattern.PreferredRange) / FMath::Max(Side, 1.f), 0.f, 1.f);
	const float DistanceScore = FMath::Lerp(1.f, MinDistanceScore, Deviation);

	// 최근 사용 감점: 직전이면 [0], 그 전이면 [1] …
	float RecentScore = 1.f;
	const int32 HistoryPosition = InState.History.IndexOfByKey(Index);
	if (RecentPenalties.IsValidIndex(HistoryPosition))
	{
		RecentScore = RecentPenalties[HistoryPosition];
	}

	return Pattern.Weight * DistanceScore * RecentScore;
}

bool USBossPatternComponent::CanActivatePattern(const FSBossPattern& Pattern) const
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC)
	{
		return false;
	}

	// 태그가 같은 능력 중 하나라도 지금 발동할 수 있으면 (쿨다운·차단 태그는 GAS의 CanActivateAbility가 확인)
	TArray<FGameplayAbilitySpec*> Specs;
	ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(FGameplayTagContainer(Pattern.AbilityTag), Specs, false);
	for (const FGameplayAbilitySpec* Spec : Specs)
	{
		if (Spec && Spec->Ability && Spec->Ability->CanActivateAbility(Spec->Handle, ASC->AbilityActorInfo.Get()))
		{
			return true;
		}
	}
	return false;
}

void USBossPatternComponent::RefillBag(FSelectState& InState) const
{
	InState.Bag.Reset(Patterns.Num());
	for (int32 Index = 0; Index < Patterns.Num(); ++Index)
	{
		InState.Bag.Add(Index);
	}

	// 피셔-예이츠 섞기: 뒤에서부터 앞쪽 아무 칸과 바꿈 → 모든 순서가 같은 확률
	for (int32 Index = InState.Bag.Num() - 1; Index > 0; --Index)
	{
		InState.Bag.Swap(Index, InState.Random.RandRange(0, Index));
	}

	// 가방 경계 연속 방지: 직전에 쓴 패턴이 새 가방 맨 앞이면 뒤쪽 아무 칸과 바꿈
	if (InState.Bag.Num() > 1 && !InState.History.IsEmpty() && InState.Bag[0] == InState.History[0])
	{
		InState.Bag.Swap(0, InState.Random.RandRange(1, InState.Bag.Num() - 1));
	}
}

void USBossPatternComponent::DrawScores(const TArray<float>& Scores, int32 Chosen, float DistanceToTarget) const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// 디버그 글꼴에 한글이 없어서 영어로
	FString Text = FString::Printf(TEXT("dist %.0f\n"), DistanceToTarget);
	for (int32 Index = 0; Index < Patterns.Num(); ++Index)
	{
		Text += FString::Printf(
			TEXT("%s %.2f%s\n"),
			*Patterns[Index].AbilityTag.GetTagName().ToString(),
			Scores.IsValidIndex(Index) ? Scores[Index] : 0.f,
			Index == Chosen ? TEXT("  <<") : TEXT(""));
	}

	DrawDebugString(GetWorld(), FVector(0.f, 0.f, 320.f), Text, const_cast<AActor*>(Owner), FColor::Yellow, 2.f, true, 1.3f);
}

UAbilitySystemComponent* USBossPatternComponent::GetASC() const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
}

void USBossPatternComponent::RunSimulation(int32 Count, int32 Seed) const
{
	if (Patterns.IsEmpty() || Count <= 0)
	{
		return;
	}

	// 두 경우로 나눠 잼
	// ① 고를 패턴이 2개 이상인 거리만: 고를 게 있을 때 같은 패턴이 몇 번 이어지는가 (선택기가 실제로 푸는 문제)
	// ② 전체 거리: 한 패턴만 닿는 거리에서는 연속이 강제로 생김 (바로 앞 플레이어에게 닿지 않는 공격을 쓰면 안 되므로 정상)
	float FullMin = TNumericLimits<float>::Max();
	float FullMax = 0.f;
	for (const FSBossPattern& Pattern : Patterns)
	{
		FullMin = FMath::Min(FullMin, Pattern.MinRange);
		FullMax = FMath::Max(FullMax, Pattern.MaxRange);
	}

	auto PatternsInRange = [this](float Distance)
	{
		TArray<int32> InRange;
		for (int32 Index = 0; Index < Patterns.Num(); ++Index)
		{
			if (Distance >= Patterns[Index].MinRange && Distance <= Patterns[Index].MaxRange)
			{
				InRange.Add(Index);
			}
		}
		return InRange;
	};

	auto Distribution = [this](const TArray<int32>& Picks)
	{
		FString Result;
		for (int32 Index = 0; Index < Patterns.Num(); ++Index)
		{
			const int32 Hits = Picks.FilterByPredicate([Index](int32 Pick) { return Pick == Index; }).Num();
			Result += FString::Printf(
				TEXT("%s %.1f%%  "),
				*Patterns[Index].AbilityTag.GetTagName().ToString(),
				Picks.IsEmpty() ? 0.f : 100.f * Hits / Picks.Num());
		}
		return Result;
	};

	auto RunRange = [this, Count, Seed, FullMin, FullMax, &Distribution, &PatternsInRange](const TCHAR* Label, bool bRequireChoice)
	{
		const float MinDistance = FullMin;
		const float MaxDistance = FullMax;

		// 선택기: 실제 게임과 같은 SelectIndex, 상태만 복사본 (게임 중인 가방·기록은 그대로)
		FSelectState SimState;
		SimState.Random.Initialize(Seed);
		RefillBag(SimState);

		// 비교: 그 거리에 닿는 패턴 중 완전 무작위
		FRandomStream PureRandom(Seed);
		FRandomStream DistanceRandom(Seed + 1);

		TArray<int32> SelectorPicks;
		TArray<int32> RandomPicks;
		for (int32 Step = 0; Step < Count; ++Step)
		{
			// ①이면 고를 게 2개 이상인 거리가 나올 때까지 다시 뽑음 (그런 거리가 없으면 1000번 뒤 포기)
			float Distance = DistanceRandom.FRandRange(MinDistance, MaxDistance);
			TArray<int32> InRange = PatternsInRange(Distance);
			for (int32 Retry = 0; bRequireChoice && InRange.Num() < 2 && Retry < 1000; ++Retry)
			{
				Distance = DistanceRandom.FRandRange(MinDistance, MaxDistance);
				InRange = PatternsInRange(Distance);
			}
			if (bRequireChoice && InRange.Num() < 2)
			{
				UE_LOG(LogTemp, Display, TEXT("[패턴 시뮬레이션] %s: 패턴 2개 이상이 닿는 거리가 없음"), Label);
				return;
			}

			const int32 Picked = SelectIndex(SimState, Distance, false, nullptr);
			if (Picked != INDEX_NONE)
			{
				SelectorPicks.Add(Picked);
			}

			if (!InRange.IsEmpty())
			{
				RandomPicks.Add(InRange[PureRandom.RandRange(0, InRange.Num() - 1)]);
			}
		}

		UE_LOG(LogTemp, Display, TEXT("[패턴 시뮬레이션] %s: 거리 %.0f~%.0f 무작위 %d번, 시드 %d, 쿨다운 무시"), Label, MinDistance, MaxDistance, Count, Seed);
		UE_LOG(LogTemp, Display, TEXT("[패턴 시뮬레이션]   (같은 패턴 3연속 이상 = 플레이어가 \"또?\"라고 느끼는 횟수)"));
		UE_LOG(LogTemp, Display, TEXT("[패턴 시뮬레이션]   선택기    : 최대 연속 %d, 3연속 이상 %d번 | %s"),
			LongestRun(SelectorPicks), CountRunsOf3OrMore(SelectorPicks), *Distribution(SelectorPicks));
		UE_LOG(LogTemp, Display, TEXT("[패턴 시뮬레이션]   완전 무작위: 최대 연속 %d, 3연속 이상 %d번 | %s"),
			LongestRun(RandomPicks), CountRunsOf3OrMore(RandomPicks), *Distribution(RandomPicks));
	};

	RunRange(TEXT("① 고를 패턴이 2개 이상인 거리"), true);
	RunRange(TEXT("② 전체 거리"), false);
}
