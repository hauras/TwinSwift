#include "Character/SMirrorBoss.h"

#include "AbilitySystemComponent.h"
#include "Ability/SAbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Character/PlayerCharacter.h"
#include "SGameplayTags.h"
#include "State/SPlayerState.h"
#include "Mirror/SActionReplayComponent.h"
#include "Mirror/SMirrorDefenseComponent.h"

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

	ReplayComponent = CreateDefaultSubobject<USActionReplayComponent>("ReplayComponent");
	DefenseComponent = CreateDefaultSubobject<USMirrorDefenseComponent>("DefenseComponent");

	// 방어 표시 갱신에 틱이 필요 (부모 ACharacterBase가 꺼 둠)
	PrimaryActorTick.bCanEverTick = true;

	// 플레이어와 같은 회전 속도 (보스 기본 180은 느려서 따라 하는 움직임보다 몸이 늦게 돎)
	GetCharacterMovement()->RotationRate = FRotator(0.f, 720.f, 0.f);
}

bool ASMirrorBoss::IsDefending() const
{
	return DefenseComponent && DefenseComponent->IsDefending();
}

void ASMirrorBoss::SwapTo(ETwinSlot NewSlot)
{
	// 이미 그 몸이면 아무것도 안 함 (진행 중 능력을 끊지 않게)
	// 처음 BeginPlay에서는 두 몸이 다 보이는 상태라 여기를 지나 몸을 맞춤 (ActiveSlot만 보면 이미 Warrior라 건너뛰게 됨)
	const bool bWarriorVisible = GetMesh()->IsVisible();
	if (ActiveSlot == NewSlot && bWarriorVisible == (NewSlot == ETwinSlot::Warrior)
		&& MageMesh->IsVisible() == (NewSlot == ETwinSlot::Mage)) return;

	// 공격 중이면 끊고 바꿈 (플레이어 교체와 같음). 숨긴 몸은 애니 틱이 꺼져 몽타주가 끝나지 않음 → 능력·State.Attacking이 남음
	// 취소 불가 능력(시전 확정된 마법)은 CancelAllAbilities가 건너뜀
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->CancelAllAbilities();
	}

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

bool ASMirrorBoss::StartAI()
{
	ReplayComponent->StartReplay();
	return true;
}

void ASMirrorBoss::StopAI()
{
	// 부모: 길 찾기 이동 정지 (행동 트리는 없음)
	Super::StopAI();
	ReplayComponent->StopReplay();
	DefenseComponent->CancelPendingEvade();
}

void ASMirrorBoss::ResumeAI()
{
	if (!IsInCombat() || IsDead()) return;

	ReplayComponent->StartReplay();
}

void ASMirrorBoss::BeginPlay()
{
	// 부모(EnemyCharacter)가 여기서 AbilityActorInfo를 만듦 → 그 뒤에 몸을 맞춰야 몽타주 메시도 바뀜
	Super::BeginPlay();

	SwapTo(ETwinSlot::Warrior);
}

void ASMirrorBoss::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 지금 플레이어 몸으로 때리면 막히는지 (화면만 보고 "지금 때려도 되나"를 알게)
	const APlayerCharacter* Player = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	const bool bGuard = Player && IsGuardingAgainst(Player->GetActiveSlot());
	if (bGuard == bGuardShown) return;

	// 두 몸·칼 모두에 (숨긴 몸은 안 그려짐 → 막는 중에 교체해도 다시 켤 필요 없음)
	bGuardShown = bGuard;
	UMaterialInterface* Overlay = bGuard ? GuardOverlayMaterial.Get() : nullptr;
	GetMesh()->SetOverlayMaterial(Overlay);
	MageMesh->SetOverlayMaterial(Overlay);
	WeaponMesh->SetOverlayMaterial(Overlay);
}

bool ASMirrorBoss::IsGuardingAgainst(ETwinSlot AttackerSlot) const
{
	if (AttackerSlot != ActiveSlot || IsDead() || !AbilitySystemComponent) return false;

	// 경직·큰 경직은 빈틈 (깨면 보상)
	FGameplayTagContainer OpenTags;
	OpenTags.AddTag(SGameplayTags::State_Stun);
	OpenTags.AddTag(SGameplayTags::State_Groggy);
	if (AbilitySystemComponent->HasAnyMatchingGameplayTags(OpenTags)) return false;

	// 능력이 켜져 있으면 빈틈 (공격 준비·휘두름·회복, 마법 시전, 회피). 마법은 State.Attacking을 안 붙여서 능력으로 봄
	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (Spec.IsActive()) return false;
	}
	return true;
}

float ASMirrorBoss::GetIncomingDamageScale(const UAbilitySystemComponent* SourceASC) const
{
	// 때린 ASC가 플레이어의 어느 몸 것인지 (교체 어시스트 분신은 나간 캐릭터의 ASC로 때림 → 그 몸으로 친 것)
	const ASPlayerState* AttackerState = SourceASC ? Cast<ASPlayerState>(SourceASC->GetOwnerActor()) : nullptr;
	if (!AttackerState) return 1.f;

	for (const ETwinSlot Slot : { ETwinSlot::Warrior, ETwinSlot::Mage })
	{
		const UAbilitySystemComponent* SlotASC = AttackerState->GetSAbilitySystemComponent(Slot);
		if (SlotASC == SourceASC)
		{
			return IsGuardingAgainst(Slot) ? SameBodyDamageScale : 1.f;
		}
	}
	return 1.f;
}
