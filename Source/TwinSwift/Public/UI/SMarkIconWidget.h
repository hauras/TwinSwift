#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SMarkIconWidget.generated.h"

class UImage;
class UMaterialInterface;
class UProgressBar;
class USMarkIndicatorComponent;
class UWidget;

/**
 * 머리 위 표식 아이콘 위젯 (USMarkIndicatorComponent가 띄움). WBP 배치를 사용하며, 미지정 시 기본 배치를 코드로 만듦
 * - [균열]·[빙결] 아이콘 두 칸, 각 아이콘 아래 남은 시간 막대
 * - 붙은 표식만 보이게. 남은 시간이 1.5초 이하면 깜빡임 ("곧 사라짐")
 * - 남은 시간 갱신은 위젯이 보이는 동안만 (컴포넌트가 숨기면 위젯도 그려지지 않음)
 */
UCLASS()
class TWINSWIFT_API USMarkIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 컴포넌트가 처음 한 번: 남은 시간을 물어볼 곳과 아이콘 머티리얼
	void Setup(USMarkIndicatorComponent* InOwner, UMaterialInterface* CrackIcon, UMaterialInterface* FrostIcon);

	// 붙은 표식만 보이게
	void SetMarks(bool bCrack, bool bFrost);

protected:
	// 위젯 트리를 코드로 만듦 (가로로 아이콘 두 칸)
	virtual TSharedRef<SWidget> RebuildWidget() override;

	virtual void NativeConstruct() override;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	// 아이콘 한 칸 (아이콘 + 남은 시간 막대)
	struct FMarkSlot
	{
		TObjectPtr<UWidget> Root;
		TObjectPtr<UImage> Icon;
		TObjectPtr<UProgressBar> TimeBar;
	};

	// 한 칸 만들기
	FMarkSlot MakeSlot(const FLinearColor& BarColor);

	// 한 칸의 남은 시간 막대·깜빡임 갱신
	void UpdateSlot(const FMarkSlot& MarkSlot, const struct FGameplayTag& MarkTag, float Time);

	// 아이콘 머티리얼 넣기 (Setup이 위젯 트리보다 먼저 올 수 있어서 두 곳에서 부름)
	void ApplyIcons();

	FMarkSlot CrackSlot;
	FMarkSlot FrostSlot;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> CrackMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> FrostMaterial;

	TWeakObjectPtr<USMarkIndicatorComponent> OwnerComponent;

	// 깜빡임용 시간
	float BlinkTime = 0.f;

	bool bShowCrack = false;
	bool bShowFrost = false;
};
