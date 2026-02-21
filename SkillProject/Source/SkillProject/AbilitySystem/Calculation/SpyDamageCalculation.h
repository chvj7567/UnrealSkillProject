// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Calculation/SKBaseMagnitudeCalculation.h"

#include "SpyDamageCalculation.generated.h"

UCLASS()
class SKILLPROJECT_API USpyDamageCalculation : public USKBaseMagnitudeCalculation
{
	GENERATED_BODY()

public:
	USpyDamageCalculation();

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
