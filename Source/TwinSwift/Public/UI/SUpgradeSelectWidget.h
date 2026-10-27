#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SUpgradeSelectWidget.generated.h"

class USUpgradeData;

// 고른 강화를 게임 모드에 알림
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUpgradeChosen, USUpgradeData*);

/**
 * 보스 사이 강화 카드 화면 (WBP_UpgradeSelect, 화면 가운데 카드 3장)
 * 카드는 이름 규칙으로 찾음: Card0Button / Card0Title / Card0Description / Card0Target … Card2
 * 마우스 클릭과 키보드 1/2/3 둘 다. 고르면 알리기만 하고, 닫기·게임 다시 돌리기는 게임 모드가 함
 */
UCLASS()
class TWINSWIFT_API USUpgradeSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 게임 모드가 뽑은 카드(최대 3장)를 넘김 → 글자 채우고, 남는 카드는 숨기고, 키 입력 받을 준비
	void ShowChoices(const TArray<USUpgradeData*>& InChoices);

	FOnUpgradeChosen OnUpgradeChosen;

protected:
	// 카드 버튼 3개의 클릭 연결, 키 입력을 받을 수 있게 (포커스 가능)
	virtual void NativeConstruct() override;

	// 키보드 1/2/3 → 카드 선택. 다른 키는 부모에게 넘김
	virtual FReply NativeOnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	// 버튼 클릭은 인자가 없어 어느 카드인지 모름 → 버튼마다 하나씩
	UFUNCTION()
	void OnCard0Clicked();

	UFUNCTION()
	void OnCard1Clicked();

	UFUNCTION()
	void OnCard2Clicked();

	// 클릭·키 모두 여기로. 이미 골랐거나 없는 카드면 무시
	void Choose(int32 Index);

	// 지금 보여 준 카드 (GC가 지우지 않게 UPROPERTY)
	UPROPERTY()
	TArray<TObjectPtr<USUpgradeData>> Choices;

	// 두 번 고르기 방지 (클릭과 키가 같은 프레임에 들어와도 한 번만)
	bool bChosen = false;
};
