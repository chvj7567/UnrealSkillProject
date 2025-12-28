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
	virtual void HandleGameplayCue(AActor* TargetActor, FGameplayTag GameplayCueTag,
		EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters,
		EGameplayCueExecutionOptions Options) override;
};
