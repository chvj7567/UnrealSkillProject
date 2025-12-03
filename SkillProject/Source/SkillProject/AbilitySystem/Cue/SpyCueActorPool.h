// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SpyCueActorPool.generated.h"

struct FPoolEntry
{
    TArray<TWeakObjectPtr<AActor>> Available;
    TSet<TWeakObjectPtr<AActor>> InUse;
    int32 MaxSize = 16;
    double LastUsedTime = 0.0;
};

UCLASS()
class SKILLPROJECT_API USpyCueActorPool : public UObject
{
	GENERATED_BODY()
	
public:
    void Initialize(UWorld* InWorld);
    AActor* RentCueActor(TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform, int32 MaxPoolSize = 16);
    void ReturnCueActor(AActor* Actor);

protected:
    void DeactivateActorForPool(AActor* Actor);
    void ActivateActorFromPool(AActor* Actor, const FTransform& SpawnTransform);
    void Tick(float DeltaSeconds);
    void FlushAll();

protected:
    UWorld* World = nullptr;
    
    TMap<UClass*, FPoolEntry> Pools;

    TMap<TWeakObjectPtr<AActor>, UClass*> ActorToClass;

    float UsedIntervalSeconds = 60.f;
};
