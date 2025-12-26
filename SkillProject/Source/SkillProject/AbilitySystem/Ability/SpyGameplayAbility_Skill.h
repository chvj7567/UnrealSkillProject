// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/SpyGameplayAbility.h"
#include "Util/DefineEnum.h"

#include "SpyGameplayAbility_Skill.generated.h"

UCLASS()
class SKILLPROJECT_API USpyGameplayAbility_Skill : public USpyGameplayAbility
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

    void SetWeaponSkillTag();

private:
    UPROPERTY(EditAnywhere)
    ESpyCharacterType CharacterType;
};
