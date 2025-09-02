// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/CharacterGameplayAbility.h"
#include "GameplayAbility_SkillA.generated.h"

/**
 * 
 */
UCLASS()
class SKILLPROJECT_API UGameplayAbility_SkillA : public UCharacterGameplayAbility
{
	GENERATED_BODY()
	
protected:
    virtual void OnMontageCompleted() override;
    virtual void OnMontageCancelled() override;
    virtual void OnWaitGameplayEvent(FGameplayEventData Payload) override;

public:
    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;
};
