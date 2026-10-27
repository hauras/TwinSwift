
#include "Character/BossCharacter.h"

#include "SGameplayTags.h"
#include "Abilities/GameplayAbility.h"
#include "Ability/SAbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AI/SAIController.h"
#include "AI/SBossPatternComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Ability/SAttributeSet.h"
#include "Components/CapsuleComponent.h"
#include "Ability/SAbilityFunctionLibrary.h"
#include "Animation/AnimInstance.h"
#include "Camera/SCameraShake_BossImpact.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

ABossCharacter::ABossCharacter()
{
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	
	GetCharacterMovement()->MaxWalkSpeed = 300.f;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 180.f, 0.f);

	PatternComponent = CreateDefaultSubobject<USBossPatternComponent>("PatternComponent");

	AIControllerClass = ASAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	ArmorBreakShake = USCameraShake_BossImpact::StaticClass();
}

void ABossCharacter::BeginPlay()
{
	Super::BeginPlay();

	GiveStartupAbilitiesAndAttributes();

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		GroggyTagHandle = ASC->RegisterGameplayTagEvent(SGameplayTags::State_Groggy, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &ThisClass::OnGroggyTagChanged);

		// 체력 0 감지 (사망)
		HealthChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(USAttributeSet::GetHealthAttribute())
			.AddUObject(this, &ThisClass::OnHealthChanged);

		// 대지의 분노가 끝나는 순간 (강타가 나갔거나 끊김)
		AbilityEndedHandle = ASC->OnAbilityEnded.AddUObject(this, &ThisClass::OnAbilityEnded);
	}
}

void ABossCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->RegisterGameplayTagEvent(SGameplayTags::State_Groggy, EGameplayTagEventType::NewOrRemoved).Remove(GroggyTagHandle);
		ASC->GetGameplayAttributeValueChangeDelegate(USAttributeSet::GetHealthAttribute()).Remove(HealthChangedHandle);
		ASC->OnAbilityEnded.Remove(AbilityEndedHandle);
	}

	GetWorldTimerManager().ClearTimer(PhaseTwoTimer);
	GetWorldTimerManager().ClearTimer(GroggyPoseTimer);
	GetWorldTimerManager().ClearTimer(GroggyEndTimer);

	Super::EndPlay(EndPlayReason);
}

void ABossCharacter::OnGroggyTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// 붙을 때만 (떨어질 때는 할 일 없음)
	if (NewCount <= 0)
	{
		return;
	}

	// 부모 태그 하나로 보스 공격 전부 → 보스 2·3도 코드 변경 없음
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		const FGameplayTagContainer BossAttackTags(SGameplayTags::Ability_Boss);
		ASC->CancelAbilities(&BossAttackTags);
	}
}


void ABossCharacter::GiveStartupAbilitiesAndAttributes()
{
	USAbilitySystemComponent* SASC = Cast<USAbilitySystemComponent>(GetAbilitySystemComponent());
	if (!SASC ||SASC->HasGivenStartupAbilities())
	{
		return;
	}

	if (DefaultAttributesEffect)
	{
		FGameplayEffectContextHandle Context = SASC->MakeEffectContext();
		Context.AddSourceObject(this);

		const FGameplayEffectSpecHandle Spec = SASC->MakeOutgoingSpec(DefaultAttributesEffect, 1.f, Context);
		if (Spec.IsValid())
		{
			SASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
	SASC->AddStartupAbilities(StartupAbilities);
}

bool ABossCharacter::TryActivateAttack(const FGameplayTag& AbilityTag)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !AbilityTag.IsValid())
	{
		return false;
	}
	
	// 갑옷 중 = 대지의 분노 충전 중 (다른 공격을 섞지 않음)
	if (ASC->HasMatchingGameplayTag(SGameplayTags::State_Stun)
		|| ASC->HasMatchingGameplayTag(SGameplayTags::State_Groggy)
		|| ASC->HasMatchingGameplayTag(SGameplayTags::State_Armored)
		|| ASC->HasMatchingGameplayTag(SGameplayTags::State_Dead))
	{
		return false;
	}

	return ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AbilityTag));
}

