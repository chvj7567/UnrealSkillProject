// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Ability/SKGameplayAbility_SkillAction.h"

#include "SpyGameplayAbility_SkillAction.generated.h"

UCLASS()
class SKILLPROJECT_API USpyGameplayAbility_SkillAction : public USKGameplayAbility_SkillAction
{
	GENERATED_BODY()
	
public:
	virtual void InputPressed(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

};
