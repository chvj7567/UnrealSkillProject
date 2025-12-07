// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Util/SpyGameplayTags.h"

struct FPoolEntry
{
    TArray<TWeakObjectPtr<AActor>> Available;
    TSet<TWeakObjectPtr<AActor>> InUse;
    FGameplayTag Tag;
    int32 MaxSize = 16;
    double LastUsedTime = 0.0;
};

class FSpyCueActorPool
{
public:
    void Initialize(UWorld* InWorld);
    AActor* RentCueActor(TSubclassOf<AActor> ActorClass, FGameplayTag GameplayCueTag, const FTransform& SpawnTransform, int32 MaxPoolSize = 16);
    void ReturnCueActor(AActor* Actor);
    void Clear();

protected:
    void DeactivateActorForPool(AActor* Actor);
    void ActivateActorFromPool(AActor* Actor, const FTransform& SpawnTransform);
    void Tick(float DeltaSeconds);

protected:
    UWorld* World = nullptr;
    
    TMap<UClass*, FPoolEntry> Pools;

    TMap<TWeakObjectPtr<AActor>, UClass*> ActorToClass;

    float UsedIntervalSeconds = 60.f;
};