bool ABossCharacter::IsAttacking() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return ASC && ASC->HasMatchingGameplayTag(SGameplayTags::State_Attacking);
}

bool ABossCharacter::IsDead() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return ASC && ASC->HasMatchingGameplayTag(SGameplayTags::State_Dead);
}

bool ABossCharacter::StartCombat()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController || !BehaviorTree || IsDead())
	{
		UE_LOG(LogTemp, Error, TEXT("[보스] %s 전투 시작 실패: 컨트롤러 %s, 행동 트리 %s, 사망 %s"),
			*GetName(),
			AIController ? TEXT("있음") : TEXT("없음"),
			BehaviorTree ? TEXT("있음") : TEXT("없음"),
			IsDead() ? TEXT("예") : TEXT("아니오"));
		return false;
	}

	bInCombat = AIController->RunBehaviorTree(BehaviorTree);
	return bInCombat;
}

bool ABossCharacter::PlayIntro(const FVector& Landing)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !IntroAbilityTag.IsValid() || IsDead())
	{
		return false;
	}

	// 등장 능력(도약 공격)이 발동할 때 이 위치를 가져감
	LeapTargetOverride = Landing;
	if (!ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(IntroAbilityTag)))
	{
		LeapTargetOverride.Reset();
		UE_LOG(LogTemp, Warning, TEXT("[보스] %s 등장 능력 발동 실패 (%s가 StartupAbilities에 있는지)"), *GetName(), *IntroAbilityTag.ToString());
		return false;
	}
	return true;
}

bool ABossCharacter::IsIntroPlaying() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !IntroAbilityTag.IsValid())
	{
		return false;
	}
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.IsActive() && Spec.Ability && Spec.Ability->GetAssetTags().HasTag(IntroAbilityTag))
		{
			return true;
		}
	}
	return false;
}

void ABossCharacter::SkipIntro(const FVector& Landing)
{
	// 공중에 있어도 능력을 끊으면 몽타주·이동이 같이 멈춤 → 착지 지점(바닥)에 바로 세움. 캡슐 가운데라 반 높이만큼 위
	StopCombat();
	LeapTargetOverride.Reset();
	const FVector Standing = Landing + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	SetActorLocation(Standing, false, nullptr, ETeleportType::TeleportPhysics);
}

bool ABossCharacter::ConsumeLeapTargetOverride(FVector& OutLocation)
{
	if (!LeapTargetOverride.IsSet())
	{
		return false;
	}
	OutLocation = LeapTargetOverride.GetValue();
	LeapTargetOverride.Reset();
	return true;
}

void ABossCharacter::StopCombat()
{
	// 먼저 내림: 아래 취소로 대지의 분노가 끝나도 행동 트리를 다시 켜지 않게
	bInCombat = false;

	// 큰 경직 중이었으면 풀고 일어섬 (플레이어 패배 뒤 쓰러진 채 멈춰 있지 않게)
	ClearGroggy(true);

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->CancelAllAbilities();
	}

	StopAI();
}

void ABossCharacter::StopAI()
{
	// 행동 트리를 멈추고 길 찾기 이동도 멈춤 (뚝 끊지 않고 자연 감속)
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		if (UBrainComponent* Brain = AIController->GetBrainComponent())
		{
			Brain->StopLogic(TEXT("StopAI"));
		}
		AIController->StopMovement();
	}
}

void ABossCharacter::ResumeAI()
{
	if (!bInCombat || IsDead())
	{
		return;
	}

	// 일시 정지 대신 처음부터 다시: 멈춘 사이 이동 노드가 끝 알림을 놓쳐 멈춰 있는 일이 없음 (블랙보드 값은 그대로)
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController && BehaviorTree)
	{
		AIController->RunBehaviorTree(BehaviorTree);
	}
}

