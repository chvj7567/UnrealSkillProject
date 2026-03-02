// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_MoveToRange.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerState.h"
#include "Util/SpyGameplayTags.h"
#include "Navigation/PathFollowingComponent.h"
#include "AITypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BTTask_MoveToRange)

UBTTask_MoveToRange::UBTTask_MoveToRange()
{
    NodeName = "Move To Range";
}

EBTNodeResult::Type UBTTask_MoveToRange::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    UBlackboardComponent* BlackBoardComp = OwnerComp.GetBlackboardComponent();
    FName Key = GetSelectedBlackboardKey();

    if (AIController == nullptr || BlackBoardComp == nullptr)
        return EBTNodeResult::Failed;

    AActor* TargetActor = Cast<AActor>(BlackBoardComp->GetValueAsObject(Key));
    if (TargetActor == nullptr)
        return EBTNodeResult::Failed;

    //# 움직일 수 있는 상태인지 확인
    if (CanMove(AIController) == false)
    {
        AIController->StopMovement();
        return EBTNodeResult::Failed;
    }

    //# 타겟을 향해 바라봄
    FVector Direction = TargetActor->GetActorLocation() - AIController->GetPawn()->GetActorLocation();
    Direction.Z = 0.f;

    FRotator TargetRot = FRotationMatrix::MakeFromX(Direction).Rotator();
    AIController->SetControlRotation(TargetRot);
    AIController->GetPawn()->SetActorRotation(TargetRot);

    //# 타겟과의 거리 확인
    float Distance = FVector::Dist(AIController->GetPawn()->GetActorLocation(), TargetActor->GetActorLocation());
    if (Distance <= StoppingDistance)
    {
        AIController->StopMovement();
        return EBTNodeResult::Succeeded;
    }

    FAIMoveRequest MoveReq(TargetActor);
    MoveReq.SetAcceptanceRadius(StoppingDistance);

    FPathFollowingRequestResult Result = AIController->MoveTo(MoveReq);

    if (Result.Code == EPathFollowingRequestResult::RequestSuccessful)
    {
        const FName MoveFinishedMessage = TEXT("MoveFinished");
        WaitForMessage(OwnerComp, MoveFinishedMessage);

        return EBTNodeResult::InProgress;
    }
    else if (Result.Code == EPathFollowingRequestResult::AlreadyAtGoal)
    {
        return EBTNodeResult::Succeeded;
    }

    return EBTNodeResult::Failed;
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
