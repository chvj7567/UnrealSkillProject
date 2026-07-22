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
    //# SetMoveState(true) 가 실제로 실행된 경우에만 내부에서 되돌린다.
    SetMoveState(false);

    //# 인스턴스가 재사용(InstancedPerActor)되어도 다음 활성화가 깨끗한 상태에서 시작하도록 리셋.
    bMoveStateEngaged = false;

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USKGameplayAbility_SkillMove::PlayMontage()
{
    if (AbilityMontage)
    {
        if (UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, AbilityMontage))
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
            }

            if (CapsuleComponent)
            {
                CapsuleComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Ignore);
            }

            //# 이 어빌리티가 이동 상태를 실제로 점유했음을 표시한다.
            bMoveStateEngaged = true;
        }
        else
        {
            //# 켠 적이 없으면 되돌릴 상태도 없다.
            //# (스킬이 성립하지 않아 SetMoveState(true) 없이 종료되는 경로에서 이 가드가 없으면
            //#  공중에 있던 캐릭터가 MOVE_Walking 으로 스냅되고 캡슐 충돌이 강제로 Block 이 된다)
            if (bMoveStateEngaged == false)
            {
                return;
            }

            bMoveStateEngaged = false;

            if (CharacterMovementComponent)
            {
                CharacterMovementComponent->SetMovementMode(EMovementMode::MOVE_Walking);
            }

            if (CapsuleComponent)
            {
                CapsuleComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
            }
        }
    }
}
