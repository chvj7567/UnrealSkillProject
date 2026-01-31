// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Calculation/SpyHealCalculation.h"

USpyHealCalculation::USpyHealCalculation()
{
	CaptureMaxHealth(EGameplayEffectAttributeCaptureSource::Target);
	CaptureHealth(EGameplayEffectAttributeCaptureSource::Target);
}

float USpyHealCalculation::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	FAggregatorEvaluateParameters EvaluateParameters;
	GetSourceAndTargetTags(Spec, EvaluateParameters);

	float MaxHealth = 0.0f;
	GetCapturedAttributeMagnitude(MaxHealthDef, Spec, EvaluateParameters, MaxHealth);

	float Health = 0.0f;
	GetCapturedAttributeMagnitude(HealthDef, Spec, EvaluateParameters, Health);

	return Health + 1.0f >= MaxHealth ? MaxHealth - Health : 1.0f;
}
