// Fill out your copyright notice in the Description page of Project Settings.


#include "Ability/SKGameplayAbility_SkillMove.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "SKGameplayEffectContext.h"
#include "SKAbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKGameplayAbility_SkillMove)

USKGameplayAbility_SkillMove::USKGameplayAbility_SkillMove()
{
    bIsCollide = false;
}

void USKGameplayAbility_SkillMove::OnMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USKGameplayAbility_SkillMove::OnMontageCancelled()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void USKGameplayAbility_SkillMove::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    CurrentSpecHandle = Handle;
    CurrentActorInfo = ActorInfo;
    CurrentActivationInfo = ActivationInfo;

    if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
    {
        CharacterMovementComponent = OwnerCharacter->GetCharacterMovement();
        CapsuleComponent = OwnerCharacter->GetCapsuleComponent();
    }
}

void USKGameplayAbility_SkillMove::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    SetMoveState(false);

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USKGameplayAbility_SkillMove::PlayMontage()
{
    if (SkillMontage)
    {
        if (UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, SkillMontage))
        {
            MontageTask->OnCompleted.AddDynamic(this, &USKGameplayAbility_SkillMove::OnMontageCompleted);
            MontageTask->OnInterrupted.AddDynamic(this, &USKGameplayAbility_SkillMove::OnMontageCancelled);
            MontageTask->OnCancelled.AddDynamic(this, &USKGameplayAbility_SkillMove::OnMontageCancelled);
            MontageTask->ReadyForActivation();
        }
    }
}

void USKGameplayAbility_SkillMove::SetMoveState(bool bActive)
{
    if (bIsCollide == false)
    {
        if (bActive)
        {
            if (CharacterMovementComponent)
            {
                CharacterMovementComponent->SetMovementMode(EMovementMode::MOVE_Flying);
                CharacterMovementComponent->bIgnoreClientMovementErrorChecksAndCorrection = true;
            }

            if (CapsuleComponent)
            {
                CapsuleComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Ignore);
            }
        }
        else
        {
            if (CharacterMovementComponent)
            {
                CharacterMovementComponent->SetMovementMode(EMovementMode::MOVE_Walking);
                CharacterMovementComponent->bIgnoreClientMovementErrorChecksAndCorrection = false;
            }

            if (CapsuleComponent)
            {
                CapsuleComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
            }
        }
    }
}
