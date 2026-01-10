// Fill out your copyright notice in the Description page of Project Settings.


#include "Calculation/SKBaseMagnitudeCalculation.h"
#include "Attribute/SKAttributeSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKBaseMagnitudeCalculation)

USKBaseMagnitudeCalculation::USKBaseMagnitudeCalculation()
{
}

void USKBaseMagnitudeCalculation::GetSourceAndTargetTags(const FGameplayEffectSpec& Spec, FAggregatorEvaluateParameters& OutParams) const
{
	OutParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	OutParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
}

void USKBaseMagnitudeCalculation::CaptureHealth(EGameplayEffectAttributeCaptureSource Source, bool bSnapshot)
{
	HealthDef.AttributeToCapture = USKAttributeSet::GetHealthAttribute();
	HealthDef.AttributeSource = Source;
	HealthDef.bSnapshot = bSnapshot;

	RelevantAttributesToCapture.Add(HealthDef);
}

void USKBaseMagnitudeCalculation::CaptureMaxHealth(EGameplayEffectAttributeCaptureSource Source, bool bSnapshot)
{
	MaxHealthDef.AttributeToCapture = USKAttributeSet::GetMaxHealthAttribute();
	MaxHealthDef.AttributeSource = Source;
	MaxHealthDef.bSnapshot = bSnapshot;

	RelevantAttributesToCapture.Add(MaxHealthDef);
}

void USKBaseMagnitudeCalculation::CaptureMana(EGameplayEffectAttributeCaptureSource Source, bool bSnapshot)
{
	ManaDef.AttributeToCapture = USKAttributeSet::GetManaAttribute();
	ManaDef.AttributeSource = Source;
	ManaDef.bSnapshot = bSnapshot;

	RelevantAttributesToCapture.Add(ManaDef);
}

void USKBaseMagnitudeCalculation::CaptureMaxMana(EGameplayEffectAttributeCaptureSource Source, bool bSnapshot)
{
	MaxManaDef.AttributeToCapture = USKAttributeSet::GetMaxManaAttribute();
	MaxManaDef.AttributeSource = Source;
	MaxManaDef.bSnapshot = bSnapshot;

	RelevantAttributesToCapture.Add(MaxManaDef);
}
