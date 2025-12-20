// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Calculation/SpyDamageCalculation.h"
#include "AbilitySystem/Attribute/SpyAttributeSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyDamageCalculation)

USpyDamageCalculation::USpyDamageCalculation()
{
    HealthDef.AttributeToCapture = USpyAttributeSet::GetHealthAttribute();
    HealthDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
    HealthDef.bSnapshot = false;

    MaxHealthDef.AttributeToCapture = USpyAttributeSet::GetMaxHealthAttribute();
    MaxHealthDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
    MaxHealthDef.bSnapshot = false;

    RelevantAttributesToCapture.Add(HealthDef);
    RelevantAttributesToCapture.Add(MaxHealthDef);
}

float USpyDamageCalculation::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
    const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    FAggregatorEvaluateParameters EvaluateParameters;
    EvaluateParameters.SourceTags = SourceTags;
    EvaluateParameters.TargetTags = TargetTags;

    float Health = 0.0f;
    GetCapturedAttributeMagnitude(HealthDef, Spec, EvaluateParameters, Health);

    float MaxHealth = 0.0f;
    GetCapturedAttributeMagnitude(MaxHealthDef, Spec, EvaluateParameters, MaxHealth);

    return Health < 1.0f ? -Health : -1.0f;
}
