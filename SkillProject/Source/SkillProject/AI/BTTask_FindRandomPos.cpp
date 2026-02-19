// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_FindRandomPos.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BTTask_FindRandomPos)

UBTTask_FindRandomPos::UBTTask_FindRandomPos()
{
    NodeName = "Find Random Position";

    TargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindRandomPos, TargetLocationKey));
}

EBTNodeResult::Type UBTTask_FindRandomPos::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (AIController == nullptr)
        return EBTNodeResult::Failed;

    APawn* Pawn = AIController->GetPawn();
    if (Pawn == nullptr)
        return EBTNodeResult::Failed;

    UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(Pawn->GetWorld());
    if (NavSystem == nullptr)
        return EBTNodeResult::Failed;

    FNavLocation RandomLocation;

    bool bSuccess = NavSystem->GetRandomReachablePointInRadius(
        Pawn->GetActorLocation(),
        SearchRadius,
        RandomLocation);

    if (bSuccess == false)
        return EBTNodeResult::Failed;

    OwnerComp.GetBlackboardComponent()->SetValueAsVector(
        TargetLocationKey.SelectedKeyName,
        RandomLocation.Location);

    return EBTNodeResult::Succeeded;
}