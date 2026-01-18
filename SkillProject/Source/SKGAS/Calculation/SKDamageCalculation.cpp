// Fill out your copyright notice in the Description page of Project Settings.


#include "Calculation/SKDamageCalculation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKDamageCalculation)

USKDamageCalculation::USKDamageCalculation()
{
    CaptureHealth(EGameplayEffectAttributeCaptureSource::Target);
}

float USKDamageCalculation::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
    FAggregatorEvaluateParameters EvaluateParameters;
    GetSourceAndTargetTags(Spec, EvaluateParameters);

    float Health = 0.0f;
    GetCapturedAttributeMagnitude(HealthDef, Spec, EvaluateParameters, Health);

    return Health < 1.0f ? -Health : -1.0f;
}
