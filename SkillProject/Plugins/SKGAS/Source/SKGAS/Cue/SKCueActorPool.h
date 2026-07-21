// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SKGameplayTags.h"

struct FPoolEntry
{
    FGameplayTag Tag;
    TArray<TWeakObjectPtr<AActor>> Available;
    TArray<TWeakObjectPtr<AActor>> InUse;
    double LastUsedTime = 0.0;
};

class FSKCueActorPool
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
    //# 일반 C++ 클래스(UCLASS 아님)라 UPROPERTY 가 무효하므로 붙이지 않는다.
    //# 풀은 액터를 소유하지 않고 약참조로만 추적한다 (FPoolEntry 도 TWeakObjectPtr).
    //# TODO: 실제로 배선할 때 Pools 키를 TObjectKey<UClass> 로 하드닝할지 재검토.
    TWeakObjectPtr<UWorld> World;

    TMap<UClass*, FPoolEntry> Pools;

    float UsedIntervalSeconds = 60.f;
};
