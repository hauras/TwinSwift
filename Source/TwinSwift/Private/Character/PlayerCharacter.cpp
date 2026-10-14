#include "Character/PlayerCharacter.h"

#include "Ability/SAbilitySystemComponent.h"
#include "Ability/SAttributeSet.h"
#include "Camera/CameraComponent.h"
#include "Camera/SCameraConfig.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "State/SPlayerState.h"

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
	// 오른손 뼈에 붙임. 손 안에서의 위치·각도·크기는 BP의 WeaponMesh 기본값으로 맞춤
	WeaponMesh->SetupAttachment(GetMesh(), TEXT("hand_R"));

	// 타격 판정은 나중에 트레이스로 함. 무기 자체는 충돌 없음 (캐릭터 이동을 막지 않게)
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// BP에서 RunSpeed를 바꿨을 수 있으니 시작할 때 다시 적용
	ApplyWalkSpeed(false);
}

void APlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	InitAbilityActorInfo();
	GiveStartupAbilitiesAndAttributes();
}

void APlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	InitAbilityActorInfo();
}

void APlayerCharacter::InitAbilityActorInfo()
{
	ASPlayerState* SPlayerState = GetPlayerState<ASPlayerState>();
	if (!SPlayerState)
	{
		return;
	}

	// 주인 = PlayerState(능력·스탯 보관), 몸 = 이 캐릭터(애니·위치)
	// 서버는 PossessedBy, 클라는 OnRep_PlayerState에서 각각 한 번씩만 불림
	// 1차와 달리 여기서 Loose 태그를 붙이거나 위젯을 만들지 않음 (불릴 때마다 쌓였음)
	USAbilitySystemComponent* SASC = SPlayerState->GetSAbilitySystemComponent();
	SASC->InitAbilityActorInfo(SPlayerState, this);

	AbilitySystemComponent = SASC;
	AttributeSet = SPlayerState->GetAttributeSet();
}

void APlayerCharacter::GiveStartupAbilitiesAndAttributes()
{
	USAbilitySystemComponent* SASC = Cast<USAbilitySystemComponent>(AbilitySystemComponent);
	if (!HasAuthority() || !SASC || SASC->HasGivenStartupAbilities())
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
	// 내 쪽은 바로 바꿔서 입력 반응이 늦지 않게
	ApplyWalkSpeed(bNewWalking);

	if (!HasAuthority())
	{
		Server_SetWalking(bNewWalking);
	}
}

void APlayerCharacter::Server_SetWalking_Implementation(bool bNewWalking)
{
	ApplyWalkSpeed(bNewWalking);
}

void APlayerCharacter::ApplyWalkSpeed(bool bNewWalking)
{
	GetCharacterMovement()->MaxWalkSpeed = bNewWalking ? WalkSpeed : RunSpeed;
}
