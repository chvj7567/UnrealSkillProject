// Fill out your copyright notice in the Description page of Project Settings.


#include "Cue/SKCueManager.h"
#include "SKAbilitySystemGlobals.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKCueManager)

USKCueManager::USKCueManager()
{
	UE_LOG(LogTemp, Warning, TEXT("USKCueManager Create"));
}

USKCueManager* USKCueManager::Get()
{
	return Cast<USKCueManager>(USKAbilitySystemGlobals::Get().GetGameplayCueManager());
}

void USKCueManager::HandleGameplayCue(AActor* TargetActor, FGameplayTag GameplayCueTag, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters, EGameplayCueExecutionOptions Options)
{
	Super::HandleGameplayCue(TargetActor, GameplayCueTag, EventType, Parameters, Options);

}
