// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayEffectTypes.h"

#include "SpyGameplayEffectContext.generated.h"

class ISpyAbilitySourceInterface;

USTRUCT()
struct FSpyGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

	FSpyGameplayEffectContext()
		: FGameplayEffectContext()
	{
	}

	FSpyGameplayEffectContext(AActor* InInstigator, AActor* InEffectCauser)
		: FGameplayEffectContext(InInstigator, InEffectCauser)
	{
	}
	
	static SKILLPROJECT_API FSpyGameplayEffectContext* ExtractEffectContext(struct FGameplayEffectContextHandle Handle);

	void SetAbilitySource(ISpyAbilitySourceInterface* InObject, float InSourceLevel);

	const ISpyAbilitySourceInterface* GetAbilitySource() const;

	virtual FGameplayEffectContext* Duplicate() const override
	{
		FSpyGameplayEffectContext* NewContext = new FSpyGameplayEffectContext();
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
		return FSpyGameplayEffectContext::StaticStruct();
	}

	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;

	const UPhysicalMaterial* GetPhysicalMaterial() const;
};

template<>
struct TStructOpsTypeTraits<FSpyGameplayEffectContext> : public TStructOpsTypeTraitsBase2<FSpyGameplayEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};
