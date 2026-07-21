// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AbilitySystemGlobals.h"

#include "SKAbilitySystemGlobals.generated.h"


UCLASS(Config = Game)
class SKGAS_API USKAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()

public:
	USKAbilitySystemGlobals();

public:
	virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;
};
