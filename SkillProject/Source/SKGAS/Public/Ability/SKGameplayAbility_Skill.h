// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Ability/SKGameplayAbility.h"

#include "SKGameplayAbility_Skill.generated.h"

UCLASS()
class SKGAS_API USKGameplayAbility_Skill : public USKGameplayAbility
{
	GENERATED_BODY()
	
public:
    USKGameplayAbility_Skill();

protected:
    virtual void OnMontageCompleted() override;
    virtual void OnMontageCancelled() override;
    virtual void OnWaitGameplayEvent(FGameplayEventData Payload) override;
    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

protected:
    UFUNCTION()
    void CheckHit();

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag WaitSkillTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<UGameplayEffect> DamageEffectClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UAnimMontage> SkillMontage;
};