void ABossCharacter::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	// 0 이하가 된 첫 번째에만 (죽은 뒤에 또 맞아도 다시 죽지 않게)
	if (Data.NewValue <= 0.f && !IsDead())
	{
		Die();
		return;
	}

	// 2페이즈: 체력이 기준 아래로 처음 내려가면 (진입 능력이 없는 보스도)
	// 피해 처리(AttributeSet) 도중이라 다음 틱에 시작 (그 자리에서 능력을 끊고 켜지 않음)
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (bPhaseTwoEntered || !bInCombat || PhaseTwoHealthRatio <= 0.f || !ASC || IsDead())
	{
		return;
	}
	const float MaxHealth = ASC->GetNumericAttribute(USAttributeSet::GetMaxHealthAttribute());
	if (MaxHealth > 0.f && Data.NewValue / MaxHealth <= PhaseTwoHealthRatio)
	{
		bPhaseTwoEntered = true;
		UE_LOG(LogTemp, Display, TEXT("[보스] 체력 %.0f / %.0f → 다음 틱에 2페이즈"), Data.NewValue, MaxHealth);
		PhaseTwoTimer = GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::EnterPhaseTwo);
	}
}

void ABossCharacter::Die()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || IsDead())
	{
		return;
	}

	// 사망 표시 (한 번만 붙임 → Loose 태그가 쌓이지 않음). 공격 발동·표적 고르기가 이 태그를 봄
	ASC->AddLooseGameplayTag(SGameplayTags::State_Dead);

	// 아직 시작 안 한 대지의 분노·큰 경직 정리 (갑옷은 아래 StopCombat의 능력 취소 → OnAbilityEnded가 벗김)
	GetWorldTimerManager().ClearTimer(PhaseTwoTimer);
	ClearGroggy(false);

	// 진행 중인 공격·행동 트리·이동 정지 (플레이어 패배 때와 같은 처리)
	StopCombat();

	// 쓰러진 몸이 길을 막지 않게 캐릭터끼리 충돌 끔 (바닥과는 그대로 → 떨어지지 않음)
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	// 큰 경직에서 사망 몽타주를 멈춰 둔 상태면 거기서 이어서 쓰러짐 (처음부터 다시 재생하면 일어났다 다시 쓰러짐)
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (DeathMontage && AnimInstance && AnimInstance->Montage_IsActive(DeathMontage))
	{
		AnimInstance->Montage_Resume(DeathMontage);
	}
	else if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}

	// 누가 듣는지는 모름 (전투 진행 액터가 클리어 처리)
	OnBossDied.Broadcast(this);
}



void ABossCharacter::EnterPhaseTwo()
{
	if (IsDead() || !bInCombat)
	{
		return;
	}

	PatternComponent->SetPhase(2);
	UE_LOG(LogTemp, Display, TEXT("[보스] %s 2페이즈 (진입 능력 %s)"), *GetName(), RageAbilityTag.IsValid() ? *RageAbilityTag.ToString() : TEXT("없음"));

	// 바뀌었다는 걸 알림: HUD 안내 글자 + 화면 흔들림
	OnPhaseChanged.Broadcast(2);
	if (PhaseTwoShake)
	{
		UGameplayStatics::PlayWorldCameraShake(
			this,
			PhaseTwoShake,
			GetActorLocation(),
			1000.f,
			4000.f);
	}

	// 진입 능력이 있는 보스(골렘)는 그 능력이 끝날 때(EndArmor) 속도를 올림
	if (RageAbilityTag.IsValid())
	{
		StartEarthRage();
		return;
	}
	ApplyPhaseTwoSpeed();
}

void ABossCharacter::ApplyPhaseTwoSpeed()
{
	if (!bPhaseTwoApplied)
	{
		bPhaseTwoApplied = true;
		GetCharacterMovement()->MaxWalkSpeed *= PhaseTwoSpeedScale;
	}
}

