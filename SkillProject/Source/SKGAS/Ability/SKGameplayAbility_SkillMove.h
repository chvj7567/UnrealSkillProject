// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Ability/SKGameplayAbility.h"

#include "SKGameplayAbility_SkillMove.generated.h"

class UCharacterMovementComponent;
class UCapsuleComponent;

UCLASS()
class SKGAS_API USKGameplayAbility_SkillMove : public USKGameplayAbility
{
	GENERATED_BODY()
	
public:
    USKGameplayAbility_SkillMove();

protected:
    virtual void OnMontageCompleted() override;
    virtual void OnMontageCancelled() override;

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

protected:
    UFUNCTION()
    virtual void PlayMontage();

};
