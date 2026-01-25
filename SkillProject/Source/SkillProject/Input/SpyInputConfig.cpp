// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyInputConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyInputConfig)

USpyInputConfig::USpyInputConfig(const FObjectInitializer& ObjectInitializer)
{
}

const UInputAction* USpyInputConfig::FindNativeInputActionForTag(const FGameplayTag& InputTag) const
{
	for (const FSpyInputAction& Action : NativeInputActions)
	{
		if (Action.InputAction && (Action.InputTag == InputTag))
		{
			return Action.InputAction;
		}
	}

	return nullptr;
}

const UInputAction* USpyInputConfig::FindAbilityInputActionForTag(const FGameplayTag& InputTag) const
{
	for (const FSpyInputAction& Action : AbilityInputActions)
	{
		if (Action.InputAction && (Action.InputTag == InputTag))
		{
			return Action.InputAction;
		}
	}

	return nullptr;
}
