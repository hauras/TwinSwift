#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SPoolable.generated.h"

UINTERFACE(MinimalAPI)
class USPoolable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 오브젝트 풀(USActorPool)에 넣을 수 있는 액터
 * - 풀은 "언제 꺼내고 언제 넣는지"만 알고, "꺼낼 때·넣을 때 무엇을 켜고 끌지"는 액터가 정함
 * - 구현: 투사체(ASProjectile), 데미지 숫자(ADamageTextActor)
 */
class TWINSWIFT_API ISPoolable
{
	GENERATED_BODY()

public:
	// 풀이 새로 만들 때 생성 마무리(BeginPlay) 전에 한 번. "풀이 관리함" 표시용
	virtual void OnCreatedByPool() {}

	// 풀에서 꺼낼 때 (새로 만든 것·다시 쓰는 것 모두). 위치·주인은 풀이 이미 맞춰 둠
	virtual void OnAcquiredFromPool() {}

	// 풀에 넣을 때: 숨기고 끄고, 지난번 상태를 비움 (다음에 꺼낼 때 물려받지 않게)
	virtual void OnReleasedToPool() = 0;
};
