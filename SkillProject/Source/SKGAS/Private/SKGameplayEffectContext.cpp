// Fill out your copyright notice in the Description page of Project Settings.


#include "SKGameplayEffectContext.h"
#include "SKAbilitySourceInterface.h"
#include "Engine/HitResult.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKGameplayEffectContext)

FSKGameplayEffectContext* FSKGameplayEffectContext::ExtractEffectContext(struct FGameplayEffectContextHandle Handle)
{
	FGameplayEffectContext* BaseEffectContext = Handle.Get();
	if ((BaseEffectContext != nullptr) && BaseEffectContext->GetScriptStruct()->IsChildOf(FSKGameplayEffectContext::StaticStruct()))
	{
		return (FSKGameplayEffectContext*)BaseEffectContext;
	}

	return nullptr;
}

bool FSKGameplayEffectContext::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess);

	return true;
}

void FSKGameplayEffectContext::SetAbilitySource(ISKAbilitySourceInterface* InObject, float InAbilityLevel)
{
	SourceObject = MakeWeakObjectPtr(Cast<UObject>(InObject));
	AbilityLevel = InAbilityLevel;
}

const ISKAbilitySourceInterface* FSKGameplayEffectContext::GetAbilitySource() const
{
	return Cast<ISKAbilitySourceInterface>(SourceObject.Get());
}

const UPhysicalMaterial* FSKGameplayEffectContext::GetPhysicalMaterial() const
{
	if (const FHitResult* HitResultPtr = GetHitResult())
	{
		return HitResultPtr->PhysMaterial.Get();
	}
	return nullptr;
}