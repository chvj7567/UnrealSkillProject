// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayEffectTypes.h"

#include "SKGameplayEffectContext.generated.h"

class ISKAbilitySourceInterface;

USTRUCT()
struct FSKGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

	FSKGameplayEffectContext()
		: FGameplayEffectContext()
	{
	}

	FSKGameplayEffectContext(AActor* InInstigator, AActor* InEffectCauser)
		: FGameplayEffectContext(InInstigator, InEffectCauser)
	{
	}
	
	static FSKGameplayEffectContext* ExtractEffectContext(struct FGameplayEffectContextHandle Handle);

	void SetAbilitySource(ISKAbilitySourceInterface* InObject, float InSourceLevel);

	const ISKAbilitySourceInterface* GetAbilitySource() const;

	virtual FGameplayEffectContext* Duplicate() const override
	{
		FSKGameplayEffectContext* NewContext = new FSKGameplayEffectContext();
		*NewContext = *this;
		if (GetHitResult())
		{
			// Does a deep copy of the hit result
			NewContext->AddHitResult(*GetHitResult(), true);
		}
		return NewContext;
	}

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FSKGameplayEffectContext::StaticStruct();
	}

	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;

	const UPhysicalMaterial* GetPhysicalMaterial() const;
};

template<>
struct TStructOpsTypeTraits<FSKGameplayEffectContext> : public TStructOpsTypeTraitsBase2<FSKGameplayEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};
