// Fill out your copyright notice in the Description page of Project Settings.

#include "SpyAIUtils.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Util/SpyGameplayTags.h"

bool SpyAIUtils::CanMove(AAIController* InAIController)
{
    if (!InAIController) return false;

    APawn* Pawn = InAIController->GetPawn();
    if (!Pawn) return false;

    APlayerState* PS = Pawn->GetPlayerState();
    if (!PS) return false;

    UAbilitySystemComponent* ASC = PS->FindComponentByClass<UAbilitySystemComponent>();
    if (!ASC) return false;

    if (ASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Death)) return false;
    if (ASC->HasMatchingGameplayTag(SpyGameplayTags::Lock_Input_Move)) return false;

    return true;
}

bool SpyAIUtils::CanTargetAttack(ACharacter* InTarget, UBlackboardComponent* InBlackboard, FName TargetKeyName)
{
    if (!InTarget) return false;

    APlayerState* PS = InTarget->GetPlayerState();
    if (!PS) return false;

    UAbilitySystemComponent* ASC = PS->FindComponentByClass<UAbilitySystemComponent>();
    if (!ASC) return false;

    if (ASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Death))
    {
        if (InBlackboard)
            InBlackboard->ClearValue(TargetKeyName);
        return false;
    }

    return true;
}
