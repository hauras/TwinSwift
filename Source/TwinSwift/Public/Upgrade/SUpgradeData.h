
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SUpgradeData.generated.h"

UENUM(BlueprintType)
enum class EUpgradeTarget : uint8
{
	//둘다
	Both,
	Warrior,
	Mage
};

class UTexture2D;
class UGameplayEffect;
/**
 * 
 */
UCLASS()
class TWINSWIFT_API USUpgradeData : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Upgrade")
	FText DisplayName;

	UPROPERTY(EditAnywhere, Category = "Upgrade", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditAnywhere, Category = "Upgrade")
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, Category = "Upgrade")
	EUpgradeTarget Target = EUpgradeTarget::Both;;

	UPROPERTY(EditAnywhere, Category = "Upgrade")
	TSubclassOf<UGameplayEffect> Effect;
	
};
