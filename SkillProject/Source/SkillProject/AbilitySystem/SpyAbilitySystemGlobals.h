// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AbilitySystemGlobals.h"
#include "SpyAbilitySystemGlobals.generated.h"


UCLASS(Config = Game)
class SKILLPROJECT_API USpyAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()

	USpyAbilitySystemGlobals()
		: UAbilitySystemGlobals()
	{
	}
	
	virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;
};
