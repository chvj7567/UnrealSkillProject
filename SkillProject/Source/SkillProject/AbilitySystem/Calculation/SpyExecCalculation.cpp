// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Calculation/SpyExecCalculation.h"
#include "AbilitySystem/Attribute/SpyAttributeSet.h"
#include "Manager/SpyAssetManager.h"
#include "Util/SpyGameplayTags.h"
#include "AbilitySystem/SpyGameplayEffectContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyExecCalculation)

struct DamageCapture
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(Health);

	DECLARE_ATTRIBUTE_CAPTUREDEF(Mana);

	DamageCapture()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(USpyAttributeSet, Health, Target, false);

		DEFINE_ATTRIBUTE_CAPTUREDEF(USpyAttributeSet, Mana, Source, false);
	}
};

static DamageCapture& GetDamageCapture()
{
	static DamageCapture DamageCapture;
	return DamageCapture;
}

USpyExecCalculation::USpyExecCalculation()
{
	RelevantAttributesToCapture.Add(GetDamageCapture().HealthDef);
	RelevantAttributesToCapture.Add(GetDamageCapture().ManaDef);
}

void USpyExecCalculation::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
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

	//SourceASC->ApplyModToAttribute(USpyAttributeSet::GetManaAttribute(), EGameplayModOp::Additive, -Damage / 2.0f);

	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(GetDamageCapture().HealthProperty, EGameplayModOp::Additive, Damage));
	//OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(GetDamageCapture().ManaProperty, EGameplayModOp::Additive, -Damage / 2.0f));

	USpyAssetManager& AssetManager = USpyAssetManager::Get();
	TSubclassOf<UGameplayEffect> EffectClass = AssetManager.GetSubclassByName<UGameplayEffect>(FName("Mana"));

	if (SourceASC && EffectClass) // ManaRecoveryGEClass는 TSubclassOf<UGameplayEffect>
	{
		// 3-1. GE Spec 생성
		FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
		if (FSpyGameplayEffectContext* CustomContext = FSpyGameplayEffectContext::ExtractEffectContext(ContextHandle))
		{
			CustomContext->AddInstigator(SourceActor, SourceActor);
		}

		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, 1.0f, ContextHandle);

		if (SpecHandle.IsValid())
		{
			// 3-2. 계산된 마나 회복량을 SetByCaller로 주입 (태그는 프로젝트 설정에 맞게)
			float RecoveryAmount = Damage * 0.5f;
			SpecHandle.Data.Get()->SetSetByCallerMagnitude(SpyGameplayTags::Skill_E, -1.0f);

			// 3-3. 시전자 자신에게 적용
			SourceASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
}
