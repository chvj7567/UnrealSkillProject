// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "Calculation/SpyBaseMagnitudeCalculation.h"

#include "SpyDamageCalculation.generated.h"

UCLASS()
class SPYGAS_API USpyDamageCalculation : public USpyBaseMagnitudeCalculation
{
	GENERATED_BODY()
	
public:
	USpyDamageCalculation();

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
