// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"

#include "SpyAbilitySystemComponent.generated.h"


DECLARE_MULTICAST_DELEGATE_TwoParams(FAbilityChangedDelegate, FGameplayAbilitySpecHandle, bool/*bGiven*/);

UCLASS()
class SKILLPROJECT_API USpyAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
	
public:
	USpyAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

	virtual void OnGiveAbility(FGameplayAbilitySpec& AbilitySpec) override;
	virtual void OnRemoveAbility(FGameplayAbilitySpec& AbilitySpec) override;

public:
	FAbilityChangedDelegate AbilityChangedDelegate;
};
