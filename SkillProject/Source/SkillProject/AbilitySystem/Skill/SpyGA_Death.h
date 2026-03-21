// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Ability/SKGameplayAbility_SkillAction.h"
#include "SpyGA_Death.generated.h"

UCLASS()
class SKILLPROJECT_API USpyGA_Death : public USKGameplayAbility_SkillAction
{
	GENERATED_BODY()
	
public:
    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;
};
