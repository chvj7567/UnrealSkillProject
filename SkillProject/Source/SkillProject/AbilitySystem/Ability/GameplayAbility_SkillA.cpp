// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Ability/GameplayAbility_SkillA.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Character/SkillProjectCharacter.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemBlueprintLibrary.h"

void UGameplayAbility_SkillA::OnMontageCompleted()
{
    Super::OnMontageCompleted();

    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGameplayAbility_SkillA::OnMontageCancelled()
{
    Super::OnMontageCancelled();

    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGameplayAbility_SkillA::OnWaitGameplayEvent(FGameplayEventData Payload)
{
    Super::OnWaitGameplayEvent(Payload);

    if (GameplayEffectClass == nullptr)
        return;

    AActor* TargetActor = const_cast<AActor*>(Payload.Target.Get());
    if (TargetActor == nullptr)
        return;

    FGameplayAbilityTargetDataHandle TargetDataHandle = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(TargetActor);

    ApplyGameplayEffectToTarget(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, TargetDataHandle, GameplayEffectClass, GetAbilityLevel());
}

void UGameplayAbility_SkillA::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    UE_LOG(LogTemp, Warning, TEXT("ActivateAbility called"));

    if (CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        CurrentSpecHandle = Handle;
        CurrentActorInfo = ActorInfo;
        CurrentActivationInfo = ActivationInfo;

        if (UAbilityTask_WaitGameplayEvent* WaitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, WaitGameplayTag, nullptr, false, false))
        {
            WaitTask->EventReceived.AddDynamic(this, &UGameplayAbility_SkillA::OnWaitGameplayEvent);
            WaitTask->ReadyForActivation();
        }

        if (UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, SkillMontage))
        {
            MontageTask->OnCompleted.AddDynamic(this, &UGameplayAbility_SkillA::OnMontageCompleted);
            MontageTask->OnInterrupted.AddDynamic(this, &UGameplayAbility_SkillA::OnMontageCancelled);
            MontageTask->OnCancelled.AddDynamic(this, &UGameplayAbility_SkillA::OnMontageCancelled);
            MontageTask->ReadyForActivation();
        }
    }
    else
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
    }
}