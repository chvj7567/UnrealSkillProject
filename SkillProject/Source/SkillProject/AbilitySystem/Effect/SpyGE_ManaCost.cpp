// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Effect/SpyGE_ManaCost.h"
#include "Attribute/SKAttributeSet.h"
#include "Util/SpyGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGE_ManaCost)

USpyGE_ManaCost::USpyGE_ManaCost()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = SpyGameplayTags::Data_Cost_Mana;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = USKAttributeSet::GetManaAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	Modifiers.Add(Modifier);
}
