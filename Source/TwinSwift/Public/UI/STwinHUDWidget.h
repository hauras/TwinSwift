#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Character/TwinSlot.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "STwinHUDWidget.generated.h"

class APlayerCharacter;
class UAbilitySystemComponent;
class UImage;
class UProgressBar;
class UTextBlock;
class UMaterialInterface;
struct FOnAttributeChangeData;

/**
 * 전투 HUD의 로직 (화면 배치는 이 클래스를 부모로 한 위젯 블루프린트 WBP_TwinHUD에서)
 * - 체력: 매 프레임 읽지 않고 GAS 능력치가 바뀔 때만 체력바·숫자 갱신
 * - 교체: 현재 캐릭터 카드·스킬은 크게, 대기 카드는 작게 표시
 * - 컨트롤러가 한 번만 만들고, 교체해도 다시 만들지 않음 (1차: 교체마다 위젯이 쌓였음)
 *
 * WBP에서 아래 이름의 위젯을 만들면 코드가 찾아 씀 (BindWidget: 이름이 다르면 WBP 컴파일 오류로 알려 줌)
 *   WarriorCard / MageCard             : 카드 전체를 감싸는 위젯 (Border, Overlay 등 아무거나)
 *   WarriorHealthBar / MageHealthBar   : 체력바 (Progress Bar)
 *   WarriorHealthText / MageHealthText : "120 / 120" 숫자 (Text, 없어도 됨)
 * 교체 규칙 (모두 없어도 됨)
 *   WarriorSwapCooldown / MageSwapCooldown         : 대기 카드를 덮는 쿨다운 덮개 (Progress Bar, 1 → 0으로 걷힘)
 *   WarriorSwapCooldownText / MageSwapCooldownText : 남은 시간 "1.2" (Text)
 *   WarriorDownText / MageDownText                 : "쓰러짐" (Text)
 *   DefeatText                                     : "패배" (Text, 화면 가운데)
 *   PerfectEvadeText                               : "완벽 회피!" (Text, 슬로 동안만 보임)
 */
UCLASS(Abstract)
class TWINSWIFT_API USTwinHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 캐릭터의 두 ASC 체력과 교체 알림에 연결. 다시 불러도 예전 연결은 끊고 새로 연결
	void BindToCharacter(APlayerCharacter* InCharacter);

protected:
	virtual void NativeDestruct() override;

	// 교체됐을 때 블루프린트에서 꾸밈을 더하고 싶으면 (애니메이션 재생 등). 크기·투명도는 C++이 이미 바꿈
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnActiveSlotChangedBP(ETwinSlot NewSlot);

	// 대기 카드 크기 (1 = 그대로). 카드의 Render Transform Pivot은 (0, 0): 왼쪽 위 기준으로 작아짐
	UPROPERTY(EditDefaultsOnly, Category = "HUD", meta = (ClampMin = "0.3", ClampMax = "1"))
	float BenchScale = 0.75f;

	// 대기 카드 투명도 (1 = 그대로)
	UPROPERTY(EditDefaultsOnly, Category = "HUD", meta = (ClampMin = "0", ClampMax = "1"))
	float BenchOpacity = 0.7f;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> WarriorCard;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> MageCard;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> WarriorHealthBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> MageHealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WarriorHealthText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MageHealthText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> WarriorSwapCooldown;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> MageSwapCooldown;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WarriorSwapCooldownText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MageSwapCooldownText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WarriorDownText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MageDownText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DefeatText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PerfectEvadeText;

	// 맞았을 때 화면 가장자리 빨간 번쩍 (화면 전체 이미지, 가운데는 투명한 M_HUD_HitVignette)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> HitFlash;

	// 번쩍 최대 투명도와 사라지는 시간(실제 초)
	UPROPERTY(EditDefaultsOnly, Category = "HUD", meta = (ClampMin = "0", ClampMax = "1"))
	float HitFlashPeakOpacity = 0.7f;

	UPROPERTY(EditDefaultsOnly, Category = "HUD", meta = (ClampMin = "0.05"))
	float HitFlashDuration = 0.35f;

	// 쓰러진 카드 투명도
	UPROPERTY(EditDefaultsOnly, Category = "HUD", meta = (ClampMin = "0", ClampMax = "1"))
	float DownOpacity = 0.35f;

