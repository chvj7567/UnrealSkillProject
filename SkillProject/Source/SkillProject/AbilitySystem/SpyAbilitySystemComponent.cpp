// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/SpyAbilitySystemComponent.h"

USpyAbilitySystemComponent::USpyAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void USpyAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	UE_LOG(LogTemp, Warning, TEXT("InitAbilityActorInfo called"));
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

void USpyAbilitySystemComponent::NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability)
{
	Super::NotifyAbilityActivated(Handle, Ability);

	UE_LOG(LogTemp, Warning, TEXT("NotifyAbilityActivated called"));
}

void USpyAbilitySystemComponent::NotifyAbilityFailed(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason)
{
	Super::NotifyAbilityFailed(Handle, Ability, FailureReason);

	UE_LOG(LogTemp, Warning, TEXT("NotifyAbilityFailed called"));
}

void USpyAbilitySystemComponent::NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, bool bWasCancelled)
{
	Super::NotifyAbilityEnded(Handle, Ability, bWasCancelled);

	UE_LOG(LogTemp, Warning, TEXT("NotifyAbilityEnded called"));
}

void USpyAbilitySystemComponent::ApplyAbilityBlockAndCancelTags(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bEnableBlockTags, const FGameplayTagContainer& BlockTags, bool bExecuteCancelTags, const FGameplayTagContainer& CancelTags)
{
	Super::ApplyAbilityBlockAndCancelTags(AbilityTags, RequestingAbility, bEnableBlockTags, BlockTags, bExecuteCancelTags, CancelTags);

	UE_LOG(LogTemp, Warning, TEXT("ApplyAbilityBlockAndCancelTags called"));
}

void USpyAbilitySystemComponent::HandleChangeAbilityCanBeCanceled(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bCanBeCanceled)
{
	Super::HandleChangeAbilityCanBeCanceled(AbilityTags, RequestingAbility, bCanBeCanceled);

	UE_LOG(LogTemp, Warning, TEXT("HandleChangeAbilityCanBeCanceled called"));
}
