// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Components/GameFrameworkComponent.h"

#include "SpyHealthComponent.generated.h"

class USpyAbilitySystemComponent;
class USpyCharacterAttributeSet;
struct FGameplayEffectSpec;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSpyHealth_DeathEvent, AActor*, OwningActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FSpyHealth_AttributeChanged, USpyHealthComponent*, HealthComponent, float, OldValue, float, NewValue, AActor*, Instigator);

UENUM(BlueprintType)
enum class ESpyDeathState : uint8
{
	NotDead = 0,
	DeathStarted,
	DeathFinished
};

UCLASS()
class SKILLPROJECT_API USpyHealthComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure)
	static USpyHealthComponent* FindHealthComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<USpyHealthComponent>() : nullptr); }

	void InitializeByAbilitySystem(USpyAbilitySystemComponent* InASC);
	void UnInitializeByAbilitySystem();
	float GetHealth() const;
	float GetMaxHealth() const;
	float GetHealthNormalized() const;

	UFUNCTION(BlueprintCallable)
	ESpyDeathState GetDeathState() const { return DeathState; }
	
protected:
	virtual void OnUnregister() override;

	virtual void HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
	virtual void HandleMaxHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);

	UFUNCTION()
	virtual void OnRep_DeathState(ESpyDeathState OldDeathState);

public:
	UPROPERTY()
	FSpyHealth_AttributeChanged OnHealthChanged;

	UPROPERTY()
	FSpyHealth_AttributeChanged OnMaxHealthChanged;

	UPROPERTY(BlueprintAssignable)
	FSpyHealth_DeathEvent OnDeathStarted;

	UPROPERTY(BlueprintAssignable)
	FSpyHealth_DeathEvent OnDeathFinished;

protected:

	UPROPERTY()
	TObjectPtr<USpyAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<const USpyCharacterAttributeSet> HealthSet;

	UPROPERTY(ReplicatedUsing = OnRep_DeathState)
	ESpyDeathState DeathState;
};
