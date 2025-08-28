// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterGameplayAbility.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "SkillProjectCharacter.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemBlueprintLibrary.h"

void UCharacterGameplayAbility::OnMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UCharacterGameplayAbility::OnMontageCancelled()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UCharacterGameplayAbility::OnWaitGameplayEvent(FGameplayEventData Payload)
{
    if (GameplayEffectClass == nullptr)
        return;

    AActor* TargetActor = const_cast<AActor*>(Payload.Target.Get());
    if (TargetActor == nullptr)
        return;

    FGameplayAbilityTargetDataHandle TargetDataHandle = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(TargetActor);

    ApplyGameplayEffectToTarget(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, TargetDataHandle, GameplayEffectClass, GetAbilityLevel());
}

void UCharacterGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    UE_LOG(LogTemp, Warning, TEXT("ActivateAbility called"));

    if (CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        CurrentSpecHandle = Handle;
        CurrentActorInfo = ActorInfo;
        CurrentActivationInfo = ActivationInfo;

        UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, SkillMontage);
        MontageTask->OnCompleted.AddDynamic(this, &UCharacterGameplayAbility::OnMontageCompleted);
        MontageTask->OnInterrupted.AddDynamic(this, &UCharacterGameplayAbility::OnMontageCancelled);
        MontageTask->OnCancelled.AddDynamic(this, &UCharacterGameplayAbility::OnMontageCancelled);

        UAbilityTask_WaitGameplayEvent* WaitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, WaitGameplayTag, nullptr, false, false);
        WaitTask->EventReceived.AddDynamic(this, &UCharacterGameplayAbility::OnWaitGameplayEvent);

        WaitTask->ReadyForActivation();
        MontageTask->ReadyForActivation();
    }
    else
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
    }
}

void UCharacterGameplayAbility::CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility)
{
    Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);

    UE_LOG(LogTemp, Warning, TEXT("CancelAbility called"));
}

bool UCharacterGameplayAbility::CommitAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, OUT FGameplayTagContainer* OptionalRelevantTags)
{
    bool bResult = Super::CommitAbility(Handle, ActorInfo, ActivationInfo, OptionalRelevantTags);

    UE_LOG(LogTemp, Warning, TEXT("CommitAbility called, returning: %s"), bResult ? TEXT("true") : TEXT("false"));

    return bResult;
}

void UCharacterGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

    UE_LOG(LogTemp, Warning, TEXT("EndAbility called, WasCancelled: %s"), bWasCancelled ? TEXT("true") : TEXT("false"));
}

bool UCharacterGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
    bool bResult = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);

    UE_LOG(LogTemp, Warning, TEXT("CanActivateAbility called, returning: %s"), bResult ? TEXT("true") : TEXT("false"));

    return bResult;
}

bool UCharacterGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
    bool bResult = Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);

    UE_LOG(LogTemp, Warning, TEXT("CheckCost called, returning: %s"), bResult ? TEXT("true") : TEXT("false"));

    return bResult;
}

void UCharacterGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
    Super::ApplyCost(Handle, ActorInfo, ActivationInfo);

    UE_LOG(LogTemp, Warning, TEXT("ApplyCost called"));
}

const FGameplayTagContainer* UCharacterGameplayAbility::GetCooldownTags() const
{
    const FGameplayTagContainer* Result = Super::GetCooldownTags();

    UE_LOG(LogTemp, Warning, TEXT("GetCooldownTags called"));

    return Result;
}

void UCharacterGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
    Super::OnGiveAbility(ActorInfo, Spec);

    UE_LOG(LogTemp, Warning, TEXT("OnGiveAbility called"));
}

void UCharacterGameplayAbility::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
    Super::OnRemoveAbility(ActorInfo, Spec);

    UE_LOG(LogTemp, Warning, TEXT("OnRemoveAbility called"));
}

