// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/SpyAbilitySystemComponent.h"

USpyAbilitySystemComponent::USpyAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void USpyAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);
}

void USpyAbilitySystemComponent::OnGiveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	Super::OnGiveAbility(AbilitySpec);

	if (AbilityChangedDelegate.IsBound())
	{
		AbilityChangedDelegate.Broadcast(AbilitySpec.Handle, true);
	}
}

void USpyAbilitySystemComponent::OnRemoveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	if (AbilityChangedDelegate.IsBound())
	{
		AbilityChangedDelegate.Broadcast(AbilitySpec.Handle, false);
	}

	Super::OnRemoveAbility(AbilitySpec);
}
