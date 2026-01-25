// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyGameInstance.h"
#include "System/SpyPlayerController.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Util/SpyGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGameInstance)

USpyGameInstance::USpyGameInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

ASpyPlayerController* USpyGameInstance::GetPrimaryPlayerController() const
{
	return Cast<ASpyPlayerController>(Super::GetPrimaryPlayerController(false));
}

void USpyGameInstance::Init()
{
	Super::Init();

	UGameFrameworkComponentManager* Manager = GetSubsystem<UGameFrameworkComponentManager>(this);
	if (ensure(Manager))
	{
		Manager->RegisterInitState(SpyGameplayTags::InitState_Spawned, false, FGameplayTag());
		Manager->RegisterInitState(SpyGameplayTags::InitState_DataAvailable, false, SpyGameplayTags::InitState_Spawned);
		Manager->RegisterInitState(SpyGameplayTags::InitState_DataInitialized, false, SpyGameplayTags::InitState_DataAvailable);
		Manager->RegisterInitState(SpyGameplayTags::InitState_GameplayReady, false, SpyGameplayTags::InitState_DataInitialized);
	}
}

void USpyGameInstance::Shutdown()
{
	Super::Shutdown();
}