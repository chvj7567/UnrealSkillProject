// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Components/GameFrameworkComponent.h"

#include "SpyHealthComponent.generated.h"

class USpyAbilitySystemComponent;

class USpyAttributeSet;
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
	TObjectPtr<USpyAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<const USpyAttributeSet> HealthSet;
	
public:
	void Initialize(USpyAbilitySystemComponent* InASC);
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
