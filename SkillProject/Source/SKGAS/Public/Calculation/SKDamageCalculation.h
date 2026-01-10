// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "Calculation/SKBaseMagnitudeCalculation.h"

#include "SKDamageCalculation.generated.h"

UCLASS()
class SKGAS_API USKDamageCalculation : public USKBaseMagnitudeCalculation
{
	GENERATED_BODY()
	
public:
	USKDamageCalculation();

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
