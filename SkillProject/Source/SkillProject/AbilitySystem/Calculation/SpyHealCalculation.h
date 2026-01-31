// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Calculation/SKBaseMagnitudeCalculation.h"

#include "SpyHealCalculation.generated.h"

UCLASS()
class SKILLPROJECT_API USpyHealCalculation : public USKBaseMagnitudeCalculation
{
	GENERATED_BODY()

public:
	USpyHealCalculation();

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