void ABossCharacter::StartEarthRage()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || IsDead() || !bInCombat)
	{
		UE_LOG(LogTemp, Warning, TEXT("[보스] 대지의 분노 시작 안 함: 사망 %d, 전투 중 %d"), IsDead(), bInCombat);
		return;
	}
	UE_LOG(LogTemp, Display, TEXT("[보스] 대지의 분노 시작: 표식 폭발 %d번이면 갑옷 파괴"), ArmorHitsRequired);

	// 행동 트리부터 멈춤 (진행 중인 패턴 작업이 끝나며 자기 공격을 취소) → 남은 보스 공격도 취소
	StopAI();
	const FGameplayTagContainer BossAttackTags(SGameplayTags::Ability_Boss);
	ASC->CancelAbilities(&BossAttackTags);

	// 갑옷을 능력보다 먼저: 능력이 바로 끝나도(몽타주 실패) OnAbilityEnded가 갑옷을 보고 정리함
	ASC->AddLooseGameplayTag(SGameplayTags::State_Armored);
	ArmorHitsLeft = ArmorHitsRequired;

	// 2페이즈로 넘긴 그 표식 폭발이 바로 1회로 세지지 않게 지금부터 간격을 잼
	LastArmorHitTime = GetWorld()->GetTimeSeconds();

	SetArmorVisible(true);
	OnArmorChanged.Broadcast(EBossArmorEvent::Raised);

	if (!ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(RageAbilityTag)))
	{
		UE_LOG(LogTemp, Warning, TEXT("[보스] %s 대지의 분노 발동 실패 (%s가 StartupAbilities에 있는지, 막는 태그가 있는지)"), *GetName(), *RageAbilityTag.ToString());
		EndArmor(false);
	}
}

void ABossCharacter::NotifyMarkBurst()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || IsDead() || !ASC->HasMatchingGameplayTag(SGameplayTags::State_Armored))
	{
		UE_LOG(LogTemp, Display, TEXT("[보스] 표식 폭발 받음 (갑옷 없음 → 세지 않음)"));
		return;
	}

	// 균열 연쇄처럼 한 번에 여러 번 들어와도 1번
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastArmorHitTime < ArmorHitInterval)
	{
		UE_LOG(LogTemp, Display, TEXT("[보스] 표식 폭발 받음 (%.2f초 전에 셈 → 같은 폭발로 봄)"), Now - LastArmorHitTime);
		return;
	}
	LastArmorHitTime = Now;

	ArmorHitsLeft = FMath::Max(ArmorHitsLeft - 1, 0);
	UE_LOG(LogTemp, Display, TEXT("[보스] 갑옷 표식 폭발 %d / %d"), ArmorHitsRequired - ArmorHitsLeft, ArmorHitsRequired);
	if (ArmorHitsLeft > 0)
	{
		RefreshArmorGlow();
		OnArmorChanged.Broadcast(EBossArmorEvent::Hit);
		return;
	}

	EndArmor(true);
}

void ABossCharacter::EndArmor(bool bBroken)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !ASC->HasMatchingGameplayTag(SGameplayTags::State_Armored))
	{
		return;
	}

	// 갑옷부터 벗김: 아래 큰 경직이 충전 능력을 취소하면 OnAbilityEnded가 다시 부르는데, 그때는 할 일이 없게
	ASC->RemoveLooseGameplayTag(SGameplayTags::State_Armored);
	ArmorHitsLeft = 0;
	SetArmorVisible(false);

	// 깨든 못 깨든 여기부터 더 빨리 걸어옴 (한 번만)
	ApplyPhaseTwoSpeed();

	UE_LOG(LogTemp, Display, TEXT("[보스] 갑옷 %s"), bBroken ? TEXT("파괴 → 큰 경직") : TEXT("못 깸 (강타 또는 중단)"));
	OnArmorChanged.Broadcast(bBroken ? EBossArmorEvent::Broken : EBossArmorEvent::Expired);

	if (!bBroken)
	{
		ResumeAI();
		return;
	}

	// 깨진 순간: 가슴 높이에서 바위가 튀고 화면이 크게 흔들림
	const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	const FVector Chest = Feet + FVector(0.f, 0.f, ArmorBreakEffectHeight);
	USAbilityFunctionLibrary::SpawnOneShotEffect(
		this,
		ArmorBreakEffect,
		Chest,
		FRotator::ZeroRotator,
		FVector(ArmorBreakEffectScale));

	if (ArmorBreakShake)
	{
		UGameplayStatics::PlayWorldCameraShake(
			this,
			ArmorBreakShake,
			Chest,
			1000.f,
			4000.f);
	}

	BeginGroggy(GroggyDuration);
}

