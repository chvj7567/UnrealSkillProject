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

protected:
    UFUNCTION()
    void OnSyncMotionWarpingData(FVaultMotionWarpingData InVaultData);

protected:
    UPROPERTY(EditAnywhere)
    FName MotionWarpingStartName;

    UPROPERTY(EditAnywhere)
    FName MotionWarpingEndName;
};
