// Fill out your copyright notice in the Description page of Project Settings.


#include "Attribute/SKAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "SKAbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKAttributeSet)

UWorld* USKAttributeSet::GetWorld() const
{
	const UObject* Outer = GetOuter();
	check(Outer);

	return Outer->GetWorld();
}

USKAbilitySystemComponent* USKAttributeSet::GetSKAbilitySystemComponent() const
{
	return Cast<USKAbilitySystemComponent>(GetOwningAbilitySystemComponent());
}


void USKAttributeSet::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	DOREPLIFETIME(USKAttributeSet, Health);
	DOREPLIFETIME(USKAttributeSet, Mana);
	DOREPLIFETIME(USKAttributeSet, MaxHealth);
	DOREPLIFETIME(USKAttributeSet, MaxMana);
}

void USKAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USKAttributeSet, Health, OldValue);

	OnHealthChanged.Broadcast(nullptr, nullptr, nullptr, GetHealth() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetHealth());
}

void USKAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USKAttributeSet, MaxHealth, OldValue);

	OnMaxHealthChanged.Broadcast(nullptr, nullptr, nullptr, GetMaxHealth() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetMaxHealth());
}

void USKAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USKAttributeSet, Mana, OldValue);

	OnManaChanged.Broadcast(nullptr, nullptr, nullptr, GetMana() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetMana());
}

void USKAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USKAttributeSet, MaxMana, OldValue);

	OnMaxManaChanged.Broadcast(nullptr, nullptr, nullptr, GetMaxMana() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetMaxMana());
}
