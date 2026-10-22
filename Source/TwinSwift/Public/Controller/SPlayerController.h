#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "UI/DamageTextActor.h"
#include "SPlayerController.generated.h"

class APlayerCharacter;
class ASProjectile;
class UCameraShakeBase;
class UInputAction;
class UInputMappingContext;
class USCameraConfig;
class USInputConfig;
class USAbilitySystemComponent;
class USInputBufferComponent;
class USTwinHUDWidget;
struct FInputActionValue;
enum class ETwinSlot : uint8;

UCLASS()
class TWINSWIFT_API ASPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASPlayerController();

	// 스킬이 겨냥할 위치. 시점마다 구하는 방법만 다르고, 스킬은 이 함수만 부름
	UFUNCTION(BlueprintCallable, Category = "Aim")
	FVector GetAimPoint() const;

	// HUD 키 표시는 실제 입력 매핑에서 읽는다.
	const USInputConfig* GetHUDInputConfig() const { return AbilityInputConfig; }
	const UInputMappingContext* GetHUDMappingContext() const { return DefaultMappingContext; }

	// 시점 바꾸기 (CameraConfigs 순서대로)
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void CycleCameraConfig();

	// 내가 때린 데미지 숫자를 대상 머리 위에 띄움
	void ShowDamageNumber(float Damage, AActor* Target, EDamageNumberType Type = EDamageNumberType::Normal);

	// 내 캐릭터가 맞음 (APlayerCharacter::NotifyDamaged가 부름): 빨간 숫자 + 화면 흔들림. 빨간 번쩍은 HUD가
	void OnPawnDamaged(float Damage);

	// [디버그] 겨냥 위치를 구로 표시 (콘솔: ShowAim)
	UFUNCTION(Exec)
	void ShowAim();

	// [디버그] 지금 나와 있는 캐릭터에게 데미지 (콘솔: DebugDamage 50). 적이 아직 플레이어를 때리지 않아서 교체 규칙 시험용
	UFUNCTION(Exec)
	void DebugDamage(float Amount);

	// [디버그] 맵의 모든 적에게 표식 10초 (콘솔: DebugMarkAll Crack / DebugMarkAll Frost). 연쇄 폭발·산산조각 시험용
	UFUNCTION(Exec)
	void DebugMarkAll(const FString& MarkName);

	// [측정] 맵의 첫 보스 패턴 선택기로 Count번 고르기 시뮬레이션 → 선택기 vs 완전 무작위 (콘솔: BossPatternSim 1000)
	UFUNCTION(Exec)
	void BossPatternSim(int32 Count = 1000);

	// [측정] 투사체를 초당 PerSecond발씩 Seconds초 동안 자동 발사 (콘솔: BenchProjectiles 60 20)
	// 오브젝트 풀 전후 비교용: 매번 같은 개수·같은 방향(가장 가까운 적 쪽, 고정 시드). 끝나면 생성 시간·프레임·GC를 로그로
	// 단독 실행 인자 -Bench=60,20 이면 시작 3초 뒤 자동, -BenchQuit이면 끝나고 종료
	UFUNCTION(Exec)
	void BenchProjectiles(float PerSecond, float Seconds);

	// 능력·교체 입력을 실제로 실행 (입력 버퍼가 "지금 해도 된다"고 판단한 뒤 부름)
	// 교체(InputTag.Swap)면 교체, 나머지는 그 입력을 받을 캐릭터의 ASC로 (상대 캐릭터 스킬 키면 먼저 교체)
	void ExecuteAbilityInput(const FGameplayTag& InputTag);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void AcknowledgePossession(APawn* P) override;
	virtual void PlayerTick(float DeltaTime) override;

	// 비교할 시점들. 첫 번째가 시작 시점
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	TArray<TObjectPtr<USCameraConfig>> CameraConfigs;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> WalkAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> CycleCameraAction;

	// 캐릭터 교체 (전사 ↔ 마법사)
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SwapAction;

	// 능력 입력 목록 (IA_Attack → InputTag.Attack ...)
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<USInputConfig> AbilityInputConfig;

	// 전투 HUD 위젯 블루프린트 (WBP_TwinHUD, 부모 USTwinHUDWidget)
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USTwinHUDWidget> HUDWidgetClass;

	// 표식 피해(산산조각·균열 폭발) 때 화면 흔들림 (기본 USCameraShake_Shatter). 비우면 흔들지 않음
	UPROPERTY(EditDefaultsOnly, Category = "Feedback")
	TSubclassOf<UCameraShakeBase> MarkCameraShake;

	// 내 캐릭터가 맞았을 때 화면 흔들림 (기본 USCameraShake_Shatter, 짧은 "툭"). 비우면 흔들지 않음
	UPROPERTY(EditDefaultsOnly, Category = "Feedback")
	TSubclassOf<UCameraShakeBase> HurtCameraShake;

	// [측정] BenchProjectiles가 쏠 투사체 (화염탄 BP)
	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	TSubclassOf<ASProjectile> BenchProjectileClass;

