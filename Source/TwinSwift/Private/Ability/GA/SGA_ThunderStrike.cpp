#include "Ability/GA/SGA_ThunderStrike.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "SGameplayTags.h"
#include "Engine/World.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

USGA_ThunderStrike::USGA_ThunderStrike()
{
	InputTag = SGameplayTags::InputTag_Mage_Skill2;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(SGameplayTags::Ability_Mage_Thunder);
	SetAssetTags(AssetTags);

	CooldownDuration = 10.f;
	CooldownTags.AddTag(SGameplayTags::Cooldown_Mage_Thunder);

	// 원 안의 적 모두에게 2.0배
	DamageMultiplier = 2.f;
}

void USGA_ThunderStrike::Strike()
{
	// 타이머와 바로 부르기가 겹쳐도 한 번만
	if (bStruck)
	{
		return;
	}
	bStruck = true;

	UWorld* World = GetWorld();
	if (World && StrikeEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, StrikeEffect, StrikeLocation);
	}

	// 원 안의 적 모두에게 배율 데미지 (같은 편은 알아서 거름)
	TArray<AActor*> StruckActors;
	ApplyDamageInRadius(StrikeLocation, Radius, DamageMultiplier, &StruckActors);

	// 맞은 적 중 [균열] 적부터 연쇄 폭발
	StartCrackChain(StruckActors);

	// 시전 후 대기도 끝났고 연쇄도 없으면 능력 종료 (연쇄가 있으면 끝날 때 다시 시도)
	TryFinishCast();
}

void USGA_ThunderStrike::StartCrackChain(const TArray<AActor*>& StruckActors)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	World->GetTimerManager().ClearTimer(ChainTimer);
	ChainWave.Reset();
	ChainVisited.Reset();
	ChainDepth = 1;

	// 1단계: 번개에 직접 맞은 [균열] 적
	for (AActor* Actor : StruckActors)
	{
		const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
		if (ASC && ASC->HasMatchingGameplayTag(SGameplayTags::Mark_Crack))
		{
			ChainWave.Add(Actor);
			ChainVisited.Add(Actor);
		}
	}

	// 번개가 먼저 보이고 한 박자 뒤에 터짐
	if (ChainWave.Num() > 0)
	{
		World->GetTimerManager().SetTimer(
			ChainTimer,
			this,
			&ThisClass::ExplodeChainWave,
			FMath::Max(ChainStepDelay, KINDA_SMALL_NUMBER),
			false);
	}
}

void USGA_ThunderStrike::ExplodeChainWave()
{
	// BFS 한 단계: 이번 단계의 적을 모두 터뜨리고, 그 폭발 범위에서 다음 단계를 모음
	// 한 단계 안에서 한 적은 한 번만 맞음 (폭발 범위가 겹쳐도 피해가 쌓이지 않게)
	TSet<AActor*> DamagedThisWave;
	TArray<TWeakObjectPtr<AActor>> NextWave;
	int32 ExplodedCount = 0;
	for (const TWeakObjectPtr<AActor>& Source : ChainWave)
	{
		if (ExplodeOne(Source.Get(), DamagedThisWave, NextWave))
		{
			++ExplodedCount;
		}
	}

	// 단계별 확인용 (출력 로그). 나중에 적이 많을 때 찾기 비용을 잴 때도 씀
	UE_LOG(LogTemp, Log, TEXT("[균열 연쇄] %d단계: %d개 폭발, 피해 %d명, 다음 단계 %d개"),
		ChainDepth,
		ExplodedCount,
		DamagedThisWave.Num(),
		NextWave.Num());

	ChainWave = MoveTemp(NextWave);
	++ChainDepth;

	UWorld* World = GetWorld();
	if (World && ChainWave.Num() > 0)
	{
		World->GetTimerManager().SetTimer(
			ChainTimer,
			this,
			&ThisClass::ExplodeChainWave,
			FMath::Max(ChainStepDelay, KINDA_SMALL_NUMBER),
			false);
		return;
	}

	// 연쇄 끝: 능력이 아직 켜져 있으면 종료 시도
	TryFinishCast();
}

