// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/SpyCueManager.h"
#include "GameplayTagsManager.h"
#include "AbilitySystem/SpyAbilitySystemGlobals.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "UObject/UObjectThreadContext.h"
#include "Misc/CoreDelegates.h"
#include "Manager/SpyAssetManager.h"
#include "Util/SpyGameplayTags.h"

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
#if !UE_BUILD_SHIPPING
    UE_LOG(LogTemp, Verbose,
        TEXT("[SpyCueManager] %s (%s)"),
        /**UEnum::GetValueAsString(TargetActor->GetNetMode()),*/
        *GameplayCueTag.ToString(),
        *UEnum::GetValueAsString(EventType));
#endif

	Super::HandleGameplayCue(TargetActor, GameplayCueTag, EventType, Parameters, Options);

}
