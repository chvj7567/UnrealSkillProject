// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Ability/SKGameplayAbility_SkillMove.h"

#include "SpyGA_SkillMove_Vault.generated.h"

UCLASS()
class SKILLPROJECT_API USpyGA_SkillMove_Vault : public USKGameplayAbility_SkillMove
{
	GENERATED_BODY()
	
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

protected:
    UFUNCTION()
    void OnSyncMotionWarpingData(FMotionWarpingData InVaultData);

protected:
    UPROPERTY(EditAnywhere)
    FName MotionWarpingStartName;

    UPROPERTY(EditAnywhere)
    FName MotionWarpingEndName;
};
