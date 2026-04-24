// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/SpyHealthComponent.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Character/SpyCharacterAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "SKGameplayEffectContext.h"
#include "System/SpyPlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyHealthComponent)

void USpyHealthComponent::InitializeByAbilitySystem(USpyAbilitySystemComponent* InASC)
{
	AActor* Owner = GetOwner();
	check(Owner);

	if (InASC == nullptr)
		return;

	AbilitySystemComponent = InASC;

	HealthSet = AbilitySystemComponent->GetSet<USpyCharacterAttributeSet>();

	HealthSet->OnHealthChanged.AddUObject(this, &ThisClass::HandleHealthChanged);
	HealthSet->OnMaxHealthChanged.AddUObject(this, &ThisClass::HandleMaxHealthChanged);

	OnHealthChanged.Broadcast(this, HealthSet->GetHealth(), HealthSet->GetHealth(), nullptr);
	OnMaxHealthChanged.Broadcast(this, HealthSet->GetHealth(), HealthSet->GetHealth(), nullptr);
}

void USpyHealthComponent::UnInitializeByAbilitySystem()
{
	if (HealthSet)
	{
		HealthSet->OnHealthChanged.RemoveAll(this);
		HealthSet->OnMaxHealthChanged.RemoveAll(this);
	}

	HealthSet = nullptr;
	AbilitySystemComponent = nullptr;
}

float USpyHealthComponent::GetHealth() const
{
	return (HealthSet ? HealthSet->GetHealth() : 0.0f);
}

float USpyHealthComponent::GetMaxHealth() const
{
	return (HealthSet ? HealthSet->GetMaxHealth() : 0.0f);
}

float USpyHealthComponent::GetHealthNormalized() const
{
	if (HealthSet)
	{
		const float Health = HealthSet->GetHealth();
		const float MaxHealth = HealthSet->GetMaxHealth();

		return ((MaxHealth > 0.0f) ? (Health / MaxHealth) : 0.0f);
	}

	return 0.0f;
}

void USpyHealthComponent::OnUnregister()
{
	UnInitializeByAbilitySystem();

	Super::OnUnregister();
}

void USpyHealthComponent::HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	if (NewValue <= 0)
	{
		OnDeath.Broadcast(DamageInstigator, DamageCauser);
	}

	OnHealthChanged.Broadcast(this, OldValue, NewValue, DamageInstigator);

	// 실제 피해가 발생한 경우에만 (DamageMagnitude < 0 = 감소)
	if (DamageMagnitude < 0.0f)
	{
		bool bCritical = false;
		if (DamageEffectSpec)
		{
			FSKGameplayEffectContext* Ctx = FSKGameplayEffectContext::ExtractEffectContext(
				DamageEffectSpec->GetContext());
			if (Ctx)
			{
				bCritical = Ctx->IsCritical();
			}
		}

		const float ActualDamage = FMath::Abs(DamageMagnitude);

		// 피격자 쪽 이벤트
		OnHit.Broadcast(ActualDamage, bCritical, DamageCauser);

		// 공격자(DamageCauser)가 플레이어라면 해당 PC에도 알림
		if (APawn* CauserPawn = Cast<APawn>(DamageCauser))
		{
			if (ASpyPlayerController* PC = Cast<ASpyPlayerController>(CauserPawn->GetController()))
			{
				PC->HandleDealtHit(bCritical);
			}
		}
	}
}

void USpyHealthComponent::HandleMaxHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	OnMaxHealthChanged.Broadcast(this, OldValue, NewValue, DamageInstigator);
}