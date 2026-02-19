// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_MoveToRange.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerState.h"
#include "Util/SpyGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BTTask_MoveToRange)

UBTTask_MoveToRange::UBTTask_MoveToRange()
{
    NodeName = "Move To Range";
}

EBTNodeResult::Type UBTTask_MoveToRange::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    UBlackboardComponent* BlackBoardComp = OwnerComp.GetBlackboardComponent();

    if (AIController == nullptr || BlackBoardComp == nullptr)
        return EBTNodeResult::Failed;

    AActor* TargetActor = Cast<AActor>(BlackBoardComp->GetValueAsObject(GetSelectedBlackboardKey()));
    if (TargetActor == nullptr)
        return EBTNodeResult::Failed;

    //# 움직일 수 있는 상태인지 확인
    if (CanMove(AIController) == false)
        return EBTNodeResult::Failed;

    AIController->MoveToActor(TargetActor, StoppingDistance);

    return Super::ExecuteTask(OwnerComp, NodeMemory);
}

bool UBTTask_MoveToRange::CanMove(AAIController* AIController)
{
    APawn* Pawn = AIController->GetPawn();
    if (Pawn == nullptr)
        return false;

    APlayerState* PS = Pawn->GetPlayerState();
    if (PS == nullptr)
        return false;

    UAbilitySystemComponent* ASC = PS->FindComponentByClass<UAbilitySystemComponent>();
    if (ASC == nullptr)
        return false;

    if (ASC->HasMatchingGameplayTag(SpyGameplayTags::Lock_Input_Move))
        return false;

    return true;
}
