// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueManager.h"
#include "Cue/SKCueActorPool.h"

#include "SKCueManager.generated.h"

UCLASS()
class SKGAS_API USKCueManager : public UGameplayCueManager
{
	GENERATED_BODY()
	
public:
	USKCueManager();

	static USKCueManager* Get();

public:
	virtual void HandleGameplayCue(AActor* TargetActor, FGameplayTag GameplayCueTag,
		EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters,
		EGameplayCueExecutionOptions Options) override;
};
