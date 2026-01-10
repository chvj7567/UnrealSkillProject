// Fill out your copyright notice in the Description page of Project Settings.


#include "Cue/SKCueActorPool.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

void FSKCueActorPool::Initialize(UWorld* InWorld)
{
    World = InWorld;
}

AActor* FSKCueActorPool::RentCueActor(TSubclassOf<AActor> ActorClass, FGameplayTag GameplayCueTag, AActor* TargetActor)
{
    if (ActorClass == nullptr || World == nullptr)
        return nullptr;

    AActor* RentActor = nullptr;
    FPoolEntry& Entry = Pools.FindOrAdd(ActorClass);

    if (Entry.Available.Num() > 0)
    {
        //# 약한 포인터이기에 외부에서 파괴될 가능성 있음
        //# nullptr이면 RentCueActor 재호출
        TWeakObjectPtr<AActor> WeakA = Entry.Available.Pop();
        if (AActor* Actor = WeakA.Get())
        {
            Entry.InUse.Add(Actor);
            RentActor = Actor;
        }
        else
        {
            RentActor = RentCueActor(ActorClass, GameplayCueTag, TargetActor);
        }
    }
    else
    {
        FActorSpawnParameters SpawnParams;

        //# 스폰 위치에 충돌 여부 상관 없이 스폰하도록 설정
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        if (AActor* NewActor = World->SpawnActor<AActor>(ActorClass, TargetActor->GetActorTransform(), SpawnParams))
        {
            Entry.Tag = GameplayCueTag;
            Entry.InUse.Add(NewActor);
            RentActor = NewActor;
        }
    }

    ActivateActorFromPool(RentActor, TargetActor);

    return RentActor;
}

void FSKCueActorPool::ReturnCueActor(AActor* Actor)
{
    if (Actor == nullptr)
        return;

    FPoolEntry* Entry = Pools.Find(Actor->GetClass());
    if (Entry != nullptr && Entry->InUse.Remove(Actor) > 0)
    {
        Entry->Available.Add(Actor);
        DeactivateActorForPool(Actor);
        Entry->LastUsedTime = FPlatformTime::Seconds();
    }
    else
    {
        //# 풀 대상 액터가 아님
        Actor->Destroy();
    }
}

void FSKCueActorPool::ReturnCueActor(FGameplayTag Tag)
{
    for (auto& Pool : Pools)
    {
        if (Pool.Value.Tag == Tag)
        {
            int Index = 0;
            for (auto& UseActor : Pool.Value.InUse)
            {
                if (AActor* PoolActor = Pool.Value.InUse[Index].Get())
                {
                    ReturnCueActor(PoolActor);
                    return;
                }

                ++Index;
            }
        }
    }
}

void FSKCueActorPool::DeactivateActorForPool(AActor* Actor)
{
    if (Actor == nullptr)
        return;

    Actor->SetActorHiddenInGame(true);
    Actor->SetActorEnableCollision(false);
    Actor->SetActorTickEnabled(false);

    //# 사용자 초기화
    // IPoolableGameplayCueActor* Poolable = Cast<IPoolableGameplayCueActor>(Actor);
    // if (Poolable) Poolable->OnReturnedToPool();
}

void FSKCueActorPool::ActivateActorFromPool(AActor* Actor, AActor* TargetActor)
{
    if (Actor == nullptr)
        return;

    Actor->AttachToComponent(TargetActor->GetRootComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
    //Actor->SetActorTransform(FTransform());
    Actor->SetActorHiddenInGame(false);
    Actor->SetActorEnableCollision(true);
    Actor->SetActorTickEnabled(true);

    //# 사용자 초기화
    // IPoolableGameplayCueActor* Poolable = Cast<IPoolableGameplayCueActor>(Actor);
    // if (Poolable) Poolable->OnAcquiredFromPool();
}

void FSKCueActorPool::Clear()
{
    for (auto& Pair : Pools)
    {
        FPoolEntry& Entry = Pair.Value;

        for (TWeakObjectPtr<AActor>& WeakA : Entry.Available)
        {
            if (AActor* PoolA = WeakA.Get())
            {
                PoolA->Destroy();
            }
        }

        Entry.Available.Empty();

        for (TWeakObjectPtr<AActor> WeakA : Entry.InUse)
        {
            if (AActor* PoolA = WeakA.Get())
            {
                PoolA->Destroy();
            }
        }

        Entry.InUse.Empty();
    }

    Pools.Empty();
}