private:
	// 캐릭터 하나의 ASC 연결 정보
	struct FSlotBinding
	{
		TWeakObjectPtr<UAbilitySystemComponent> ASC;
		FDelegateHandle HealthHandle;
		FDelegateHandle MaxHealthHandle;
	};

	// 스킬은 0.1초 간격으로 갱신. 체력 갱신은 기존 GAS 알림을 유지한다.
	void RefreshSkillHUD();
	void SetHUDText(FName Name, const FString& Value);
	FString GetInputKeyLabel(FGameplayTag Input, const FString& Fallback) const;

	// 스킬 아이콘 (이름 → 아이콘 머티리얼). WBP_TwinHUD에서 지정
	// 코드에 경로 문자열로 두면 패키징 때 빠질 수 있어서(문자열은 참조로 안 잡힘) 에셋에서 직접 참조
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TMap<FName, TObjectPtr<UMaterialInterface>> SkillMaterials;

	FTimerHandle SkillRefreshTimer;

	// 예전 ASC·캐릭터 연결을 모두 끊음
	void Unbind();

	// 체력·최대 체력이 바뀌면 GAS가 부름 (CharSlot은 연결할 때 넣어 둔 값. 이름이 Slot이면 UWidget::Slot을 가려서 빌드 오류)
	void OnHealthChanged(const FOnAttributeChangeData& Data, ETwinSlot CharSlot);

	// 카드의 체력바·숫자를 ASC 값으로 다시 그림
	void RefreshHealth(ETwinSlot CharSlot);

	// 교체되면 활성 카드는 왼쪽, 대기 카드는 오른쪽으로 배치
	void OnActiveSlotChanged(ETwinSlot NewSlot);

	UWidget* GetCardWidget(ETwinSlot CharSlot) const;
	UProgressBar* GetHealthBar(ETwinSlot CharSlot) const;
	UTextBlock* GetHealthText(ETwinSlot CharSlot) const;

	// ---------- 교체 규칙 표시 ----------

	// 교체 직후: 쿨다운 덮개를 움직이는 짧은 타이머 시작 (쿨다운이 끝나면 멈춤)
	void StartSwapCooldownDisplay();

	// 대기 카드의 쿨다운 덮개·남은 시간 갱신
	void UpdateSwapCooldownDisplay();

	// 쓰러진 카드 흐리게 + "쓰러짐", 패배면 "패배"
	void RefreshDownStates();

	void OnSlotDown(ETwinSlot DownSlot);
	void OnDefeated();

	// 완벽 회피 슬로 시작·끝 → 글자 보이기·숨기기
	void OnPerfectEvade(bool bStarted);
	FDelegateHandle PerfectEvadeHandle;

	// 맞음 → 빨간 번쩍 시작 / 짧은 타이머로 투명도를 줄여 감
	void OnPlayerDamaged(float Damage);
	void UpdateHitFlash();
	FDelegateHandle PlayerDamagedHandle;
	FTimerHandle HitFlashTimer;

	// 번쩍이 시작된 실제 시간 (완벽 회피 슬로 중에도 같은 속도로 사라지게 실제 시간 기준)
	double HitFlashStartTime = 0.0;

	FTimerHandle SwapCooldownTimer;
	FDelegateHandle SlotDownHandle;
	FDelegateHandle DefeatedHandle;

	FSlotBinding WarriorBinding;
	FSlotBinding MageBinding;

	TWeakObjectPtr<APlayerCharacter> BoundCharacter;
	FDelegateHandle ActiveSlotChangedHandle;
};

