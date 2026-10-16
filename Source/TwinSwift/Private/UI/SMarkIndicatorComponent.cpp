#include "UI/SMarkIndicatorComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "SGameplayTags.h"
#include "UI/SMarkIconWidget.h"

USMarkIndicatorComponent::USMarkIndicatorComponent()
{
	// 화면 공간: 카메라 거리와 상관없이 같은 크기, 항상 카메라를 봄
	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawSize(FVector2D(96.f, 52.f));
	SetWidgetClass(USMarkIconWidget::StaticClass());

	// 머리 위 (캡슐 가운데 기준)
	SetRelativeLocation(FVector(0.f, 0.f, 140.f));

	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);

	// 표식이 붙기 전에는 안 보임
	SetHiddenInGame(true);
}

void USMarkIndicatorComponent::BeginPlay()
{
	Super::BeginPlay();

	// 숨겨진 상태에서도 위젯을 먼저 만들어 초기 표식 상태를 전달함
	InitWidget();

	OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (UAbilitySystemComponent* ASC = OwnerASC.Get())
	{
		// 태그가 새로 붙거나(0 → 1) 다 떨어질 때(1 → 0)만 알림
		CrackHandle = ASC->RegisterGameplayTagEvent(SGameplayTags::Mark_Crack, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &ThisClass::OnMarkTagChanged);
		FrostHandle = ASC->RegisterGameplayTagEvent(SGameplayTags::Mark_Frost, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &ThisClass::OnMarkTagChanged);
	}

	if (USMarkIconWidget* Icons = Cast<USMarkIconWidget>(GetUserWidgetObject()))
	{
		Icons->Setup(this, CrackIcon, FrostIcon);
	}
	RefreshIcons();
}

void USMarkIndicatorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAbilitySystemComponent* ASC = OwnerASC.Get())
	{
		ASC->RegisterGameplayTagEvent(SGameplayTags::Mark_Crack, EGameplayTagEventType::NewOrRemoved).Remove(CrackHandle);
		ASC->RegisterGameplayTagEvent(SGameplayTags::Mark_Frost, EGameplayTagEventType::NewOrRemoved).Remove(FrostHandle);
	}

	Super::EndPlay(EndPlayReason);
}

bool USMarkIndicatorComponent::GetMarkTimeRemaining(const FGameplayTag& MarkTag, float& OutFraction, float& OutSeconds) const
{
	OutFraction = 0.f;
	OutSeconds = 0.f;
	const UAbilitySystemComponent* ASC = OwnerASC.Get();
	if (!ASC)
	{
		return false;
	}

	// 표식은 공통 시간 태그 GE에 태그를 붙여 건 것 → 그 태그를 주는 효과의 남은 시간
	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(MarkTag));
	const TArray<TPair<float, float>> Times = ASC->GetActiveEffectsTimeRemainingAndDuration(Query);
	if (Times.Num() == 0)
	{
		return false;
	}

	// 같은 표식이 여러 번 걸렸으면 가장 오래 남는 것
	OutSeconds = 0.f;
	OutFraction = 0.f;
	for (const TPair<float, float>& Time : Times)
	{
		if (Time.Key > OutSeconds)
		{
			OutSeconds = Time.Key;
			OutFraction = Time.Value > 0.f ? Time.Key / Time.Value : 0.f;
		}
	}
	return true;
}

void USMarkIndicatorComponent::OnMarkTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	RefreshIcons();
}

void USMarkIndicatorComponent::RefreshIcons()
{
	const UAbilitySystemComponent* ASC = OwnerASC.Get();
	const bool bCrack = ASC && ASC->HasMatchingGameplayTag(SGameplayTags::Mark_Crack);
	const bool bFrost = ASC && ASC->HasMatchingGameplayTag(SGameplayTags::Mark_Frost);

	if (USMarkIconWidget* Icons = Cast<USMarkIconWidget>(GetUserWidgetObject()))
	{
		Icons->SetMarks(bCrack, bFrost);
	}
	SetHiddenInGame(!bCrack && !bFrost);
}
