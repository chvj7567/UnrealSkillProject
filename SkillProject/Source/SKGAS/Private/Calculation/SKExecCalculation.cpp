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
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	AActor* TargetActor = TargetASC != nullptr ? TargetASC->GetAvatarActor() : nullptr;

	UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	AActor* SourceActor = SourceASC != nullptr ? SourceASC->GetAvatarActor() : nullptr;

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.TargetTags = TargetTags;
	EvaluateParameters.SourceTags = SourceTags;

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
	//SourceASC->ApplyModToAttribute(USKAttributeSet::GetManaAttribute(), EGameplayModOp::Additive, -Damage / 2.0f);

	/*if (SourceASC && EffectClass)
	{
		FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
		if (FSKGameplayEffectContext* CustomContext = FSKGameplayEffectContext::ExtractEffectContext(ContextHandle))
		{
			CustomContext->AddInstigator(SourceActor, SourceActor);
		}

		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, 1.0f, ContextHandle);

		if (SpecHandle.IsValid())
		{
			float RecoveryAmount = Damage * 0.5f;
			SpecHandle.Data.Get()->SetSetByCallerMagnitude(SKGASGameplayTags::Skill_A, -10.0f);

			SourceASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}*/
}
