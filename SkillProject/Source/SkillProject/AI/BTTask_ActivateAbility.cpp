// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_ActivateAbility.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemComponent.h"

UBTTask_ActivateAbility::UBTTask_ActivateAbility()
{
    NodeName = "Activate Ability";
}

EBTNodeResult::Type UBTTask_ActivateAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (AIController == nullptr)
        return EBTNodeResult::Failed;

    APawn* Pawn = AIController->GetPawn();
    if (Pawn == nullptr)
        return EBTNodeResult::Failed;

    APlayerState* PS = Pawn->GetPlayerState();
    if (PS == nullptr)
        return EBTNodeResult::Failed;

    UAbilitySystemComponent* ASC = PS->FindComponentByClass<UAbilitySystemComponent>();
    if (ASC == nullptr)
        return EBTNodeResult::Failed;

    if (AbilityTag.IsValid() == false)
        return EBTNodeResult::Failed;

    FGameplayTagContainer TagContainer;
    TagContainer.AddTag(AbilityTag);

    bool bActivated = ASC->TryActivateAbilitiesByTag(TagContainer);

    return bActivated ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}
