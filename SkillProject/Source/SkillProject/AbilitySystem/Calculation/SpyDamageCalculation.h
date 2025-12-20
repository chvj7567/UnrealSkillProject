// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "SpyDamageCalculation.generated.h"

UCLASS()
class SKILLPROJECT_API USpyDamageCalculation : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()
	
public:
	USpyDamageCalculation();

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	FGameplayEffectAttributeCaptureDefinition HealthDef;
	FGameplayEffectAttributeCaptureDefinition MaxHealthDef;
};
