#include "Ability/GA/SGA_ThunderStrike.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Actor/SBossTelegraph.h"
#include "Pool/SActorPool.h"
#include "UObject/ConstructorHelpers.h"
#include "SGameplayTags.h"
#include "Ability/SAbilityFunctionLibrary.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
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

	static const ConstructorHelpers::FClassFinder<ASBossTelegraph> Telegraph(
		TEXT("/Game/Blueprints/Boss/BP_BossTelegraph"));
	WarningTelegraphClass = Telegraph.Class;
}

void USGA_ThunderStrike::Strike()
{
	// 타이머와 바로 부르기가 겹쳐도 한 번만
	if (bStruck)
	{
		return;
	}
	bStruck = true;

	if (ASBossTelegraph* Telegraph = WarningTelegraph.Get())
	{
		WarningTelegraph.Reset();
		Telegraph->Release();
	}

	UWorld* World = GetWorld();
	if (World && StrikeEffect)
	{
		USAbilityFunctionLibrary::SpawnOneShotEffect(World, StrikeEffect, StrikeLocation);
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
	for (const TWeakObjectPtr<AActor>& Source : ChainWave)
	{
		ExplodeOne(Source.Get(), DamagedThisWave, NextWave);
	}

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

void USGA_ThunderStrike::ExplodeOne(
	AActor* Source,
	TSet<AActor*>& DamagedThisWave,
	TArray<TWeakObjectPtr<AActor>>& OutNextWave)
{
	// 기다리는 동안 사라졌거나 죽었거나 [균열] 시간이 끝났으면 터지지 않음 (이전 단계에서 저장한 대상 → 터뜨리는 지금 다시 확인)
	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Source);
	if (!USAbilityFunctionLibrary::IsTargetable(GetAvatarActorFromActorInfo(), Source)
		|| !SourceASC
		|| !SourceASC->HasMatchingGameplayTag(SGameplayTags::Mark_Crack))
	{
		return;
	}

	// 표식을 먼저 지움 (터뜨린 표식은 사라짐)
	SourceASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(SGameplayTags::Mark_Crack));

	const FVector Center = Source->GetActorLocation();
	UWorld* World = GetWorld();
	if (World && BurstEffect)
	{
		USAbilityFunctionLibrary::SpawnOneShotEffect(World, BurstEffect, Center);
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

	if (ASBossTelegraph* Telegraph = WarningTelegraph.Get())
	{
		WarningTelegraph.Reset();
		Telegraph->Release();
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

	// 사거리를 자르면 조준점과 낙뢰 지점의 바닥 높이가 다를 수 있다.
	const FVector Start = Avatar->GetActorLocation();
	const FVector Offset = (GetAimPoint() - Start).GetClampedToMaxSize2D(MaxRange);
	const APawn* CastingPawn = Cast<APawn>(Avatar);
	const double GroundHeight = CastingPawn ? CastingPawn->GetNavAgentLocation().Z : Start.Z;
	const FVector Candidate(Start.X + Offset.X, Start.Y + Offset.Y, GroundHeight);

	FHitResult FloorHit;
	const FCollisionQueryParams Query(SCENE_QUERY_STAT(ThunderStrikeFloor), false, Avatar);
	if (!World->LineTraceSingleByObjectType(
		FloorHit,
		Candidate + FVector(0.f, 0.f, 300.f),
		Candidate - FVector(0.f, 0.f, 600.f),
		FCollisionObjectQueryParams(ECC_WorldStatic),
		Query) || FloorHit.ImpactNormal.Z < 0.7f)
	{
		// 바닥이 없으면 효과 없이 시전 회복만 마친다.
		bStruck = true;
		return;
	}

	StrikeLocation = FloorHit.ImpactPoint;
	bStruck = false;

	// 시전 예고는 타격 위치와 반경을 그대로 보여 준다.
	if (WarningEffect)
	{
		USAbilityFunctionLibrary::SpawnOneShotEffect(World, WarningEffect, StrikeLocation);
	}

	else if (WarningTelegraphClass && StrikeDelay > 0.f)
	{
		if (USActorPool* Pool = World->GetSubsystem<USActorPool>())
		{
			ASBossTelegraph* Telegraph = Pool->Acquire<ASBossTelegraph>(WarningTelegraphClass, FTransform(StrikeLocation));
			if (Telegraph)
			{
				Telegraph->Setup(StrikeLocation, FRotator::ZeroRotator, Radius, 360.f, FLinearColor(0.12f, 0.72f, 1.f));
				WarningTelegraph = Telegraph;
			}
		}
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
