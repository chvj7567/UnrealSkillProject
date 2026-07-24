// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Effect/SpyGE_ManaRegen.h"
#include "Attribute/SKAttributeSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGE_ManaRegen)

USpyGE_ManaRegen::USpyGE_ManaRegen()
{
	//# 무한 지속 + 2초 주기로 반복 적용된다
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(2.0f);

	//# 주기마다 Mana +3 (상수). = 1.5/초
	FGameplayModifierInfo Modifier;
	Modifier.Attribute = USKAttributeSet::GetManaAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.0f));

	Modifiers.Add(Modifier);
}
