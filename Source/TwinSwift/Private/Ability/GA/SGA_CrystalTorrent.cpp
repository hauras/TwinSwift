#include "Ability/GA/SGA_CrystalTorrent.h"

#include "SGameplayTags.h"
#include "Engine/World.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

namespace
{
	// 띠를 검사하는 간격(초)
	constexpr float TickInterval = 0.05f;

	// 띠 높이의 절반(cm). 캐릭터 캡슐 정도
	constexpr float BandHalfHeight = 90.f;
}

USGA_CrystalTorrent::USGA_CrystalTorrent()
{
	InputTag = SGameplayTags::InputTag_Mage_Skill1;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(SGameplayTags::Ability_Mage_FrostBolt);
	SetAssetTags(AssetTags);

	CooldownDuration = 6.f;
	CooldownTags.AddTag(SGameplayTags::Cooldown_Mage_FrostBolt);

	// 한 번에 0.5배 × 3번 = 1.5배. 첫 타에 [빙결]
	DamageMultiplier = 0.5f;
	MarkTag = SGameplayTags::Mark_Frost;

	// 얼음 앞쪽 끝 위치 (NS_Crystal_Torrent를 에디터에서 시간별로 재고 영상으로 보정한 값). 처음엔 느리고 점점 빨라짐
	FRichCurve* Front = FrontDistanceCurve.GetRichCurve();
	Front->AddKey(0.0f, 0.f);
	Front->AddKey(0.1f, 100.f);
	Front->AddKey(0.2f, 270.f);
	Front->AddKey(0.3f, 370.f);
	Front->AddKey(0.4f, 580.f);
	Front->AddKey(0.5f, 780.f);
	Front->AddKey(0.7f, 1180.f);
	Front->AddKey(1.0f, 1780.f);
}

void USGA_CrystalTorrent::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 취소돼도 판정 타이머가 남지 않게
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TickTimer);
	}
	HitRecords.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USGA_CrystalTorrent::OnCast(const FVector& Origin, const FVector& Direction)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TorrentOrigin = Origin;
	TorrentDirection = Direction;
	StartTime = World->GetTimeSeconds();

	// 지난 시전의 기록 지우기 (능력 인스턴스는 재사용됨)
	HitRecords.Reset();

	if (TorrentEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetAvatarActorFromActorInfo(), TorrentEffect, Origin, Direction.Rotation());
	}

	World->GetTimerManager().SetTimer(TickTimer, this, &ThisClass::TickTorrent, TickInterval, true);
	TickTorrent();
}

bool USGA_CrystalTorrent::IsCastFinished() const
{
	const UWorld* World = GetWorld();
	return !World || !World->GetTimerManager().IsTimerActive(TickTimer);
}

void USGA_CrystalTorrent::TickTorrent()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const double Now = World->GetTimeSeconds();
	const float Elapsed = static_cast<float>(Now - StartTime);

	// 판정 시간이 끝남 → 타이머를 멈추고 능력 종료 시도 (시전 후 대기도 끝났으면 종료)
	if (Elapsed > Duration)
	{
		World->GetTimerManager().ClearTimer(TickTimer);
		TryFinishCast();
		return;
	}

	// 얼음 앞쪽 끝까지의 거리. 띠 = 손 ~ 앞쪽 끝
	const float Front = FMath::Min(FrontDistanceCurve.GetRichCurveConst()->Eval(Elapsed), MaxRange);
	if (Front <= 0.f)
	{
		return;
	}

	// 칼날 판정·돌진 베기와 같은 기준: Pawn만. 캡슐을 손에서 앞쪽 끝까지 쓸어서 띠 안을 찾음
	TArray<FHitResult> Hits;
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(CrystalTorrent), false, GetAvatarActorFromActorInfo());
	World->SweepMultiByObjectType(
		Hits,
		TorrentOrigin,
		TorrentOrigin + TorrentDirection * Front,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeCapsule(HalfWidth, BandHalfHeight),
		Params);

	// 한 액터의 컴포넌트가 여럿 걸려도 이번 검사에서는 한 번만
	TSet<AActor*> Visited;
	for (const FHitResult& Hit : Hits)
	{
		AActor* Target = Hit.GetActor();
		if (!Target || Visited.Contains(Target))
		{
			continue;
		}
		Visited.Add(Target);

		// 없으면 새 기록, 있으면 기존 기록 (TMap: 키로 바로 찾음)
		FTorrentHitRecord& Record = HitRecords.FindOrAdd(Target);
		if (Record.Count >= HitsPerTarget)
		{
			continue;
		}
		if (Record.Count > 0 && Now - Record.LastHitTime < HitInterval)
		{
			continue;
		}

		// 같은 편이면 ApplyDamageTo가 false → 기록은 남아도 맞지 않음
		if (!ApplyDamageTo(Target, DamageMultiplier))
		{
			continue;
		}

		if (Record.Count == 0)
		{
			ApplyMarkTo(Target);
		}
		++Record.Count;
		Record.LastHitTime = Now;
	}
}
