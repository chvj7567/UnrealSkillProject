// Fill out your copyright notice in the Description page of Project Settings.

#include "SpyAbilitySystemGlobals.h"
#include "SpyGameplayEffectContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyAbilitySystemGlobals)

USpyAbilitySystemGlobals::USpyAbilitySystemGlobals()
{
	UE_LOG(LogTemp, Warning, TEXT("USpyAbilitySystemGlobals Create"));
}

FGameplayEffectContext* USpyAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FSpyGameplayEffectContext();
}