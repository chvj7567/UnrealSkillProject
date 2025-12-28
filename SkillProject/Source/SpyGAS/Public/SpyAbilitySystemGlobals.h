// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AbilitySystemGlobals.h"

#include "SpyAbilitySystemGlobals.generated.h"


UCLASS(Config = Game)
class SPYGAS_API USpyAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()

public:
	USpyAbilitySystemGlobals();

public:
	virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;
};
