#include "Pool/SActorPool.h"

#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Pool/SPoolable.h"

namespace
{
	// 0이면 다시 쓰지 않고 매번 생성·삭제 (풀 전후 측정용). 실행 인자 -BenchNoPool이 0으로 바꿈
	TAutoConsoleVariable<int32> CVarPoolEnabled(
		TEXT("ts.Pool.Enabled"),
		1,
		TEXT("1: 오브젝트 풀 사용 (액터 풀 + 나이아가라 이펙트 풀), 0: 매번 생성·삭제"));
}

bool USActorPool::IsPoolingEnabled()
{
	return CVarPoolEnabled.GetValueOnGameThread() != 0;
}

AActor* USActorPool::AcquireActor(
	TSubclassOf<AActor> Class,
	const FTransform& Transform,
	AActor* Owner,
	APawn* InInstigator)
{
	if (!Class)
	{
		return nullptr;
	}
	FSActorFreeList& List = FreeLists.FindOrAdd(Class);

	// 1. 쉬고 있는 게 있으면 꺼내서 씀 (이미 지워진 것은 건너뜀)
	while (List.Items.Num() > 0)
	{
		AActor* Actor = List.Items.Pop();
		if (!IsValid(Actor))
		{
			continue;
		}

		// 지난번 끝난 자리 → 이번 자리로 순간이동, 주인도 이번 사람으로
		Actor->SetActorLocationAndRotation(
			Transform.GetLocation(),
			Transform.GetRotation(),
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
		// 크기도 이번 요청대로 (처음 생성할 때는 Transform 전체를 쓰므로, 재사용도 같게)
		Actor->SetActorScale3D(Transform.GetScale3D());
		Actor->SetOwner(Owner);
		Actor->SetInstigator(InInstigator);
		if (ISPoolable* Poolable = Cast<ISPoolable>(Actor))
		{
			Poolable->OnAcquiredFromPool();
		}
		return Actor;
	}

	// 2. 없으면 새로 만듦 (풀이 관리한다고 표시한 뒤 생성 마무리 → BeginPlay가 표시를 봄)
	AActor* Actor = GetWorld()->SpawnActorDeferred<AActor>(
		Class,
		Transform,
		Owner,
		InInstigator,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Actor)
	{
		return nullptr;
	}
	ISPoolable* Poolable = Cast<ISPoolable>(Actor);
	if (Poolable)
	{
		Poolable->OnCreatedByPool();
	}
	Actor->FinishSpawning(Transform);
	if (Poolable)
	{
		Poolable->OnAcquiredFromPool();
	}
	return Actor;
}

void USActorPool::Release(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	// 넣을 수 없는 액터이거나 풀이 꺼져 있으면 그냥 지움
	ISPoolable* Poolable = Cast<ISPoolable>(Actor);
	if (!Poolable || !IsPoolingEnabled())
	{
		Actor->Destroy();
		return;
	}

	// 끄고 지난번 상태 비우기 (숨김, 충돌·이동·이펙트 끔 등은 액터가 정함)
	Poolable->OnReleasedToPool();

	// 자리가 있으면 스택에 쌓고, 꽉 찼으면 그냥 지움 (풀이 끝없이 커지지 않게)
	FSActorFreeList& List = FreeLists.FindOrAdd(Actor->GetClass());
	if (List.Items.Num() < MaxFreePerClass)
	{
		List.Items.Push(Actor);
	}
	else
	{
		Actor->Destroy();
	}
}