void ABossCharacter::NotifyCountered()
{
	// 이미 누워 있으면(남은 탄이 연달아 들어옴) 늘리지 않음. 갑옷 중(대지의 분노)에는 갑옷 깨기가 우선
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (CounterGroggyDuration <= 0.f
		|| IsDead()
		|| bGroggyActive
		|| !ASC
		|| ASC->HasMatchingGameplayTag(SGameplayTags::State_Armored))
	{
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("[보스] 반격당함 → 큰 경직 %.1f초"), CounterGroggyDuration);
	OnArmorChanged.Broadcast(EBossArmorEvent::Countered);
	BeginGroggy(CounterGroggyDuration);
}

void ABossCharacter::SetArmorVisible(bool bVisible)
{
	if (!bVisible)
	{
		GetMesh()->SetOverlayMaterial(nullptr);
		return;
	}

	if (!ArmorOverlayMaterial)
	{
		return;
	}
	if (!ArmorOverlayInstance)
	{
		ArmorOverlayInstance = UMaterialInstanceDynamic::Create(ArmorOverlayMaterial, this);
	}
	RefreshArmorGlow();
	GetMesh()->SetOverlayMaterial(ArmorOverlayInstance);
}

void ABossCharacter::RefreshArmorGlow()
{
	if (ArmorOverlayInstance && ArmorHitsRequired > 0)
	{
		const float Ratio = static_cast<float>(ArmorHitsLeft) / static_cast<float>(ArmorHitsRequired);
		ArmorOverlayInstance->SetScalarParameterValue(ArmorGlowParameter, Ratio);
	}
}

UAnimMontage* ABossCharacter::GetGroggyMontage() const
{
	return GroggyMontage ? GroggyMontage.Get() : DeathMontage.Get();
}

void ABossCharacter::BeginGroggy(float Duration)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || IsDead() || bGroggyActive)
	{
		return;
	}
	bGroggyActive = true;

	// 걸어 다니지 않게 행동 트리를 멈추고, 태그 알림(OnGroggyTagChanged)이 진행 중인 보스 공격(충전)을 취소
	StopAI();
	ASC->AddLooseGameplayTag(SGameplayTags::State_Groggy);

	if (UAnimMontage* Montage = GetGroggyMontage())
	{
		PlayAnimMontage(Montage);
		if (GroggyPoseTime > 0.f)
		{
			// 쓰러지기 직전 자세에서 멈춤 (몽타주 재생 속도 1 기준)
			GetWorldTimerManager().SetTimer(GroggyPoseTimer, this, &ThisClass::HoldGroggyPose, GroggyPoseTime, false);
		}
	}

	GetWorldTimerManager().SetTimer(GroggyEndTimer, this, &ThisClass::EndGroggy, Duration, false);
}

void ABossCharacter::HoldGroggyPose()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	UAnimMontage* Montage = GetGroggyMontage();
	if (AnimInstance && Montage && AnimInstance->Montage_IsActive(Montage))
	{
		AnimInstance->Montage_Pause(Montage);
	}
}

void ABossCharacter::EndGroggy()
{
	ClearGroggy(true);
	ResumeAI();
}

void ABossCharacter::ClearGroggy(bool bStopMontage)
{
	GetWorldTimerManager().ClearTimer(GroggyPoseTimer);
	GetWorldTimerManager().ClearTimer(GroggyEndTimer);

	if (!bGroggyActive)
	{
		return;
	}
	bGroggyActive = false;

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->RemoveLooseGameplayTag(SGameplayTags::State_Groggy);
	}

	// 멈춰 둔 자세에서 원래 자세(대기)로 섞으며 일어섬
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	UAnimMontage* Montage = GetGroggyMontage();
	if (bStopMontage && AnimInstance && Montage && AnimInstance->Montage_IsActive(Montage))
	{
		AnimInstance->Montage_Stop(GroggyRecoverBlendTime, Montage);
	}
}

void ABossCharacter::OnAbilityEnded(const FAbilityEndedData& Data)
{
	if (!Data.AbilityThatEnded || !RageAbilityTag.IsValid() || !Data.AbilityThatEnded->GetAssetTags().HasTag(RageAbilityTag))
	{
		return;
	}

	// 갑옷이 남아 있으면 못 깬 것 (강타가 나갔거나, 패배·사망으로 끊김). 깨졌으면 이미 벗겨져서 아무 일 없음
	EndArmor(false);
}
