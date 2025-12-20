// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"

#include "SpyBaseMagnitudeCalculation.generated.h"

UCLASS()
class SKILLPROJECT_API USpyBaseMagnitudeCalculation : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()
	
public:
	USpyBaseMagnitudeCalculation();

public:
	void GetSourceAndTargetTags(const FGameplayEffectSpec& Spec, FAggregatorEvaluateParameters& OutParams) const;

protected:
	void CaptureHealth(EGameplayEffectAttributeCaptureSource Source, bool bSnapshot = false);
	void CaptureMaxHealth(EGameplayEffectAttributeCaptureSource Source, bool bSnapshot = false);
	void CaptureMana(EGameplayEffectAttributeCaptureSource Source, bool bSnapshot = false);
	void CaptureMaxMana(EGameplayEffectAttributeCaptureSource Source, bool bSnapshot = false);

protected:
	FGameplayEffectAttributeCaptureDefinition HealthDef;
	FGameplayEffectAttributeCaptureDefinition MaxHealthDef;

	FGameplayEffectAttributeCaptureDefinition ManaDef;
	FGameplayEffectAttributeCaptureDefinition MaxManaDef;
};
