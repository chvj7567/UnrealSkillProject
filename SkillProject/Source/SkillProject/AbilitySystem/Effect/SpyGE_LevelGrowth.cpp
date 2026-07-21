// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Effect/SpyGE_LevelGrowth.h"
#include "Character/SpyCharacterAttributeSet.h"
#include "Util/SpyGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGE_LevelGrowth)

USpyGE_LevelGrowth::USpyGE_LevelGrowth()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat HealthSetByCaller;
	HealthSetByCaller.DataTag = SpyGameplayTags::Data_Level_MaxHealthGrowth;

	FGameplayModifierInfo MaxHealthModifier;
	MaxHealthModifier.Attribute = USpyCharacterAttributeSet::GetMaxHealthAttribute();
	MaxHealthModifier.ModifierOp = EGameplayModOp::Additive;
	MaxHealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(HealthSetByCaller);

	Modifiers.Add(MaxHealthModifier);

	FSetByCallerFloat ManaSetByCaller;
	ManaSetByCaller.DataTag = SpyGameplayTags::Data_Level_MaxManaGrowth;

	FGameplayModifierInfo MaxManaModifier;
	MaxManaModifier.Attribute = USpyCharacterAttributeSet::GetMaxManaAttribute();
	MaxManaModifier.ModifierOp = EGameplayModOp::Additive;
	MaxManaModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(ManaSetByCaller);

	Modifiers.Add(MaxManaModifier);
}
