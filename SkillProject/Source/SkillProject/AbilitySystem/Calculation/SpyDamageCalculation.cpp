// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyDamageCalculation.h"
#include "SKGameplayEffectContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyDamageCalculation)

USpyDamageCalculation::USpyDamageCalculation()
{
	CaptureHealth(EGameplayEffectAttributeCaptureSource::Target);
}

float USpyDamageCalculation::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	FAggregatorEvaluateParameters EvaluateParameters;
	GetSourceAndTargetTags(Spec, EvaluateParameters);

	float Health = 0.0f;
	GetCapturedAttributeMagnitude(HealthDef, Spec, EvaluateParameters, Health);

	FGameplayEffectContextHandle ContextHandle = Spec.GetContext();
	if (FSKGameplayEffectContext* CustomContext = FSKGameplayEffectContext::ExtractEffectContext(ContextHandle))
	{
		const bool bIsCritical = FMath::FRand() <= 0.5f;
		CustomContext->SetIsCritical(bIsCritical);
	}

	return Health < 1.0f ? -Health : -1.0f;
}
