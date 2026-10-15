#include "Character/EnemyCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "DrawDebugHelpers.h"
#include "SGameplayTags.h"
#include "TimerManager.h"
#include "Ability/SAbilityFunctionLibrary.h"
#include "Ability/SAbilitySystemComponent.h"
#include "Ability/SAttributeSet.h"
#include "Ability/SGE_Damage.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"

AEnemyCharacter::AEnemyCharacter()
{
	AbilitySystemComponent = CreateDefaultSubobject<USAbilitySystemComponent>("AbilitySystemComponent");

	AttributeSet = CreateDefaultSubobject<USAttributeSet>("AttributeSet");
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	InitAbilityActorInfo();

	if (bDebugAutoAttack)
	{
		GetWorldTimerManager().SetTimer(DebugAttackTimer, this, &ThisClass::StartDebugAttack, DebugAttackInterval, true, DebugAttackInterval);
	}
}

void AEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(DebugAttackTimer);
	GetWorldTimerManager().ClearTimer(DebugAttackWarningTimer);
	GetWorldTimerManager().ClearTimer(DebugAttackHitTimer);

	Super::EndPlay(EndPlayReason);
}

void AEnemyCharacter::InitAbilityActorInfo()
{
	// 주인과 몸이 둘 다 자기 자신
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
}

void AEnemyCharacter::StartDebugAttack()
{
	DebugAttackStartTime = GetWorld()->GetTimeSeconds();

	DrawDebugAttackWarning();
	GetWorldTimerManager().SetTimer(DebugAttackWarningTimer, this, &ThisClass::DrawDebugAttackWarning, 0.05f, true);
	GetWorldTimerManager().SetTimer(DebugAttackHitTimer, this, &ThisClass::ExecuteDebugAttack, DebugAttackWindup, false);
}

void AEnemyCharacter::DrawDebugAttackWarning()
{
	const float Elapsed = static_cast<float>(GetWorld()->GetTimeSeconds() - DebugAttackStartTime);
	const float Alpha = FMath::Clamp(Elapsed / DebugAttackWindup, 0.f, 1.f);

	// 발밑 바닥 높이에 그림
	const FVector Center = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 3.f);

	// 바깥 원 = 공격 범위, 안쪽 원 = 남은 시간 (바깥 원에 닿으면 맞음)
	DrawDebugCircle(GetWorld(), Center, DebugAttackRadius, 48, FColor::Red, false, 0.06f, 0, 4.f, FVector(1, 0, 0), FVector(0, 1, 0), false);
	DrawDebugCircle(GetWorld(), Center, DebugAttackRadius * Alpha, 48, FColor::Orange, false, 0.06f, 0, 3.f, FVector(1, 0, 0), FVector(0, 1, 0), false);
}

void AEnemyCharacter::ExecuteDebugAttack()
{
	GetWorldTimerManager().ClearTimer(DebugAttackWarningTimer);

	// 맞은 순간 표시 (꽉 찬 원 여러 겹, 0.2초)
	const FVector Center = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 3.f);
	for (float Ring = DebugAttackRadius; Ring > 0.f; Ring -= 40.f)
	{
		DrawDebugCircle(GetWorld(), Center, Ring, 48, FColor::Red, false, 0.2f, 0, 6.f, FVector(1, 0, 0), FVector(0, 1, 0), false);
	}

	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player || !AbilitySystemComponent || USAbilityFunctionLibrary::IsFriend(this, Player))
	{
		return;
	}
	if (FVector::Dist2D(Player->GetActorLocation(), GetActorLocation()) > DebugAttackRadius)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Player);
	if (!TargetASC)
	{
		return;
	}

	// 플레이어 공격과 같은 데미지 GE (적 공격력 × 배율) → AttributeSet에서 무적·완벽 회피 판정을 거침
	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);
	FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(USGE_Damage::StaticClass(), 1.f, Context);
	if (Spec.IsValid())
	{
		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(Spec, SGameplayTags::SetByCaller_DamageMultiplier, DebugAttackMultiplier);
		AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}
}
