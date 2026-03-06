// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyGameplayAbility_SkillAction.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "SKGameplayTags.h"
#include "Util/SpyGameplayTags.h"
#include "System/SpyPlayerState.h"
#include "GameFramework/Character.h"
#include "Data/SpyCharacterAssetData.h"

bool USpyGameplayAbility_SkillAction::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
    // 1. 부모 클래스의 체크 결과 확인
    bool bCanActivate = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);

    if (!bCanActivate)
    {
        // 2. 실패했다면 원인이 무엇인지 OptionalRelevantTags를 통해 확인 가능하지만,
        // 간단하게 로그를 찍어 현재 내 태그 상태를 확인합니다.
        FGameplayTagContainer OwnedTags;
        ActorInfo->AbilitySystemComponent->GetOwnedGameplayTags(OwnedTags);

        UE_LOG(LogTemp, Warning, TEXT("[CanActivateAbility Failed] Ability: %s"), *GetName());
        UE_LOG(LogTemp, Warning, TEXT("Current Owned Tags: %s"), *OwnedTags.ToString());
    }

    return bCanActivate;
}

void USpyGameplayAbility_SkillAction::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    USpyAbilitySystemComponent* OwnerASC = Cast<USpyAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get());
    if (OwnerCharacter == nullptr || OwnerASC == nullptr)
    {
        Super::EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void USpyGameplayAbility_SkillAction::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (OwnerCharacter == nullptr)
        return;

    ASpyPlayerState* OwnerPS = OwnerCharacter->GetPlayerState<ASpyPlayerState>();
    USpyAbilitySystemComponent* OwnerASC = Cast<USpyAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get());
    if (OwnerPS == nullptr || OwnerASC == nullptr)
        return;

    if (OwnerASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Combo))
    {
        FGameplayTag MyTag = AbilityTags.GetByIndex(0);
        if (MyTag.IsValid() == false)
            return;
        
        int NextComboStep = OwnerPS->GetComboStep() + 1;
        if (USpyCharacterAssetData* CharacterAssetData = OwnerPS->GetCharacterAssetData())
        {
            FGameplayTag Tag = CharacterAssetData->GetComboTag(SpyGameplayTags::Character_Class_Normal, NextComboStep);
            if (Tag.IsValid())
            {
                OwnerPS->AddComboStep();

                FGameplayEventData Payload;
                Payload.EventTag = Tag;

                UE_LOG(LogTemp, Warning, TEXT("# Combo %d %s"), NextComboStep, *Tag.ToString());
                OwnerASC->HandleGameplayEvent(Payload.EventTag, &Payload);
                return;
            }
        }
    }
}

void USpyGameplayAbility_SkillAction::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
    {
        if (ASpyPlayerState* OwnerPS = OwnerCharacter->GetPlayerState<ASpyPlayerState>())
        {
            OwnerPS->InitComboStep();
        }
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
