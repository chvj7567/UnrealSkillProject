// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Ability/SKGameplayAbility.h"
#include "SpyGA_GrappleHook.generated.h"

class AGrappleCableActor;
class USpyMovementConfig;
class USpyGrappleTargetingComponent;

UCLASS()
class SKILLPROJECT_API USpyGA_GrappleHook : public USKGameplayAbility
{
    GENERATED_BODY()

public:
    USpyGA_GrappleHook();

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

private:
    UFUNCTION()
    void OnGrappleArrived();

    void LaunchToTarget(ACharacter* Character, const FVector& TargetLocation) const;

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    TObjectPtr<USpyMovementConfig> MovementConfig;

    UPROPERTY(EditDefaultsOnly, Category = "Config")
    FName HandBoneName = FName("hand_r");

private:
    UPROPERTY()
    TObjectPtr<AGrappleCableActor> CableActor;
};
