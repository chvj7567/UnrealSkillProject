// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"

#include "SKAbilitySystemComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FAbilityChangedDelegate, FGameplayAbilitySpecHandle, bool/*bGiven*/);

UCLASS()
class SKGAS_API USKAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
	
public:
	USKAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

	virtual void OnGiveAbility(FGameplayAbilitySpec& AbilitySpec) override;
	virtual void OnRemoveAbility(FGameplayAbilitySpec& AbilitySpec) override;

	virtual void NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability) override;
	virtual void NotifyAbilityFailed(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason) override;
	virtual void NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, bool bWasCancelled) override;
	virtual void ApplyAbilityBlockAndCancelTags(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bEnableBlockTags, const FGameplayTagContainer& BlockTags, bool bExecuteCancelTags, const FGameplayTagContainer& CancelTags) override;
	virtual void HandleChangeAbilityCanBeCanceled(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bCanBeCanceled) override;

public:
	UFUNCTION(BlueprintCallable)
	FGameplayTag GetSkillActionTag() const;

	UFUNCTION(BlueprintCallable)
	FGameplayTag GetEffectSkillActionTag() const;

public:
	FAbilityChangedDelegate AbilityChangedDelegate;
};
