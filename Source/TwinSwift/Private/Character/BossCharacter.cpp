
#include "Character/BossCharacter.h"

#include "SGameplayTags.h"
#include "Ability/SAbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AI/SAIController.h"
#include "AI/SBossPatternComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Ability/SAttributeSet.h"
#include "Components/CapsuleComponent.h"

ABossCharacter::ABossCharacter()
{
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	
	GetCharacterMovement()->MaxWalkSpeed = 300.f;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 180.f, 0.f);

	PatternComponent = CreateDefaultSubobject<USBossPatternComponent>("PatternComponent");

	AIControllerClass = ASAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
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
	}
}

void ABossCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->RegisterGameplayTagEvent(SGameplayTags::State_Groggy, EGameplayTagEventType::NewOrRemoved).Remove(GroggyTagHandle);
		ASC->GetGameplayAttributeValueChangeDelegate(USAttributeSet::GetHealthAttribute()).Remove(HealthChangedHandle);
	}

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
	
	if (ASC->HasMatchingGameplayTag(SGameplayTags::State_Stun)
		|| ASC->HasMatchingGameplayTag(SGameplayTags::State_Groggy)
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

	return AIController->RunBehaviorTree(BehaviorTree);
}

void ABossCharacter::StopCombat()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->CancelAllAbilities();
	}

	// 행동 트리를 멈추고 길 찾기 이동도 멈춤 (뚝 끊지 않고 자연 감속)
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		if (UBrainComponent* Brain = AIController->GetBrainComponent())
		{
			Brain->StopLogic(TEXT("StopCombat"));
		}
		AIController->StopMovement();
	}
}

void ABossCharacter::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	// 0 이하가 된 첫 번째에만 (죽은 뒤에 또 맞아도 다시 죽지 않게)
	if (Data.NewValue <= 0.f && !IsDead())
	{
		Die();
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

	// 진행 중인 공격·행동 트리·이동 정지 (플레이어 패배 때와 같은 처리)
	StopCombat();

	// 쓰러진 몸이 길을 막지 않게 캐릭터끼리 충돌 끔 (바닥과는 그대로 → 떨어지지 않음)
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}

	// 누가 듣는지는 모름 (전투 진행 액터가 클리어 처리)
	OnBossDied.Broadcast(this);
}


