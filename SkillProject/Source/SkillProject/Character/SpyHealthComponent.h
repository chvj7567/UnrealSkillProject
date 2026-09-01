// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Components/GameFrameworkComponent.h"

#include "SpyHealthComponent.generated.h"

class USpyAbilitySystemComponent;
class USpyCharacterAttributeSet;
struct FGameplayEffectSpec;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSpyHealth_DeathEvent, AActor*, OwningActor, AActor*, CauserActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSpyHealth_HitEvent,
    float, Damage,
    bool, bCritical,
    AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FSpyHealth_AttributeChanged, USpyHealthComponent*, HealthComponent, float, OldValue, float, NewValue, AActor*, Instigator);

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

	//# 사망 후 추가 피해로 OnDeath 가 중복 발화되는 것을 막는 bDead 래치를 해제한다 — 리스폰 완료 시 호출.
	void ResetDeathState();

protected:
	virtual void OnUnregister() override;

	virtual void HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
	virtual void HandleMaxHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);

	//# 사망당 OnDeath 를 1회만 발화시키는 래치. ResetDeathState() 로만 해제된다.
	bool bDead = false;

public:
	UPROPERTY()
	FSpyHealth_AttributeChanged OnHealthChanged;

	UPROPERTY()
	FSpyHealth_AttributeChanged OnMaxHealthChanged;

	UPROPERTY(BlueprintAssignable)
	FSpyHealth_DeathEvent OnDeath;

	UPROPERTY(BlueprintAssignable)
	FSpyHealth_HitEvent OnHit;

protected:

	UPROPERTY()
	TObjectPtr<USpyAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<const USpyCharacterAttributeSet> HealthSet;
};
