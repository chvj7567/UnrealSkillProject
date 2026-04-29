// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_MoveToTarget.h"
#include "SpyAIUtils.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "AITypes.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BTTask_MoveToTarget)

UBTTask_MoveToTarget::UBTTask_MoveToTarget()
{
    NodeName = "Move To Target";
}

EBTNodeResult::Type UBTTask_MoveToTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AIController = OwnerComp.GetAIOwner();
    BlackBoardComp = OwnerComp.GetBlackboardComponent();
    Key = GetSelectedBlackboardKey();

    if (AIController == nullptr || BlackBoardComp == nullptr)
        return EBTNodeResult::Failed;

    Target = Cast<ACharacter>(BlackBoardComp->GetValueAsObject(Key));

    //# 내가 움직일 수 있는 상태인지 확인
    if (SpyAIUtils::CanMove(AIController) == false)
    {
        AIController->StopMovement();
        return EBTNodeResult::Failed;
    }

    //# 타겟을 공격할 수 있는지 확인
    if (SpyAIUtils::CanTargetAttack(Target, BlackBoardComp, Key) == false)
    {
        AIController->StopMovement();
        return EBTNodeResult::Failed;
    }

    //# 타겟을 향해 바라봄
    FVector Direction = Target->GetActorLocation() - AIController->GetPawn()->GetActorLocation();
    Direction.Z = 0.f;

    FRotator TargetRot = FRotationMatrix::MakeFromX(Direction).Rotator();
    AIController->SetControlRotation(TargetRot);
    AIController->GetPawn()->SetActorRotation(TargetRot);

    //# 타겟을 향해 바라봄
    float Distance = FVector::Dist(AIController->GetPawn()->GetActorLocation(), Target->GetActorLocation());
    if (Distance <= StoppingDistance)
    {
        AIController->StopMovement();
        return EBTNodeResult::Succeeded;
    }

    FAIMoveRequest MoveReq(Target);
    MoveReq.SetAcceptanceRadius(StoppingDistance);

    FPathFollowingRequestResult Result = AIController->MoveTo(MoveReq);

    if (Result.Code == EPathFollowingRequestResult::RequestSuccessful)
    {
        WaitForMessage(OwnerComp, BTTaskMessages::MoveFinished);

        return EBTNodeResult::InProgress;
    }
    else if (Result.Code == EPathFollowingRequestResult::AlreadyAtGoal)
    {
        return EBTNodeResult::Succeeded;
    }

    return EBTNodeResult::Failed;
}

