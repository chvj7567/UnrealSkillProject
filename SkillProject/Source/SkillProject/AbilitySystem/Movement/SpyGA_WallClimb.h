// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Ability/SKGameplayAbility.h"

#include "SpyGA_WallClimb.generated.h"

class UInputMappingContext;

UCLASS()
class SKILLPROJECT_API USpyGA_WallClimb : public USKGameplayAbility
{
	GENERATED_BODY()
	
public:
    USpyGA_WallClimb();

protected:
    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled) override;

    virtual void InputPressed(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo) override;

protected:
    UFUNCTION()
    bool TryToggleClimbAction();

    UFUNCTION()
    void StartWallClimb(const FClimbData& InClimbData, const FClimbWallData& InClimbWallData);

    UFUNCTION()
    void EndWallClimb();

protected:
    UPROPERTY(EditDefaultsOnly)
    TObjectPtr<UInputMappingContext> InputMappingContext;
};
