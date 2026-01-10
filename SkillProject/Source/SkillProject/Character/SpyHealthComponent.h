// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Components/GameFrameworkComponent.h"

#include "SpyHealthComponent.generated.h"

class USKAbilitySystemComponent;

class USKAttributeSet;
struct FGameplayEffectSpec;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FSpyHealth_AttributeChanged, USpyHealthComponent*, HealthComponent, float, OldValue, float, NewValue, AActor*, Instigator);

UCLASS()
class SKILLPROJECT_API USpyHealthComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()

protected:
	bool IsInitialized;

protected:
	UPROPERTY()
	TObjectPtr<USKAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<const USKAttributeSet> HealthSet;
	
public:
	void Initialize(USKAbilitySystemComponent* InASC);
	void UnInitialize();
	float GetHealth() const;
	float GetMaxHealth() const;
	float GetHealthNormalized() const;
	
public:
	UPROPERTY()
	FSpyHealth_AttributeChanged OnHealthChanged;

	UPROPERTY()
	FSpyHealth_AttributeChanged OnMaxHealthChanged;

protected:
	void HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
	void HandleMaxHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
};
