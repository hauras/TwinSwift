
#pragma once

#include "CoreMinimal.h"
#include "Character/TwinSlot.h"
#include "Components/ActorComponent.h"
#include "SActionRecorderComponent.generated.h"

class UGameplayAbility;

//위치 기록
struct FSRecordedPose
{
	double Time = 0.0;
	FVector Location = FVector::ZeroVector;
	float Yaw = 0.f;
};

//행동 종류
enum class ERecordedActionType : uint8
{
	Ability,
	Swap,
};

struct FSRecordedAction
{
	double Time;
	ERecordedActionType Type = ERecordedActionType::Ability;
	TSubclassOf<UGameplayAbility> AbilityClass;
	ETwinSlot Slot = ETwinSlot::Warrior;
};

// 원형 버퍼: 칸 수가 고정된 배열에 차례로 쓰고, 가득 차면 가장 오래된 칸부터 덮어씀 (메모리가 늘지 않음, 쓰기 O(1))
// T = 칸에 넣을 타입 (위치 기록·행동 기록이 같은 동작을 타입만 바꿔 씀). 템플릿이라 함수 내용도 헤더에
template <typename T>
struct TSRingBuffer
{
	// 칸 수를 정하고 비움 (쓰기 전에 한 번)
	void Init(int32 InCapacity)
	{
		Items.SetNum(FMath::Max(InCapacity, 1));
		Head = 0;
		Count = 0;
	}

	// Head 칸에 쓰고 한 칸 앞으로 (끝이면 0번으로). 가득 찼으면 방금 덮어쓴 칸이 가장 오래된 기록이었음
	void Add(const T& Item)
	{
		// Init 전이면 칸이 없어 % 0이 됨
		if (Items.IsEmpty()) return;

		Items[Head] = Item;
		Head = (Head + 1) % Items.Num();
		Count = FMath::Min(Count + 1, Items.Num());
	}

	// 들어 있는 개수 (칸 수를 넘지 않음)
	int32 Num() const
	{
		return Count;
	}

	// Index 0 = 가장 오래된 기록, Num() - 1 = 가장 최근 기록. 범위 확인은 부르는 쪽에서 (0 ~ Num() - 1)
	const T& Get(int32 Index) const
	{
		// 가장 오래된 칸: 아직 안 찼으면 0번부터 차례로 썼으니 0번, 가득 찼으면 다음에 덮어쓸 Head 칸
		const int32 Oldest = Count < Items.Num() ? 0 : Head;
		return Items[(Oldest + Index) % Items.Num()];
	}

private:
	TArray<T> Items;

	// 다음에 쓸 칸
	int32 Head = 0;

	// 들어 있는 개수
	int32 Count = 0;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TWINSWIFT_API USActionRecorderComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USActionRecorderComponent();

	bool GetPoseAt(double Time, FSRecordedPose& OutPose) const;

	void GetActionsBetween(double FromTime, double ToTime, TArray<FSRecordedAction>& OutActions) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditDefaultsOnly, Category = "Mirror", meta = (ClampMin = "0.02"))
	float SampleInterval = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Mirror", meta = (ClampMin = "1"))
	float HistorySeconds = 5.f;

private:
	void OnAbilityActivated(UGameplayAbility* Ability);
	void OnActiveSlotChanged(ETwinSlot NewSlot);

	TSRingBuffer<FSRecordedPose> Poses;
	TSRingBuffer<FSRecordedAction> Actions;

	FDelegateHandle WarriorAbilityHandle;
	FDelegateHandle MageAbilityHandle;
	FDelegateHandle SwapHandle;
};
