// Fill out your copyright notice in the Description page of Project Settings.


#include "SKAbilitySystemComponent.h"
#include "SKGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKAbilitySystemComponent)


USKAbilitySystemComponent::USKAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void USKAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	//UE_LOG(LogTemp, Warning, TEXT("USKAbilitySystemComponent InitAbilityActorInfo called"));
}

void USKAbilitySystemComponent::OnGiveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	Super::OnGiveAbility(AbilitySpec);

	if (AbilityChangedDelegate.IsBound())
	{
		AbilityChangedDelegate.Broadcast(AbilitySpec.Handle, true);
	}
}

void USKAbilitySystemComponent::OnRemoveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	if (AbilityChangedDelegate.IsBound())
	{
		AbilityChangedDelegate.Broadcast(AbilitySpec.Handle, false);
	}

	Super::OnRemoveAbility(AbilitySpec);
}

void USKAbilitySystemComponent::NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability)
{
	Super::NotifyAbilityActivated(Handle, Ability);

	//UE_LOG(LogTemp, Warning, TEXT("USKAbilitySystemComponent NotifyAbilityActivated called : %s"), *Ability->GetName());
}

void USKAbilitySystemComponent::NotifyAbilityFailed(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason)
{
	Super::NotifyAbilityFailed(Handle, Ability, FailureReason);

	//UE_LOG(LogTemp, Warning, TEXT("USKAbilitySystemComponent NotifyAbilityFailed called : %s"), *Ability->GetName());
}

void USKAbilitySystemComponent::NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, bool bWasCancelled)
{
	Super::NotifyAbilityEnded(Handle, Ability, bWasCancelled);

	//UE_LOG(LogTemp, Warning, TEXT("USKAbilitySystemComponent NotifyAbilityEnded called : %s"), *Ability->GetName());
}

void USKAbilitySystemComponent::ApplyAbilityBlockAndCancelTags(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bEnableBlockTags, const FGameplayTagContainer& BlockTags, bool bExecuteCancelTags, const FGameplayTagContainer& CancelTags)
{
	Super::ApplyAbilityBlockAndCancelTags(AbilityTags, RequestingAbility, bEnableBlockTags, BlockTags, bExecuteCancelTags, CancelTags);

	//UE_LOG(LogTemp, Warning, TEXT("USKAbilitySystemComponent ApplyAbilityBlockAndCancelTags : %s called"), *RequestingAbility->GetName());
}

void USKAbilitySystemComponent::HandleChangeAbilityCanBeCanceled(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bCanBeCanceled)
{
	Super::HandleChangeAbilityCanBeCanceled(AbilityTags, RequestingAbility, bCanBeCanceled);

	//UE_LOG(LogTemp, Warning, TEXT("USKAbilitySystemComponent HandleChangeAbilityCanBeCanceled : %s called"), *RequestingAbility->GetName());
}

FGameplayTag USKAbilitySystemComponent::GetCurrentActiveSkillTag() const
{
	return SKGameplayTags::GetActionSkillTag(this);
}
