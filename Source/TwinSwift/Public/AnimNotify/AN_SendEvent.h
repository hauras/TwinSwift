#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_SendEvent.generated.h"

/**
 * 몽타주의 이 시점에 주인 액터(ASC)로 게임플레이 이벤트를 보냄
 * - 착지·발사 순간처럼 "이 프레임에 무언가 해라"를 능력에 알릴 때 (능력은 WaitGameplayEvent로 받음)
 * - 재생 속도를 바꿔도 애니메이션 프레임에 맞춰 옴 (타이머로 시간을 맞출 필요 없음)
 */
UCLASS(meta = (DisplayName = "Send Gameplay Event"))
class TWINSWIFT_API UAN_SendEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	// 몽타주 타임라인에 태그 이름이 보이게
	virtual FString GetNotifyName_Implementation() const override;

protected:
	// 보낼 신호 (예: Event.Montage.Hit). 이 신호를 기다리는 능력이 받음
	UPROPERTY(EditAnywhere, Category = "Event", meta = (Categories = "Event"))
	FGameplayTag EventTag;
};
