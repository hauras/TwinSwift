#include "Character/PlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "Ability/SAbilitySystemComponent.h"
#include "Ability/SAttributeSet.h"
#include "Camera/CameraComponent.h"
#include "Camera/SCameraConfig.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "SGameplayTags.h"
#include "State/SPlayerState.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

APlayerCharacter::APlayerCharacter()
{
	// 몸은 컨트롤러 방향이 아니라 이동 방향을 바라봄
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.f, 720.f, 0.f);
	Movement->MaxWalkSpeed = RunSpeed;

	// 입력을 놓으면 자연스럽게 멈추게 (값은 BP에서 손맛 보며 조절)
	Movement->BrakingDecelerationWalking = 2000.f;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>("SpringArm");
	SpringArm->SetupAttachment(RootComponent);

	// 카메라가 벽에 막히면 당겨짐
	SpringArm->bDoCollisionTest = true;

	Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>("WeaponMesh");
	// 스켈레톤의 WeaponSocket(hand_R 아래)에 붙임. 손 안 위치·각도·크기는 소켓이 정함
	// → 애니 에디터 미리보기 무기도 같은 소켓에 붙여서, 미리보기와 게임이 똑같이 보이게
	WeaponMesh->SetupAttachment(GetMesh(), TEXT("WeaponSocket"));

	// 타격 판정은 트레이스로 함. 무기 자체는 충돌 없음 (캐릭터 이동을 막지 않게)
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);

	MageMesh = CreateDefaultSubobject<USkeletalMeshComponent>("MageMesh");
	MageMesh->SetupAttachment(GetCapsuleComponent());

	// 충돌은 캡슐이 맡음 (전사 몸과 같게)
	MageMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APlayerCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 둘 다 캡슐에 붙어 있으니 같은 상대 위치를 주면 같은 자리에 섬
	MageMesh->SetRelativeTransform(GetMesh()->GetRelativeTransform());
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (GetNetMode() != NM_DedicatedServer)
	{
		LoadedSwapToMageEffect = SwapToMageEffect.LoadSynchronous();
		LoadedSwapToWarriorEffect = SwapToWarriorEffect.LoadSynchronous();
	}

	// BP에서 RunSpeed를 바꿨을 수 있으니 시작할 때 다시 적용
	ApplyWalkSpeed(false);

	// 처음에는 전사만 보이게 (PossessedBy보다 먼저 불려도 몸은 맞게)
	ApplyActiveSlot();
}

void APlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	InitAbilityActorInfo();
	GiveStartupAbilitiesAndAttributes();
}

void APlayerCharacter::InitAbilityActorInfo()
{
	ASPlayerState* SPlayerState = GetPlayerState<ASPlayerState>();
	if (!SPlayerState)
	{
		return;
	}

	// 주인 = PlayerState(능력·스탯 보관), 몸 = 이 캐릭터(위치·이동)
	// PossessedBy에서 한 번만 불림. 1차와 달리 Loose 태그·위젯을 만들지 않음 (불릴 때마다 쌓였음)
	for (const ETwinSlot Slot : { ETwinSlot::Warrior, ETwinSlot::Mage })
	{
		USAbilitySystemComponent* SASC = SPlayerState->GetSAbilitySystemComponent(Slot);
		SASC->InitAbilityActorInfo(SPlayerState, this);

		// 액터는 하나라 ASC가 아무 메시나 잡음 → 몽타주를 재생할 몸을 캐릭터마다 고정
		SASC->AbilityActorInfo->SkeletalMeshComponent = GetSlotMesh(Slot);

		// 체력 0 감지 (쓰러짐). 이미 연결돼 있으면 다시 연결하지 않음
		FDelegateHandle& Handle = Slot == ETwinSlot::Warrior ? WarriorHealthHandle : MageHealthHandle;
		if (!Handle.IsValid())
		{
			Handle = SASC->GetGameplayAttributeValueChangeDelegate(USAttributeSet::GetHealthAttribute())
				.AddUObject(this, &ThisClass::OnSlotHealthChanged, Slot);
		}
	}

	ApplyActiveSlot();
}

void APlayerCharacter::GiveStartupAbilitiesAndAttributes()
{
	ASPlayerState* SPlayerState = GetPlayerState<ASPlayerState>();
	if (!SPlayerState)
	{
		return;
	}

	GiveSlotStartup(SPlayerState->GetSAbilitySystemComponent(ETwinSlot::Warrior), WarriorConfig);
	GiveSlotStartup(SPlayerState->GetSAbilitySystemComponent(ETwinSlot::Mage), MageConfig);
}

