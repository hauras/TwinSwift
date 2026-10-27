#include "Character/SMirrorBoss.h"

#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

ASMirrorBoss::ASMirrorBoss()
{
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>("WeaponMesh");
	WeaponMesh->SetupAttachment(GetMesh(), TEXT("WeaponSocket"));

	// 타격 판정은 트레이스로 함. 칼 자체는 충돌 없음 (플레이어와 같음)
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);

	MageMesh = CreateDefaultSubobject<USkeletalMeshComponent>("MageMesh");
	MageMesh->SetupAttachment(GetCapsuleComponent());

	// 충돌은 캡슐이 맡음 (전사 몸과 같게)
	MageMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASMirrorBoss::SwapTo(ETwinSlot NewSlot)
{
	// 같은 몸이어도 끝까지 함 (BeginPlay에서 처음 상태를 맞출 때 ActiveSlot이 이미 Warrior)
	ActiveSlot = NewSlot;
	const bool bWarrior = ActiveSlot == ETwinSlot::Warrior;

	// 숨긴 몸은 애니 계산도 멈춤. 칼은 전사 몸의 자식이라 같이 숨음 (두 번째 인자 true)
	GetMesh()->SetVisibility(bWarrior, true);
	GetMesh()->SetComponentTickEnabled(bWarrior);
	MageMesh->SetVisibility(!bWarrior, true);
	MageMesh->SetComponentTickEnabled(!bWarrior);

	// ASC는 하나 그대로, 몽타주를 틀 메시만 보이는 몸으로 (안 바꾸면 숨긴 몸에서 공격 모션이 나옴)
	if (AbilitySystemComponent && AbilitySystemComponent->AbilityActorInfo.IsValid())
	{
		AbilitySystemComponent->AbilityActorInfo->SkeletalMeshComponent = bWarrior ? GetMesh() : MageMesh.Get();
	}
}

bool ASMirrorBoss::GetAbilityAimPoint(FVector& OutAimPoint) const
{
	// 움직임은 기록을 따라 하지만 겨냥은 지금의 플레이어 (매번 새로 찾음)
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player) return false;

	OutAimPoint = Player->GetActorLocation();
	return true;
}

bool ASMirrorBoss::GetAbilityMoveDirection(FVector& OutDirection) const
{
	// 재생 컴포넌트가 아직 방향을 안 넣었으면 능력의 기본 방향으로
	if (ReplayMoveDirection.IsNearlyZero()) return false;

	OutDirection = ReplayMoveDirection;
	return true;
}

void ASMirrorBoss::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 둘 다 캡슐에 붙어 있으니 같은 상대 위치를 주면 같은 자리에 섬
	MageMesh->SetRelativeTransform(GetMesh()->GetRelativeTransform());
}

void ASMirrorBoss::BeginPlay()
{
	// 부모(EnemyCharacter)가 여기서 AbilityActorInfo를 만듦 → 그 뒤에 몸을 맞춰야 몽타주 메시도 바뀜
	Super::BeginPlay();

	SwapTo(ETwinSlot::Warrior);
}
