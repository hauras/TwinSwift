#include "Editor/TSBossAIEditLibrary.h"

#if WITH_EDITOR
#include "AI/BT/BTService_FindNearestPlayer.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "Builders/CubeBuilder.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "UObject/UnrealType.h"
#endif

FString UTSBossAIEditLibrary::ConfigureTree(UBehaviorTree* Tree, UBlackboardData* Blackboard)
{
#if WITH_EDITOR
	if (!Tree || !Blackboard) return TEXT("ERROR: Missing asset");
	if (Tree->RootNode) return TEXT("ERROR: Tree already has nodes; preserving existing graph");
	Blackboard->Modify();
	for (const FName Name : { FName(TEXT("TargetActor")), FName(TEXT("DistanceToTarget")) })
	{
		if (Blackboard->Keys.ContainsByPredicate([Name](const FBlackboardEntry& Entry) { return Entry.EntryName == Name; })) continue;
		FBlackboardEntry Entry;
		Entry.EntryName = Name;
		if (Name == TEXT("TargetActor"))
		{
			auto* Key = NewObject<UBlackboardKeyType_Object>(Blackboard, NAME_None, RF_Transactional);
			Key->BaseClass = AActor::StaticClass();
			Entry.KeyType = Key;
		}
		else Entry.KeyType = NewObject<UBlackboardKeyType_Float>(Blackboard, NAME_None, RF_Transactional);
		Blackboard->Keys.Add(Entry);
	}
	Tree->Modify();
	Tree->BlackboardAsset = Blackboard;
	auto* Root = NewObject<UBTComposite_Selector>(Tree, TEXT("BossSelector"), RF_Transactional);
	auto* Service = NewObject<UBTService_FindNearestPlayer>(Tree, TEXT("FindNearestPlayer"), RF_Transactional);
	auto* Move = NewObject<UBTTask_MoveTo>(Tree, TEXT("MoveToPlayer"), RF_Transactional);
	auto SetKey = [Blackboard](UObject* Object, FName PropertyName, FName KeyName)
	{
		FStructProperty* Property = FindFProperty<FStructProperty>(Object->GetClass(), PropertyName);
		if (!Property || Property->Struct != FBlackboardKeySelector::StaticStruct()) return false;
		auto* Key = Property->ContainerPtrToValuePtr<FBlackboardKeySelector>(Object);
		Key->SelectedKeyName = KeyName;
		Key->ResolveSelectedKey(*Blackboard);
		return true;
	};
	if (!SetKey(Service, TEXT("TargetToSelector"), TEXT("TargetActor")) ||
		!SetKey(Service, TEXT("DistanceToTargetSelector"), TEXT("DistanceToTarget")) ||
		!SetKey(Move, TEXT("BlackboardKey"), TEXT("TargetActor"))) return TEXT("ERROR: Key property not found");
	FStructProperty* Radius = FindFProperty<FStructProperty>(Move->GetClass(), TEXT("AcceptableRadius"));
	if (!Radius) return TEXT("ERROR: MoveTo radius property not found");
	*Radius->ContainerPtrToValuePtr<FValueOrBBKey_Float>(Move) = FValueOrBBKey_Float(250.f);
	Root->Services.Add(Service);
	FBTCompositeChild Child;
	Child.ChildTask = Move;
	Root->Children.Add(Child);
	Tree->RootNode = Root;
	Blackboard->PostEditChange();
	Blackboard->MarkPackageDirty();
	Tree->MarkPackageDirty();
	return TEXT("OK: BB keys, Selector, Find Nearest Player, MoveTo radius 250. Open asset editor to generate its graph before saving.");
#else
	return TEXT("ERROR: Editor only");
#endif
}

bool UTSBossAIEditLibrary::BuildNavBox(ANavMeshBoundsVolume* Volume, FVector FullSize)
{
#if WITH_EDITOR
	if (!Volume || FullSize.GetMin() <= 0.f) return false;
	Volume->Modify();
	auto* Builder = NewObject<UCubeBuilder>(Volume, NAME_None, RF_Transactional);
	Builder->X = FullSize.X;
	Builder->Y = FullSize.Y;
	Builder->Z = FullSize.Z;
	if (!Builder->Build(Volume->GetWorld(), Volume)) return false;
	Volume->PostEditChange();
	Volume->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}