void APlayerCharacter::GiveSlotStartup(USAbilitySystemComponent* SASC, const FTwinSlotConfig& Config)
{
	if (!SASC || SASC->HasGivenStartupAbilities())
	{
		return;
	}

	if (Config.DefaultAttributesEffect)
	{
		FGameplayEffectContextHandle Context = SASC->MakeEffectContext();
		Context.AddSourceObject(this);

		const FGameplayEffectSpecHandle Spec = SASC->MakeOutgoingSpec(Config.DefaultAttributesEffect, 1.f, Context);
		if (Spec.IsValid())
		{
			SASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	SASC->AddStartupAbilities(Config.StartupAbilities);
}

void APlayerCharacter::ApplyCameraConfig(const USCameraConfig* Config)
{
	if (!Config)
	{
		return;
	}

	SpringArm->TargetArmLength = Config->ArmLength;
	SpringArm->SocketOffset = Config->SocketOffset;
	SpringArm->bEnableCameraLag = Config->bEnableCameraLag;
	SpringArm->CameraLagSpeed = Config->CameraLagSpeed;

	// 3인칭: 컨트롤러 회전(마우스)을 따라감
	// 쿼터뷰: 캐릭터가 돌아도 카메라 각도는 월드 기준으로 고정
	SpringArm->bUsePawnControlRotation = Config->bFreeLook;
	SpringArm->SetUsingAbsoluteRotation(!Config->bFreeLook);
	if (!Config->bFreeLook)
	{
		SpringArm->SetWorldRotation(Config->FixedRotation);
	}

	Camera->SetFieldOfView(Config->FieldOfView);
}

void APlayerCharacter::SetWalking(bool bNewWalking)
{
	ApplyWalkSpeed(bNewWalking);
}

void APlayerCharacter::ApplyWalkSpeed(bool bNewWalking)
{
	GetCharacterMovement()->MaxWalkSpeed = bNewWalking ? WalkSpeed : RunSpeed;
}

// ---------- 태그 교체 ----------

void APlayerCharacter::SwapCharacter()
{
	SwapTo(ActiveSlot == ETwinSlot::Warrior ? ETwinSlot::Mage : ETwinSlot::Warrior);
}

void APlayerCharacter::SwapTo(ETwinSlot NewSlot)
{
	if (!CanSwapTo(NewSlot))
	{
		return;
	}

	// 공격 중이면 나가는 캐릭터의 공격을 끊고 교체 (몽타주가 숨긴 몸에서 계속 돌지 않게)
	// 단계 7에서 여기에 교체 어시스트(나가는 캐릭터의 한 방)가 붙음
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->CancelAllAbilities();
	}

	PerformSwap(NewSlot);

	// 타이밍 교체: 직접 교체했을 때만(강제 교체 제외) 들어온 캐릭터가 잠깐 무적 + 완벽 회피 판정
	if (AbilitySystemComponent && PerfectWindowTime > 0.f)
	{
		EndSwapInvincible();
		AbilitySystemComponent->AddLooseGameplayTag(SGameplayTags::State_Invincible);
		SwapInvincibleASC = AbilitySystemComponent;
		GetWorldTimerManager().SetTimer(SwapInvincibleTimer, this, &ThisClass::EndSwapInvincible, PerfectWindowTime, false);

		OpenPerfectWindow(AbilitySystemComponent);
	}
}

void APlayerCharacter::EndSwapInvincible()
{
	GetWorldTimerManager().ClearTimer(SwapInvincibleTimer);
	if (UAbilitySystemComponent* ASC = SwapInvincibleASC.Get())
	{
		ASC->RemoveLooseGameplayTag(SGameplayTags::State_Invincible);
	}
	SwapInvincibleASC.Reset();
}

void APlayerCharacter::OpenPerfectWindow(UAbilitySystemComponent* TargetASC)
{
	if (!TargetASC || PerfectWindowTime <= 0.f)
	{
		return;
	}

	// 이전 판정이 남아 있으면 닫고 새로 (한 번에 하나만)
	ClosePerfectWindow();
	TargetASC->AddLooseGameplayTag(SGameplayTags::State_PerfectWindow);
	PerfectWindowASC = TargetASC;
	GetWorldTimerManager().SetTimer(PerfectWindowTimer, this, &ThisClass::ClosePerfectWindow, PerfectWindowTime, false);
}

void APlayerCharacter::ClosePerfectWindow()
{
	GetWorldTimerManager().ClearTimer(PerfectWindowTimer);
	if (UAbilitySystemComponent* ASC = PerfectWindowASC.Get())
	{
		ASC->RemoveLooseGameplayTag(SGameplayTags::State_PerfectWindow);
	}
	PerfectWindowASC.Reset();
}

void APlayerCharacter::TriggerPerfectEvade()
{
	// 한 번의 판정에서 한 번만
	ClosePerfectWindow();
	if (bPerfectSlow)
	{
		return;
	}

	// 세상만 느리게, 플레이어는 정상 속도 (배율의 역수로 상쇄)
	bPerfectSlow = true;
	UGameplayStatics::SetGlobalTimeDilation(this, PerfectSlowDilation);
	RefreshTimeDilation();

	// 타이머도 느려진 세상 시간으로 흐르므로, 실제 시간 PerfectSlowDuration에 맞춰 줄여서 걸어 둠
	GetWorldTimerManager().SetTimer(PerfectSlowTimer, this, &ThisClass::EndPerfectSlow, PerfectSlowDuration * PerfectSlowDilation, false);

	OnPerfectEvade.Broadcast(true);
}

void APlayerCharacter::EndPerfectSlow()
{
	GetWorldTimerManager().ClearTimer(PerfectSlowTimer);
	if (!bPerfectSlow)
	{
		return;
	}
	bPerfectSlow = false;
	UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
	RefreshTimeDilation();

	OnPerfectEvade.Broadcast(false);
}

float APlayerCharacter::GetNormalTimeDilation() const
{
	return bPerfectSlow ? 1.f / PerfectSlowDilation : 1.f;
}

void APlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	EndPerfectSlow();
	ClosePerfectWindow();
	EndSwapInvincible();

	Super::EndPlay(EndPlayReason);
}

