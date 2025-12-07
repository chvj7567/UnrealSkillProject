// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueManager.h"
#include "AbilitySystem/Cue/SpyCueActorPool.h"

#include "SpyCueManager.generated.h"

UCLASS()
class SKILLPROJECT_API USpyCueManager : public UGameplayCueManager
{
	GENERATED_BODY()
	
public:
	USpyCueManager();

	static USpyCueManager* Get();

public:
	virtual void OnCreated() override;
	virtual void OnEngineInitComplete() override;
	virtual bool ShouldSuppressGameplayCues(AActor* TargetActor) override;
	virtual void HandleGameplayCue(AActor* TargetActor, FGameplayTag GameplayCueTag,
		EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters,
		EGameplayCueExecutionOptions Options) override;

public:
	void OnGameplayTagLoaded(const FGameplayTag& Tag);
	void HandlePostLoadMap(UWorld* LoadedWorld);
	void HandlePostGarbageCollect();
	void ProcessLoadedTags();

private:
	FSpyCueActorPool NotifyCueActorPool;
	TArray<FGameplayTag> LoadedGameplayTagsToProcess;
	FCriticalSection LoadedGameplayTagsToProcessCS;
	TMap<FGameplayTag, TSoftClassPtr<AActor>> TagToCueActorClass;
};
