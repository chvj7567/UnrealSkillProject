// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"

#include "SKAttributeSet.generated.h"

struct FGameplayEffectSpec;

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName)\
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName)\
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName)\
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName)\
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

DECLARE_MULTICAST_DELEGATE_SixParams(FSKAttributeEvent, AActor* /*EffectInstigator*/, AActor* /*EffectCauser*/, const FGameplayEffectSpec* /*EffectSpec*/, float /*EffectMagnitude*/, float /*OldValue*/, float /*NewValue*/);

UCLASS()
class SKGAS_API USKAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
public:
	UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(USKAttributeSet, Health);

	UPROPERTY(ReplicatedUsing = OnRep_MaxHealth, BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(USKAttributeSet, MaxHealth);

	UPROPERTY(ReplicatedUsing = OnRep_Mana, BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(USKAttributeSet, Mana);
	UPROPERTY(ReplicatedUsing = OnRep_MaxMana, BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(USKAttributeSet, MaxMana);

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Mana(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxMana(const FGameplayAttributeData& OldValue);

public:
	mutable FSKAttributeEvent OnHealthChanged;
	mutable FSKAttributeEvent OnMaxHealthChanged;
	mutable FSKAttributeEvent OnManaChanged;
	mutable FSKAttributeEvent OnMaxManaChanged;
};