bool APlayerCharacter::CanSwapTo(ETwinSlot NewSlot) const
{
	return NewSlot != ActiveSlot
		&& !bDefeated
		&& !IsSlotDown(NewSlot)
		&& GetSwapCooldownRemaining() <= 0.f;
}

float APlayerCharacter::GetSwapCooldownRemaining() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.f;
	}
	const double Elapsed = World->GetTimeSeconds() - LastSwapTime;
	return FMath::Max(0.f, SwapCooldown - static_cast<float>(Elapsed));
}

bool APlayerCharacter::IsSlotDown(ETwinSlot Slot) const
{
	const ASPlayerState* SPlayerState = GetPlayerState<ASPlayerState>();
	const UAbilitySystemComponent* SlotASC = SPlayerState ? SPlayerState->GetSAbilitySystemComponent(Slot) : nullptr;
	return SlotASC && SlotASC->HasMatchingGameplayTag(SGameplayTags::State_Dead);
}

void APlayerCharacter::PerformSwap(ETwinSlot NewSlot)
{
	ActiveSlot = NewSlot;
	LastSwapTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

	ApplyActiveSlot();
	PlaySwapEffect(NewSlot);

	OnActiveSlotChanged.Broadcast(ActiveSlot);
}

void APlayerCharacter::OnSlotHealthChanged(const FOnAttributeChangeData& Data, ETwinSlot Slot)
{
	if (Data.NewValue <= 0.f && !IsSlotDown(Slot))
	{
		HandleSlotDown(Slot);
	}
}

void APlayerCharacter::HandleSlotDown(ETwinSlot Slot)
{
	ASPlayerState* SPlayerState = GetPlayerState<ASPlayerState>();
	USAbilitySystemComponent* SlotASC = SPlayerState ? SPlayerState->GetSAbilitySystemComponent(Slot) : nullptr;
	if (!SlotASC)
	{
		return;
	}

	// 쓰러짐 표시 (한 번만 붙임 → Loose 태그가 쌓이지 않음). 이동 막기·교체 금지가 이 태그를 봄
	SlotASC->AddLooseGameplayTag(SGameplayTags::State_Dead);
	SlotASC->CancelAllAbilities();

	// 쓰러진 캐릭터에게 대기 회복이 붙어 있었으면 뗌
	if (BenchRegenASC.Get() == SlotASC)
	{
		SlotASC->RemoveActiveGameplayEffect(BenchRegenHandle);
		BenchRegenASC.Reset();
	}

	OnSlotDown.Broadcast(Slot);

	// 대기 캐릭터가 쓰러진 거면 여기서 끝 (지금은 대기 중에 맞을 일이 없지만 막아 둠)
	if (Slot != ActiveSlot)
	{
		return;
	}

	const ETwinSlot Other = GetOtherSlot(Slot);
	if (!IsSlotDown(Other))
	{
		// 나와 있던 캐릭터가 쓰러짐 → 쿨다운 무시하고 다른 캐릭터로 바로 교체
		PerformSwap(Other);
		return;
	}

	// 둘 다 쓰러짐 → 패배. 이동은 State.Dead로, 능력·교체는 IsDefeated로 막힘
	bDefeated = true;
	OnDefeated.Broadcast();
}

