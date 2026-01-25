// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SKAbilitySystemComponent.h"

#include "SpyAbilitySystemComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FAbilityChangedDelegate, FGameplayAbilitySpecHandle, bool/*bGiven*/);

UCLASS()
class SKILLPROJECT_API USpyAbilitySystemComponent : public USKAbilitySystemComponent
{
	GENERATED_BODY()

public:
	USpyAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);
	void ClearAbilityInput();

protected:
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;
};