private:
	// [측정] 한 발 생성 + 시간 기록. 다 쏘면 결과 로그
	void BenchFireOne();

	FTimerHandle BenchTimer;

	// 남은 발 수, 쏜 발 수
	int32 BenchRemaining = 0;
	int32 BenchFired = 0;

	// 생성에 걸린 시간 합계·최대 (초)
	double BenchSpawnSeconds = 0.0;
	double BenchMaxSpawnSeconds = 0.0;

	// 측정 시작 시 고정한 발사 방향 (가장 가까운 적 쪽)과 퍼짐용 고정 시드 난수
	FVector BenchDirection = FVector::ForwardVector;
	FRandomStream BenchRandom;

	// 측정 중 프레임 시간(초) 기록 → 평균·최대·상위 1%
	TArray<float> BenchFrameTimes;

	// 측정 중 GC 횟수와 걸린 시간 합계·최대 (초)
	int32 BenchGCCount = 0;
	double BenchGCSeconds = 0.0;
	double BenchGCMaxSeconds = 0.0;
	double BenchGCStart = 0.0;

	FDelegateHandle BenchFrameHandle;
	FDelegateHandle BenchPreGCHandle;
	FDelegateHandle BenchPostGCHandle;

	// 측정 끝: 알림 해제, 결과 로그, -BenchQuit이면 게임 종료
	void FinishBench();

	void Move_Input(const FInputActionValue& InputValue);
	void Look_Input(const FInputActionValue& InputValue);
	void WalkStarted_Input();
	void WalkCompleted_Input();
	void Swap_Input();

	// 능력 키 → 입력 태그 → 입력 버퍼 (지금 되면 바로, 공격 중이라 안 되면 잠깐 들고 있다가 ExecuteAbilityInput)
	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);

	// 지금 나와 있는 캐릭터의 ASC (PlayerState가 아직 없으면 null)
	USAbilitySystemComponent* GetSAbilitySystemComponent() const;

	// 공격·경직·사망 중에는 이동 입력을 받지 않음 (멈추게 하지 않고 입력만 막아 자연 감속)
	bool IsMoveInputBlocked() const;

	// HUD를 (처음 한 번만) 만들고 지금 캐릭터에 연결
	void SetupHUD();

	// 현재 시점 설정을 캐릭터·커서·시야 제한에 적용
	void ApplyCurrentCameraConfig();

	const USCameraConfig* GetCurrentCameraConfig() const;

	FVector GetAimPointUnderCursor() const;
	FVector GetAimPointAtScreenCenter() const;

	// 아무것도 맞지 않았을 때 쓰는 위치 (캐릭터 앞)
	FVector GetFallbackAimPoint() const;

	// 한 번만 만들고, 다시 빙의해도 연결만 바꿈 (1차: 교체·빙의마다 위젯이 쌓였음)
	UPROPERTY()
	TObjectPtr<USTwinHUDWidget> HUDWidget;

	// 입력 버퍼: 공격의 끊으면 안 되는 구간에 누른 회피·교체·스킬을 잠깐 들고 있다가 창이 열리면 실행
	UPROPERTY(VisibleAnywhere, Category = "Input")
	TObjectPtr<USInputBufferComponent> InputBuffer;

	// 캐릭터를 잡은 직후: 버퍼를 지금 캐릭터 ASC에 연결하고 교체 알림 구독
	void SetupInputBuffer();

	// 교체되면 버퍼를 비우고(전 캐릭터로 누른 입력) 새 캐릭터 ASC에 다시 연결
	void OnActiveSlotChanged(ETwinSlot NewSlot);

	FDelegateHandle ActiveSlotChangedHandle;

	// 교체 알림을 구독한 캐릭터 (다른 캐릭터에 다시 빙의하면 이전 구독을 끊고 새로 구독)
	TWeakObjectPtr<APlayerCharacter> InputBufferCharacter;

	int32 CurrentCameraIndex = 0;

	bool bDrawAimPoint = false;
};