bool USGA_ThunderStrike::ExplodeOne(
	AActor* Source,
	TSet<AActor*>& DamagedThisWave,
	TArray<TWeakObjectPtr<AActor>>& OutNextWave)
{
	// 기다리는 동안 사라졌거나 [균열] 시간이 끝났으면 터지지 않음
	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Source);
	if (!SourceASC || !SourceASC->HasMatchingGameplayTag(SGameplayTags::Mark_Crack))
	{
		return false;
	}

	// 표식을 먼저 지움 (터뜨린 표식은 사라짐)
	SourceASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(SGameplayTags::Mark_Crack));

	const FVector Center = Source->GetActorLocation();
	UWorld* World = GetWorld();
	if (World && BurstEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, BurstEffect, Center);
	}
	else if (World)
	{
		DrawDebugCircle(World, Center, BurstRadius, 32, FColor::Orange,
			false, 0.3f, 0, 4.f, FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
	}

	TArray<AActor*> Enemies;
	FindEnemiesInRadius(Center, BurstRadius, Enemies);
	const bool bCanGoDeeper = ChainDepth < MaxChainDepth;
	for (AActor* Enemy : Enemies)
	{
		if (!DamagedThisWave.Contains(Enemy))
		{
			DamagedThisWave.Add(Enemy);
			ApplyDamageTo(Enemy, BurstMultiplier, nullptr, SGameplayTags::Damage_CrackBurst);
		}

		// 다음 단계: [균열]이 있고 아직 줄에 들어간 적 없는 적 (방문 표시)
		if (!bCanGoDeeper || ChainVisited.Contains(Enemy))
		{
			continue;
		}
		const UAbilitySystemComponent* EnemyASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Enemy);
		if (EnemyASC && EnemyASC->HasMatchingGameplayTag(SGameplayTags::Mark_Crack))
		{
			ChainVisited.Add(Enemy);
			OutNextWave.Add(Enemy);
		}
	}
	return true;
}

void USGA_ThunderStrike::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 시전 도중 회피 등으로 끊겨도 번개 타이머가 남지 않게 (안 지우면 능력이 끝난 뒤에 번개가 떨어짐)
	// 연쇄 타이머(ChainTimer)는 지우지 않음: 번개가 이미 떨어졌으면 연쇄는 끝까지
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StrikeTimer);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USGA_ThunderStrike::OnCast(const FVector& Origin, const FVector& Direction)
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();
	if (!Avatar || !World)
	{
		return;
	}

	// 커서 위치(바닥)를 캐릭터 기준 MaxRange로 자름. 높이는 커서 위치 그대로 (바닥)
	const FVector Aim = GetAimPoint();
	const FVector Start = Avatar->GetActorLocation();
	FVector ToAim = Aim - Start;
	ToAim.Z = 0.f;
	ToAim = ToAim.GetClampedToMaxSize(MaxRange);
	StrikeLocation = FVector(Start.X + ToAim.X, Start.Y + ToAim.Y, Aim.Z);

	bStruck = false;

	// 경고 원: 이펙트가 있으면 이펙트, 없으면 바닥에 디버그 원 (StrikeDelay 동안 보임)
	if (WarningEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, WarningEffect, StrikeLocation);
	}
	else
	{
		DrawDebugCircle(World, StrikeLocation + FVector(0.f, 0.f, 3.f), Radius, 48, FColor::Cyan,
			false, StrikeDelay, 0, 4.f, FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
	}

	// StrikeDelay 뒤 번개. 0이면 바로
	if (StrikeDelay > 0.f)
	{
		World->GetTimerManager().SetTimer(StrikeTimer, this, &ThisClass::Strike, StrikeDelay, false);
	}
	else
	{
		Strike();
	}
}

bool USGA_ThunderStrike::IsCastFinished() const
{
	// 번개가 떨어지고 연쇄도 끝나야 종료 (캐릭터는 RecoveryTime 뒤 움직일 수 있음)
	return bStruck && ChainWave.Num() == 0;
}
