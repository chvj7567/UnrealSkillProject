// Fill out your copyright notice in the Description page of Project Settings.


#include "Calculation/SKExecCalculation.h"
#include "Attribute/SKAttributeSet.h"
#include "SKGameplayTags.h"
#include "SKGameplayEffectContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKExecCalculation)

struct DamageCapture
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(Health);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Mana);

	DamageCapture()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(USKAttributeSet, Health, Target, false);

		DEFINE_ATTRIBUTE_CAPTUREDEF(USKAttributeSet, Mana, Source, false);
	}
};

static DamageCapture& GetDamageCapture()
{
	static DamageCapture DamageCapture;
	return DamageCapture;
}

USKExecCalculation::USKExecCalculation()
{
	RelevantAttributesToCapture.Add(GetDamageCapture().HealthDef);
	RelevantAttributesToCapture.Add(GetDamageCapture().ManaDef);
}

void USKExecCalculation::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	FSKGameplayEffectContext* CustomContext = FSKGameplayEffectContext::ExtractEffectContext(Spec.GetContext());
	check(CustomContext);

	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.TargetTags = TargetTags;
	EvaluateParameters.SourceTags = SourceTags;

	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	AActor* TargetActor = TargetASC != nullptr ? TargetASC->GetAvatarActor() : nullptr;

	UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	AActor* SourceActor = SourceASC != nullptr ? SourceASC->GetAvatarActor() : nullptr;

	float Health = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetDamageCapture().HealthDef, EvaluateParameters, Health);

	float Mana = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetDamageCapture().ManaDef, EvaluateParameters, Mana);

	float Damage = FMath::Min(Mana - Health, 0.0f);
	if (Health - Damage < 0.0f)
	{
		Damage = Health;
	}

	if (Damage >= 0.0f)
		return;

	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(GetDamageCapture().HealthProperty, EGameplayModOp::Additive, Damage));
}
