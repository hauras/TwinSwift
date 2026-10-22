
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_ActionWindow.generated.h"

class UAbilitySystemComponent;
/**
 * 
 */
UCLASS()
class TWINSWIFT_API UANS_ActionWindow : public UAnimNotifyState
{
	GENERATED_BODY()
public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Window", meta = (Categories = "Window"))
	FGameplayTagContainer WindowTags;

private:
	static UAbilitySystemComponent* FindOwnerASC(USkeletalMeshComponent* MeshComp);
};
