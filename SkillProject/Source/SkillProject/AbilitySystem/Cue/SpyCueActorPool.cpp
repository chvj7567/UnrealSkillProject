// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Cue/SpyCueActorPool.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

void FSpyCueActorPool::Initialize(UWorld* InWorld)
{
    World = InWorld;
}

AActor* FSpyCueActorPool::RentCueActor(TSubclassOf<AActor> ActorClass, FGameplayTag GameplayCueTag, const FTransform& SpawnTransform, int32 MaxPoolSize)
{
    if (ActorClass == nullptr || World == nullptr)
        return nullptr;

    UE_LOG(LogTemp, Warning, TEXT("USpyCueActorPool: Use Pool %s Class"), *ActorClass->GetName());

    FPoolEntry& Entry = Pools.FindOrAdd(ActorClass);
    Entry.MaxSize = FMath::Max(1, MaxPoolSize);

    if (Entry.Available.Num() > 0)
    {
        //# 약한 포인터이기에 외부에서 파괴될 가능성 있음
        //# nullptr이면 무시
        TWeakObjectPtr<AActor> WeakA = Entry.Available.Pop();
        if (AActor* Actor = WeakA.Get())
        {
            Entry.InUse.Add(Actor);
            ActorToClass.Add(Actor, ActorClass.Get());
            ActivateActorFromPool(Actor, SpawnTransform);

            return Actor;
        }
    }
    else
    {
        //# 풀의 MaxSize보다 적으면 만듦
        int32 CurrentCount = Entry.InUse.Num() + Entry.Available.Num();
        if (CurrentCount < Entry.MaxSize)
        {
            FActorSpawnParameters SpawnParams;

            //# 스폰 위치에 충돌 여부 상관 없이 스폰하도록 설정
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

            if (AActor* NewActor = World->SpawnActor<AActor>(ActorClass, SpawnTransform, SpawnParams))
            {
                Entry.Tag = GameplayCueTag;
                Entry.InUse.Add(NewActor);
                ActorToClass.Add(NewActor, ActorClass.Get());

                return NewActor;
            }
        }
    }

    //# 풀 MaxSize 오버 로그
    UE_LOG(LogTemp, Warning, TEXT("USpyCueActorPool: Pool full for class %s"), *ActorClass->GetName());

    return nullptr;
}

void FSpyCueActorPool::ReturnCueActor(AActor* Actor)
{
    if (Actor == nullptr)
        return;

    TWeakObjectPtr<AActor> WeakActor = Actor;

    UClass** ClassPtr = ActorToClass.Find(Actor);
    if (ClassPtr == nullptr)
    {
        //# 풀 대상 액터가 아님
        Actor->Destroy();
        return;
    }

    UClass* ActorClass = *ClassPtr;
    FPoolEntry* Entry = Pools.Find(ActorClass);
    if (Entry == nullptr)
    {
        //# 풀 대상 액터였지만 이후에 풀 대상에서 빠진 상태
        Actor->Destroy();
        ActorToClass.Remove(Actor);
        return;
    }

    if (Entry->InUse.Remove(Actor) > 0)
    {
        Entry->Available.Add(Actor);
        DeactivateActorForPool(Actor);
        Entry->LastUsedTime = FPlatformTime::Seconds();
    }
}

void FSpyCueActorPool::DeactivateActorForPool(AActor* Actor)
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

void FSpyCueActorPool::ActivateActorFromPool(AActor* Actor, const FTransform& SpawnTransform)
{
    if (Actor == nullptr)
        return;

    Actor->SetActorTransform(SpawnTransform);
    Actor->SetActorHiddenInGame(false);
    Actor->SetActorEnableCollision(true);
    Actor->SetActorTickEnabled(true);

    //# 사용자 초기화
    // IPoolableGameplayCueActor* Poolable = Cast<IPoolableGameplayCueActor>(Actor);
    // if (Poolable) Poolable->OnAcquiredFromPool();
}

void FSpyCueActorPool::Tick(float DeltaSeconds)
{
    double Now = FPlatformTime::Seconds();

    for (auto& Pair : Pools)
    {
        FPoolEntry& Entry = Pair.Value;
        if ((Now - Entry.LastUsedTime) < UsedIntervalSeconds)
            continue;

        //# 마지막으로 사용한 지 오래된 액터 확인
        TArray<TWeakObjectPtr<AActor>> Keep;
        for (TWeakObjectPtr<AActor>& WeakA : Entry.Available)
        {
            AActor* Actor = WeakA.Get();
            if (Actor == nullptr)
                continue;

            Keep.Add(Actor);
        }

        //# 오래된 액터의 경우 사이즈 절반으로 유지
        int32 MaxKeep = FMath::Max(0, Entry.MaxSize / 2);
        if (Keep.Num() > MaxKeep)
        {
            int32 NumToRemove = Keep.Num() - MaxKeep;
            for (int32 i = 0; i < NumToRemove; ++i)
            {
                TWeakObjectPtr<AActor> WeakA = Entry.Available.Pop();
                if (AActor* Actor = WeakA.Get())
                {
                    Actor->Destroy();
                }
            }
        }

        Entry.LastUsedTime = Now;
    }
}

void FSpyCueActorPool::Clear()
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
    ActorToClass.Empty();
}
