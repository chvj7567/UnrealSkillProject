// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyGameplayEffectContext.h"
#include "SpyAbilitySourceInterface.h"
#include "Engine/HitResult.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGameplayEffectContext)

FSpyGameplayEffectContext* FSpyGameplayEffectContext::ExtractEffectContext(struct FGameplayEffectContextHandle Handle)
{
	FGameplayEffectContext* BaseEffectContext = Handle.Get();
	if ((BaseEffectContext != nullptr) && BaseEffectContext->GetScriptStruct()->IsChildOf(FSpyGameplayEffectContext::StaticStruct()))
	{
		return (FSpyGameplayEffectContext*)BaseEffectContext;
	}

	return nullptr;
}

bool FSpyGameplayEffectContext::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess);

	return true;
}

void FSpyGameplayEffectContext::SetAbilitySource(ISpyAbilitySourceInterface* InObject, float InAbilityLevel)
{
	SourceObject = MakeWeakObjectPtr(Cast<UObject>(InObject));
	AbilityLevel = InAbilityLevel;
}

const ISpyAbilitySourceInterface* FSpyGameplayEffectContext::GetAbilitySource() const
{
	return Cast<ISpyAbilitySourceInterface>(SourceObject.Get());
}

const UPhysicalMaterial* FSpyGameplayEffectContext::GetPhysicalMaterial() const
{
	if (const FHitResult* HitResultPtr = GetHitResult())
	{
		return HitResultPtr->PhysMaterial.Get();
	}
	return nullptr;
}