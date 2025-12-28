// Fill out your copyright notice in the Description page of Project Settings.


#include "Calculation/SpyBaseMagnitudeCalculation.h"
#include "Attribute/SpyAttributeSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyBaseMagnitudeCalculation)

USpyBaseMagnitudeCalculation::USpyBaseMagnitudeCalculation()
{
}

void USpyBaseMagnitudeCalculation::GetSourceAndTargetTags(const FGameplayEffectSpec& Spec, FAggregatorEvaluateParameters& OutParams) const
{
	OutParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	OutParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
}

void USpyBaseMagnitudeCalculation::CaptureHealth(EGameplayEffectAttributeCaptureSource Source, bool bSnapshot)
{
	HealthDef.AttributeToCapture = USpyAttributeSet::GetHealthAttribute();
	HealthDef.AttributeSource = Source;
	HealthDef.bSnapshot = bSnapshot;

	RelevantAttributesToCapture.Add(HealthDef);
}

void USpyBaseMagnitudeCalculation::CaptureMaxHealth(EGameplayEffectAttributeCaptureSource Source, bool bSnapshot)
{
	MaxHealthDef.AttributeToCapture = USpyAttributeSet::GetMaxHealthAttribute();
	MaxHealthDef.AttributeSource = Source;
	MaxHealthDef.bSnapshot = bSnapshot;

	RelevantAttributesToCapture.Add(MaxHealthDef);
}

void USpyBaseMagnitudeCalculation::CaptureMana(EGameplayEffectAttributeCaptureSource Source, bool bSnapshot)
{
	ManaDef.AttributeToCapture = USpyAttributeSet::GetManaAttribute();
	ManaDef.AttributeSource = Source;
	ManaDef.bSnapshot = bSnapshot;

	RelevantAttributesToCapture.Add(ManaDef);
}

void USpyBaseMagnitudeCalculation::CaptureMaxMana(EGameplayEffectAttributeCaptureSource Source, bool bSnapshot)
{
	MaxManaDef.AttributeToCapture = USpyAttributeSet::GetMaxManaAttribute();
	MaxManaDef.AttributeSource = Source;
	MaxManaDef.bSnapshot = bSnapshot;

	RelevantAttributesToCapture.Add(MaxManaDef);
}
