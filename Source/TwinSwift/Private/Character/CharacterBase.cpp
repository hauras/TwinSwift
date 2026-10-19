#include "Character/CharacterBase.h"

#include "Ability/SAttributeSet.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace
{
	// 히트스톱 중 시간 배율 (0이면 애니메이션 상태가 이상해질 수 있어 거의 0)
	constexpr float HitStopDilation = 0.05f;
}

ACharacterBase::ACharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// 캐릭터(플레이어·적)는 카메라를 막지 않음
	// 막으면 적과 겹치거나 적이 카메라와 캐릭터 사이에 들어올 때 스프링암이 벽으로 보고 카메라를 확 당김
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
}

UAbilitySystemComponent* ACharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ACharacterBase::ApplyHitStop(float Duration)
{
	UWorld* World = GetWorld();
	if (!World || Duration <= 0.f)
	{
		return;
	}

	bInHitStop = true;
	RefreshTimeDilation();

	// 타이머는 세상 시간으로 흐름 → 세상이 느린 중(완벽 회피 슬로)이어도 실제 시간 Duration에 끝나게 맞춤
	const float WorldDuration = Duration * UGameplayStatics::GetGlobalTimeDilation(this);
	World->GetTimerManager().SetTimer(HitStopTimer, this, &ThisClass::EndHitStop, WorldDuration, false);
}

void ACharacterBase::EndHitStop()
{
	bInHitStop = false;
	RefreshTimeDilation();
}

void ACharacterBase::RefreshTimeDilation()
{
	CustomTimeDilation = bInHitStop ? HitStopDilation : GetNormalTimeDilation();
}
