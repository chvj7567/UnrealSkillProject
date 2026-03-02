// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyGameplayAbility_SkillAction.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "SKGameplayTags.h"
#include "Util/SpyGameplayTags.h"
#include "System/SpyPlayerState.h"
#include "GameFramework/Character.h"

void USpyGameplayAbility_SkillAction::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    USpyAbilitySystemComponent* OwnerASC = Cast<USpyAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get());
    if (OwnerCharacter == nullptr || OwnerASC == nullptr)
        return;

    //# 콤보 중이면 트리거로 GA 실행함
    if (OwnerASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Combo))
        return;

    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void USpyGameplayAbility_SkillAction::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    ASpyPlayerState* OwnerPS = OwnerCharacter->GetPlayerState<ASpyPlayerState>();
    USpyAbilitySystemComponent* OwnerASC = Cast<USpyAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get());
    if (OwnerCharacter == nullptr || OwnerPS == nullptr || OwnerASC == nullptr)
        return;

    if (OwnerASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Combo))
    {
        int32 BeforeComboStep = OwnerPS->GetComboStep();
        int32 AfterComboStep = OwnerPS->AddOrGetComboStep();
        if (BeforeComboStep == 0)
        {
            FGameplayEventData Payload;
            Payload.EventTag = SpyGameplayTags::Skill_Action_B;

            UE_LOG(LogTemp, Warning, TEXT("# Combo Skill_Action_B"));
            OwnerASC->HandleGameplayEvent(Payload.EventTag, &Payload);
        }
    }
}
