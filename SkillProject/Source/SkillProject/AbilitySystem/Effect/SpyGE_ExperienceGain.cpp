// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Effect/SpyGE_ExperienceGain.h"
#include "Character/SpyCharacterAttributeSet.h"
#include "Util/SpyGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGE_ExperienceGain)

USpyGE_ExperienceGain::USpyGE_ExperienceGain()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = SpyGameplayTags::Data_Experience_Gain;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = USpyCharacterAttributeSet::GetExperienceAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	Modifiers.Add(Modifier);
}
