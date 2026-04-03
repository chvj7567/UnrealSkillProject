// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyGameplayAbility_SkillAction.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "SKGameplayTags.h"
#include "Util/SpyGameplayTags.h"
#include "System/SpyPlayerState.h"
#include "GameFramework/Character.h"
#include "Data/SpyCharacterAssetData.h"
#include "ManagerComponent/SpyTargetingManagerComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGameplayAbility_SkillAction)

void USpyGameplayAbility_SkillAction::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    do
    {
        /*ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
        if (OwnerCharacter == nullptr)
            break;

        USpyTargetingManagerComponent* TargetingComp = OwnerCharacter->FindComponentByClass<USpyTargetingManagerComponent>();
        if (TargetingComp == nullptr)
            break;

        if (TargetingComp->GetTarget().IsValid())
        {
            FVector LookDir = TargetingComp->GetTarget()->GetActorLocation() - OwnerCharacter->GetActorLocation();
            LookDir.Z = 0.f;
            FRotator TargetRot = LookDir.Rotation();

            OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
            OwnerCharacter->SetActorRotation(TargetRot);
            OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = true;
        }*/

    } while (false);

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

    //# 콤보 가능 태그 있는지 확인
    if (OwnerASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Combo))
    {
        FGameplayTag MyTag = AbilityTags.GetByIndex(0);
        if (MyTag.IsValid() == false)
            return;
        
        if (USpyCharacterAssetData* CharacterAssetData = OwnerPS->GetCharacterAssetData())
        {
            FGameplayTag Tag = CharacterAssetData->GetComboTag(SpyGameplayTags::Character_Class_Normal, MyTag);
            if (Tag.IsValid())
            {
                FGameplayEventData Payload;
                Payload.EventTag = Tag;

                UE_LOG(LogTemp, Warning, TEXT("# Combo %s"), *Tag.ToString());
                OwnerASC->HandleGameplayEvent(Payload.EventTag, &Payload);
                return;
            }
        }
    }
}
