// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Util/SpyGameplayTags.h"

struct FPoolEntry
{
    FGameplayTag Tag;
    TArray<TWeakObjectPtr<AActor>> Available;
    TArray<TWeakObjectPtr<AActor>> InUse;
    double LastUsedTime = 0.0;
};

class FSpyCueActorPool
{
public:
    void Initialize(UWorld* InWorld);
    AActor* RentCueActor(TSubclassOf<AActor> ActorClass, FGameplayTag GameplayCueTag, AActor* TargetActor);
    void ReturnCueActor(AActor* Actor);
    void ReturnCueActor(FGameplayTag Tag);
    void Clear();

protected:
    void DeactivateActorForPool(AActor* Actor);
    void ActivateActorFromPool(AActor* Actor, AActor* TargetActor);

protected:
    UWorld* World = nullptr;
    
    TMap<UClass*, FPoolEntry> Pools;

    float UsedIntervalSeconds = 60.f;
};
