// Fill out your copyright notice in the Description page of Project Settings.

#include "Ability/SKGameplayAbility.h"
#include "SKAbilitySourceInterface.h"
#include "SKGameplayEffectContext.h"
#include "SKAbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKGameplayAbility)

void USKGameplayAbility::OnMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USKGameplayAbility::OnMontageInterrupted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USKGameplayAbility::OnMontageCancelled()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USKGameplayAbility::OnMontageBlendOut()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USKGameplayAbility::OnWaitGameplayEvent(FGameplayEventData Payload)
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USKGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    CurrentSpecHandle = Handle;
    CurrentActorInfo = ActorInfo;
    CurrentActivationInfo = ActivationInfo;

    if (FGameplayAbilitySpec* Spec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle))
    {
        UE_LOG(LogTemp, Warning, TEXT("# [SKGameplayAbility] ActivateAbility %s"), *Spec->Ability->AbilityTags.ToString());
    }
}

void USKGameplayAbility::CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility)
{
    Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);

    if (FGameplayAbilitySpec* Spec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle))
    {
        UE_LOG(LogTemp, Warning, TEXT("# [SKGameplayAbility] CancelAbility %s"), *Spec->Ability->AbilityTags.ToString());
    }
}

bool USKGameplayAbility::CommitAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, OUT FGameplayTagContainer* OptionalRelevantTags)
{
    bool bResult = Super::CommitAbility(Handle, ActorInfo, ActivationInfo, OptionalRelevantTags);

    if (FGameplayAbilitySpec* Spec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle))
    {
        UE_LOG(LogTemp, Warning, TEXT("# [SKGameplayAbility] CommitAbility %s"), *Spec->Ability->AbilityTags.ToString());
    }

    return bResult;
}

void USKGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

    if (FGameplayAbilitySpec* Spec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle))
    {
        UE_LOG(LogTemp, Warning, TEXT("# [SKGameplayAbility] EndAbility %s"), *Spec->Ability->AbilityTags.ToString());
    }
}

bool USKGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
    bool bResult = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);

    if (FGameplayAbilitySpec* Spec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle))
    {
        UE_LOG(LogTemp, Warning, TEXT("# [SKGameplayAbility] CanActivateAbility %s"), *Spec->Ability->AbilityTags.ToString());
    }

    return bResult;
}

bool USKGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
    bool bResult = Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);

    return bResult;
}

void USKGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
    Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
}

const FGameplayTagContainer* USKGameplayAbility::GetCooldownTags() const
{
    const FGameplayTagContainer* Result = Super::GetCooldownTags();

    return Result;
}

void USKGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
    Super::OnGiveAbility(ActorInfo, Spec);
}

void USKGameplayAbility::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
    Super::OnRemoveAbility(ActorInfo, Spec);
}

FGameplayEffectContextHandle USKGameplayAbility::MakeEffectContext(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
    FGameplayEffectContextHandle ContextHandle = Super::MakeEffectContext(Handle, ActorInfo);

    FSKGameplayEffectContext* EffectContext = FSKGameplayEffectContext::ExtractEffectContext(ContextHandle);
    check(EffectContext);
    check(ActorInfo);

    //# Default
    AActor* Instigator = ActorInfo->OwnerActor.Get();
    AActor* EffectCauser = ActorInfo->AvatarActor.Get();
    UObject* SourceObject = GetSourceObject(Handle, ActorInfo);

    EffectContext->AddInstigator(Instigator, EffectCauser);
    EffectContext->AddSourceObject(SourceObject);

    //# Custom
    ISKAbilitySourceInterface* AbilitySource = Cast<ISKAbilitySourceInterface>(SourceObject);

    EffectContext->SetAbilitySource(AbilitySource, 0.0f);

    return ContextHandle;
}