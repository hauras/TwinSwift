#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_WeaponTrace.generated.h"

class UStaticMeshComponent;

/**
 * 칼이 지나가는 구간(몽타주에 구간으로 배치) 동안 매 프레임 칼날을 따라 구를 쓸어 적을 찾음
 * 찾은 적마다 Event.Montage.Hit 이벤트를 캐릭터에 보냄 → 능력(GA_AttackBase)이 데미지 적용
 * 한 번 휘두르는 동안 같은 적은 한 번만 맞음
 * 칼 궤적은 이 노티파이가 아니라 엔진 노티파이 Timed Niagara Effect가 담당 (SwordTrailVFX 데모와 같은 방식)
 */
UCLASS(meta = (DisplayName = "Weapon Trace"))
class TWINSWIFT_API UANS_WeaponTrace : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyTick(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float FrameDeltaTime,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

protected:
	// 칼날 두께 (구 반지름)
	UPROPERTY(EditAnywhere, Category = "Trace")
	float TraceRadius = 30.f;

	// 칼날 시작·끝: 무기 메시의 소켓 두 개. 판정은 이 사이를 씀
	// 에디터에서 칼 메시의 소켓을 끌면 판정 범위가 같이 바뀜
	UPROPERTY(EditAnywhere, Category = "Blade")
	FName BladeStartSocket = TEXT("TrailStart");

	UPROPERTY(EditAnywhere, Category = "Blade")
	FName BladeEndSocket = TEXT("TrailEnd");

	// 소켓이 없을 때만 씀: 무기 길이 중 이 비율부터 칼날로 봄 (대검은 가드가 약 30% 지점)
	UPROPERTY(EditAnywhere, Category = "Blade", meta = (ClampMin = "0", ClampMax = "1"))
	float BladeStartRatio = 0.3f;

	// [디버그] 판정 구를 화면에 그림
	UPROPERTY(EditAnywhere, Category = "Trace")
	bool bDrawDebug = false;

private:
	// 칼날 시작·끝 (무기 메시 기준 좌표). 소켓이 있으면 소켓, 없으면 BladeStartRatio로 계산
	void GetBladeLocal(const UStaticMeshComponent* Weapon, FVector& OutStart, FVector& OutEnd) const;

	// 노티파이 객체는 같은 몽타주를 쓰는 모든 캐릭터가 공유 → 이미 맞은 적은 메시별로 따로 보관
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, TSet<TWeakObjectPtr<AActor>>> HitActorsByMesh;
};
