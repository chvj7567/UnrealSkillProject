// Fill out your copyright notice in the Description page of Project Settings.


#include "Input/SpyEnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyEnhancedInputComponent)

USpyEnhancedInputComponent::USpyEnhancedInputComponent(const FObjectInitializer& ObjectInitializer)
{
}

void USpyEnhancedInputComponent::AddInputMappings(const USpyInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
	check(InputConfig);
	check(InputSubsystem);

	//# TODO
}

void USpyEnhancedInputComponent::RemoveInputMappings(const USpyInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
	check(InputConfig);
	check(InputSubsystem);

	//# TODO
}

void USpyEnhancedInputComponent::RemoveBinds(TArray<uint32>& BindHandles)
{
	for (uint32 Handle : BindHandles)
	{
		RemoveBindingByHandle(Handle);
	}

	BindHandles.Reset();
}
