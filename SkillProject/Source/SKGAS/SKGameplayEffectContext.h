// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"

#include "SKGameplayEffectContext.generated.h"

class ISKAbilitySourceInterface;

USTRUCT()
struct SKGAS_API FSKGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

public:
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

	void SetHitDirectionTag(FGameplayTag InTag) { HitDirectionTag = InTag; }
	FGameplayTag GetHitDirectionTag() const { return HitDirectionTag; }

	void SetIsCritical(bool bInIsCritical) { bIsCritical = bInIsCritical; }
	bool IsCritical() const { return bIsCritical; }

protected:
	UPROPERTY()
	FGameplayTag HitDirectionTag;

	UPROPERTY()
	bool bIsCritical;
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
