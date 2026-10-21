#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h"
#include "Game/SBossEncounter.h"
#include "SBossHUDWidget.generated.h"

class UAbilitySystemComponent;
class UProgressBar;
class UTextBlock;

/**
 * 보스 HUD: 화면 위쪽 보스 이름·체력바 + 결과 글자
 * 전투 진행 액터(ASBossEncounter)의 상태 알림을 듣고 보이기·숨기기, 보스 체력은 GAS 체력 변경 알림으로만 갱신
 * 배치는 WBP_BossHUD, 아래 이름의 위젯을 코드가 찾아 씀 (BindWidget: 이름이 다르면 WBP 컴파일 오류로 알려 줌)
 */
UCLASS()
class TWINSWIFT_API USBossHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 전투 진행 액터가 위젯을 만든 직후 부름
	void BindToEncounter(ASBossEncounter* InEncounter);

protected:
	virtual void NativeDestruct() override;

	// 이름·체력바를 묶은 위쪽 영역 (전투 중에만 보임)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> BossPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BossNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> BossHealthBar;

	// "클리어 82.4초 · R: 다시 시작" / "패배 · R: 다시 시작"
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ResultText;

private:
	void Unbind();

	void OnEncounterStateChanged(EBossEncounterState NewState);

	void OnBossHealthChanged(const FOnAttributeChangeData& Data);

	// 체력바를 ASC 값으로 다시 그림
	void RefreshHealth();

	TWeakObjectPtr<ASBossEncounter> Encounter;
	TWeakObjectPtr<UAbilitySystemComponent> BossASC;

	FDelegateHandle StateChangedHandle;
	FDelegateHandle HealthChangedHandle;
	FDelegateHandle MaxHealthChangedHandle;
};
