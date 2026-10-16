#include "Ability/GA/SGA_ThunderStrike.h"

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
	ApplyDamageInRadius(StrikeLocation, Radius, DamageMultiplier);

	// [균열] 터뜨리기는 단계 8에서 여기에 추가

	// 시전 후 대기도 끝났으면 능력 종료
	TryFinishCast();
}

void USGA_ThunderStrike::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 시전 도중 회피 등으로 끊겨도 번개 타이머가 남지 않게 (안 지우면 능력이 끝난 뒤에 번개가 떨어짐)
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
	// 번개가 떨어지기 전에는 능력을 끝내지 않음 (캐릭터는 RecoveryTime 뒤 움직일 수 있음)
	return bStruck;
}
