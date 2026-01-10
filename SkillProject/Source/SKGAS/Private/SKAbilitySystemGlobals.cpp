// Fill out your copyright notice in the Description page of Project Settings.

#include "SKAbilitySystemGlobals.h"
#include "SKGameplayEffectContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKAbilitySystemGlobals)

USKAbilitySystemGlobals::USKAbilitySystemGlobals()
{
	UE_LOG(LogTemp, Warning, TEXT("USKAbilitySystemGlobals Create"));
}

FGameplayEffectContext* USKAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FSKGameplayEffectContext();
}