USkeletalMeshComponent* APlayerCharacter::GetActiveMesh() const
{
	return GetSlotMesh(ActiveSlot);
}

USkeletalMeshComponent* APlayerCharacter::GetSlotMesh(ETwinSlot Slot) const
{
	return Slot == ETwinSlot::Warrior ? GetMesh() : MageMesh.Get();
}

void APlayerCharacter::ApplyActiveSlot()
{
	const bool bWarrior = ActiveSlot == ETwinSlot::Warrior;

	// 숨긴 몸은 애니 계산도 멈춤. 칼은 전사 몸의 자식이라 같이 숨음 (두 번째 인자 true)
	GetMesh()->SetVisibility(bWarrior, true);
	GetMesh()->SetComponentTickEnabled(bWarrior);
	MageMesh->SetVisibility(!bWarrior, true);
	MageMesh->SetComponentTickEnabled(!bWarrior);

	// 지금 캐릭터의 ASC·능력치로 바꿈 → 적의 데미지, 이동 막기 태그 검사가 이쪽으로 감
	if (const ASPlayerState* SPlayerState = GetPlayerState<ASPlayerState>())
	{
		AbilitySystemComponent = SPlayerState->GetSAbilitySystemComponent(ActiveSlot);
		AttributeSet = SPlayerState->GetAttributeSet(ActiveSlot);
	}

	UpdateBenchRegen();
}

void APlayerCharacter::UpdateBenchRegen()
{
	ASPlayerState* SPlayerState = GetPlayerState<ASPlayerState>();
	if (!BenchRegenEffect || !SPlayerState)
	{
		return;
	}

	// 들어온 캐릭터에게 붙어 있던 회복을 뗌
	if (UAbilitySystemComponent* OldASC = BenchRegenASC.Get())
	{
		OldASC->RemoveActiveGameplayEffect(BenchRegenHandle);
	}
	BenchRegenASC.Reset();

	// 나간(대기) 캐릭터에게 붙임. 쓰러진 캐릭터는 회복하지 않음
	const ETwinSlot BenchSlot = GetOtherSlot(ActiveSlot);
	if (IsSlotDown(BenchSlot))
	{
		return;
	}
	USAbilitySystemComponent* BenchASC = SPlayerState->GetSAbilitySystemComponent(BenchSlot);
	if (!BenchASC)
	{
		return;
	}

	FGameplayEffectContextHandle Context = BenchASC->MakeEffectContext();
	Context.AddSourceObject(this);
	const FGameplayEffectSpecHandle Spec = BenchASC->MakeOutgoingSpec(BenchRegenEffect, 1.f, Context);
	if (Spec.IsValid())
	{
		BenchRegenHandle = BenchASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		BenchRegenASC = BenchASC;
	}
}

void APlayerCharacter::PlaySwapEffect(ETwinSlot NewSlot)
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	UNiagaraSystem* Effect = NewSlot == ETwinSlot::Mage
		? LoadedSwapToMageEffect.Get()
		: LoadedSwapToWarriorEffect.Get();
	if (!Effect)
	{
		return;
	}

	// 캐릭터의 발 위치를 기준으로, 바닥 위에 살짝 띄워 겹침 깜빡임을 피한다.
	const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	FVector Location = Feet + FVector(0.f, 0.f, 4.f);
	FHitResult GroundHit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(TagSwapGround), false, this);
	if (World->LineTraceSingleByChannel(GroundHit, Feet + FVector(0.f, 0.f, 30.f),
		Feet - FVector(0.f, 0.f, 80.f), ECC_Visibility, Query))
	{
		Location = GroundHit.ImpactPoint + FVector(0.f, 0.f, 4.f);
	}

	// 월드에 남기는 단발 효과: 이후 이동해도 원은 교체 위치에 유지된다.
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World,
		Effect,
		Location,
		FRotator::ZeroRotator,
		FVector::OneVector,
		true,
		true,
		ENCPoolMethod::None,
		false);
}
