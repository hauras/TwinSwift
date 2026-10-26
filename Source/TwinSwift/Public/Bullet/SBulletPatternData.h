#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Bullet/SBulletManager.h"
#include "SBulletPatternData.generated.h"

// 탄막 모양 
UENUM(BlueprintType)
enum class EBulletShape : uint8
{
	// 한 바퀴를 탄 수만큼 똑같이 나눔 (16발이면 22.5도 간격)
	Ring,
	// 기준 방향 좌우로 Spread 각도 안에 고르게 (7발·70도면 -35 ~ +35도)
	Fan,
};

// 발사 지시 한 줄: "몇 초에, 어떤 모양으로, 몇 발, 몇 번 반복"
// 나선 = 고리 + 반복마다 각도 돌림, 방사형 엇갈림 = 고리 2번 + 반 칸 돌림 → 모양을 따로 늘리지 않고 값으로 만듦
USTRUCT(BlueprintType)
struct FSBulletPatternEvent
{
	GENERATED_BODY()

	// 패턴 시작부터 몇 초에 첫 발 (몽타주 시간)
	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (ClampMin = "0.0"))
	float FireTime = 0.f;

	// 한 번에 쏘는 탄 수
	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (ClampMin = "1"))
	int32 BulletCount = 16;

	// 모양
	UPROPERTY(EditAnywhere, Category = "Pattern")
	EBulletShape Shape = EBulletShape::Ring;

	// 탄 속도(cm/초)
	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (ClampMin = "1"))
	float Speed = 500.f;

	// 기준 방향(표적 쪽)에 더할 각도(도)
	UPROPERTY(EditAnywhere, Category = "Pattern")
	float AngleOffset = 0.f;

	// 부채꼴 전체 각도(도). 고리는 무시
	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (ClampMin = "0", ClampMax = "360"))
	float Spread = 70.f;

	// 주황은 전사, 파랑은 마법사가 막음
	UPROPERTY(EditAnywhere, Category = "Pattern")
	EBulletColor Color = EBulletColor::Orange;

	// 켜면 Color 대신 발동마다 주황/파랑 중 하나 (한 발동 안의 무작위 지시는 모두 같은 색). 예고 동안 보스 몸에 그 색이 보임
	UPROPERTY(EditAnywhere, Category = "Pattern")
	bool bRandomColor = false;

	// 몇 번 쏘나
	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (ClampMin = "1"))
	int32 RepeatCount = 1;

	// 반복 간격(초, 몽타주 시간)
	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (ClampMin = "0.01"))
	float RepeatInterval = 0.15f;

	// 반복마다 더 돌릴 각도(도). 나선 12, 방사형 엇갈림 11.25(16발 반 칸)
	UPROPERTY(EditAnywhere, Category = "Pattern")
	float AngleStepPerRepeat = 0.f;

	// 켜면 쏠 때마다 표적을 다시 봄, 끄면 능력 시작 때 방향 그대로 (예고된 방향으로 맞게)
	UPROPERTY(EditAnywhere, Category = "Pattern")
	bool bReaim = false;

	// 1발 피해 = 보스 공격력 × 이 값
	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (ClampMin = "0"))
	float DamageMultiplier = 1.f;

	// 탄 수명(초). 0이면 매니저 기본값 (짧게 하면 가까이에만 닿는 탄)
	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (ClampMin = "0"))
	float LifeTime = 0.f;

	// 맞으면 밀려나는 거리(cm). 0이면 밀지 않음 (색을 못 맞춘 벌)
	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (ClampMin = "0"))
	float KnockbackStrength = 0.f;

	// 켜면 반격 가능: 완벽 회피(교체·회피 직후 0.25초)로 받으면 보스가 큰 경직 (탄이 많은 패턴에 켜면 너무 자주 눕음 → 큰 공격에만)
	UPROPERTY(EditAnywhere, Category = "Pattern")
	bool bCounterable = false;
};

// 지시를 반복까지 펼친 발사 한 번 (게임 중에만 쓰는 값이라 USTRUCT 없음)
struct FSBulletShot
{
	// FireTime + 반복 번호 × RepeatInterval
	float Time = 0.f;

	// 몇 번째 지시인지 (모양·색·속도는 지시에서 읽음)
	int32 EventIndex = 0;

	// 몇 번째 반복인지 (각도 = AngleOffset + 반복 번호 × AngleStepPerRepeat)
	int32 RepeatIndex = 0;
};

/**
 * 탄막 패턴 = 발사 지시 목록 (데이터 에셋: DA_Conductor_*)
 */
UCLASS()
class TWINSWIFT_API USBulletPatternData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 지시를 반복까지 펼쳐 OutShots에 담고 시간순 정렬
	void BuildShots(TArray<FSBulletShot>& OutShots) const;

	// 마지막 발사 시각 (몽타주보다 늦게 쏘는 설정을 능력이 거부할 때)
	float GetLastShotTime() const;

	// 무작위 색 지시가 하나라도 있나 (능력이 예고 색을 띄울지)
	bool HasRandomColor() const;

	// 발사할 때 지시 읽기
	const FSBulletPatternEvent& GetEvent(int32 Index) const { return Events[Index]; }

protected:
	// 발사 지시 목록. 순서 상관없이 적어도 펼칠 때 정렬됨
	UPROPERTY(EditAnywhere, Category = "Pattern")
	TArray<FSBulletPatternEvent> Events;
};
