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
	void ConsumeInputForHandle(const FGameplayAbilitySpecHandle& Handle);

public:
	bool HasAbilityByTag(const FGameplayTag& AbilityTag) const;
	bool IsActiveAbilityByTag(const FGameplayTag& AbilityTag) const;

protected:
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;

private:
	void ForEachAbilitySpec(const TArray<FGameplayAbilitySpecHandle>& Handles, TFunctionRef<void(FGameplayAbilitySpec&)> Func);
};
