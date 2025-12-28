// Fill out your copyright notice in the Description page of Project Settings.


#include "Cue/SpyCueManager.h"
#include "SpyAbilitySystemGlobals.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyCueManager)

USpyCueManager::USpyCueManager()
{
	UE_LOG(LogTemp, Warning, TEXT("USpyCueManager Create"));
}

USpyCueManager* USpyCueManager::Get()
{
	return Cast<USpyCueManager>(USpyAbilitySystemGlobals::Get().GetGameplayCueManager());
}

void USpyCueManager::HandleGameplayCue(AActor* TargetActor, FGameplayTag GameplayCueTag, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters, EGameplayCueExecutionOptions Options)
{
	Super::HandleGameplayCue(TargetActor, GameplayCueTag, EventType, Parameters, Options);

}
