
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "SInputBufferComponent.generated.h"

class UAbilitySystemComponent;
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TWINSWIFT_API USInputBufferComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	void HandleInput(const FGameplayTag& InputTag);

	void Clear();
	void BindToASC(UAbilitySystemComponent* ASC);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Input Buffer")
	float BufferDuration = 0.3f;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	FGameplayTag GetRequiredWindow(const FGameplayTag& InputTag) const;

	bool IsAllowedNow(const FGameplayTag& InputTag) const;
	void TryFlush();

	// 실제 실행: 주인 컨트롤러의 ExecuteAbilityInput (HandleInput·TryFlush가 같이 씀)
	void Execute(const FGameplayTag& InputTag);
	void OnTagChanged(const FGameplayTag Tag, int32 NewCount);

	void UnbindASC();
	FGameplayTag BufferedInput;
	double BufferedTime = 0.0;

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	TArray<TPair<FGameplayTag, FDelegateHandle>> TagHandles;
	
		
};
