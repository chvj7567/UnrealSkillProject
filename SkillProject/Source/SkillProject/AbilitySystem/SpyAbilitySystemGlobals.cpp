// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/SpyAbilitySystemGlobals.h"
#include "AbilitySystem/SpyGameplayEffectContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyAbilitySystemGlobals)

FGameplayEffectContext* USpyAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FSpyGameplayEffectContext();
}