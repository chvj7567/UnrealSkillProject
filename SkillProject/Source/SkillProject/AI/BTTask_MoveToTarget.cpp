// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_MoveToTarget.h"
#include "SpyAIUtils.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "AITypes.h"
#include "BrainComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BTTask_MoveToTarget)

UBTTask_MoveToTarget::UBTTask_MoveToTarget()
{
    NodeName = "Move To Target";
    bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_MoveToTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{

    AIController = OwnerComp.GetAIOwner();
    BlackBoardComp = OwnerComp.GetBlackboardComponent();
    Key = GetSelectedBlackboardKey();

    if (AIController == nullptr || BlackBoardComp == nullptr)
        return EBTNodeResult::Failed;

    UObject* RawTarget = BlackBoardComp->GetValueAsObject(Key);
    Target = Cast<ACharacter>(RawTarget);

    if (RawTarget == nullptr)
    {
        const FString BotName = AIController->GetPawn() ? AIController->GetPawn()->GetName() : AIController->GetName();
        UE_LOG(LogTemp, Warning, TEXT("[SpyAI %s] MoveToTarget ExecuteTask: BB.%s = NULL"), *BotName, *Key.ToString());
        return EBTNodeResult::Failed;
    }

    if (!SpyAIUtils::CanMove(AIController))
    {
        AIController->StopMovement();
        return EBTNodeResult::Failed;
    }

    if (!SpyAIUtils::CanTargetAttack(Target, BlackBoardComp, Key))
    {
        AIController->StopMovement();
        return EBTNodeResult::Failed;
    }

    //# 타겟을 향해 즉시 회전
    FVector Direction = Target->GetActorLocation() - AIController->GetPawn()->GetActorLocation();
    Direction.Z = 0.f;
    if (!Direction.IsNearlyZero())
    {
        const FRotator TargetRot = FRotationMatrix::MakeFromX(Direction).Rotator();
        AIController->SetControlRotation(TargetRot);
        AIController->GetPawn()->SetActorRotation(TargetRot);
    }

    const float Distance = FVector::Dist(AIController->GetPawn()->GetActorLocation(), Target->GetActorLocation());
    if (Distance <= StoppingDistance)
    {
        AIController->StopMovement();
        return EBTNodeResult::Succeeded;
    }

    FAIMoveRequest MoveReq(Target);
    MoveReq.SetAcceptanceRadius(StoppingDistance);
    //# UE는 기본적으로 acceptance에 agent/goal radius를 합산함 (capsule 34 + 34 = 68 추가).
    //# StopDist를 정확히 GA 사거리와 맞추려면 이 합산을 끔.
    MoveReq.SetReachTestIncludesAgentRadius(false);
    MoveReq.SetReachTestIncludesGoalRadius(false);
    MoveReq.SetUsePathfinding(true);

    const FPathFollowingRequestResult Result = AIController->MoveTo(MoveReq);

    //# 진단: MoveTo 직후 PFC가 Moving 상태로 진입했는지
    EPathFollowingStatus::Type PostStatus = EPathFollowingStatus::Idle;
    if (UPathFollowingComponent* PFC = AIController->GetPathFollowingComponent())
    {
        PostStatus = PFC->GetStatus();
    }
    const FString BotName = AIController->GetPawn() ? AIController->GetPawn()->GetName() : AIController->GetName();
    UE_LOG(LogTemp, Warning, TEXT("[SpyAI %s] MoveTo: Code=%d Dist=%.0f StopDist=%.0f PostStatus=%d"),
        *BotName, (int32)Result.Code, Distance, StoppingDistance, (int32)PostStatus);

    if (Result.Code == EPathFollowingRequestResult::RequestSuccessful)
    {
        return EBTNodeResult::InProgress;
    }
    if (Result.Code == EPathFollowingRequestResult::AlreadyAtGoal)
    {
        return EBTNodeResult::Succeeded;
    }
    return EBTNodeResult::Failed;
}

EBTNodeResult::Type UBTTask_MoveToTarget::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    if (AAIController* AIC = OwnerComp.GetAIOwner())
    {
        AIC->StopMovement();
    }
    return Super::AbortTask(OwnerComp, NodeMemory);
}

void UBTTask_MoveToTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

    AAIController* AIC = OwnerComp.GetAIOwner();
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!IsValid(AIC) || !IsValid(BB))
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    APawn* MyPawn = AIC->GetPawn();
    ACharacter* CurTarget = Cast<ACharacter>(BB->GetValueAsObject(GetSelectedBlackboardKey()));
    if (!IsValid(MyPawn) || !IsValid(CurTarget))
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    //# 이동 차단 상태(Lock_Input_Move 등)면 중단
    if (!SpyAIUtils::CanMove(AIC) || !SpyAIUtils::CanTargetAttack(CurTarget, BB, GetSelectedBlackboardKey()))
    {
        AIC->StopMovement();
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    const float CurDist = FVector::Dist(MyPawn->GetActorLocation(), CurTarget->GetActorLocation());
    if (CurDist <= StoppingDistance)
    {
        const FString TickBotName = MyPawn ? MyPawn->GetName() : AIC->GetName();
        UE_LOG(LogTemp, Warning, TEXT("[SpyAI %s] MoveToTarget: Succeeded (Dist=%.0f ≤ %.0f) → ActivateAbility로"), *TickBotName, CurDist, StoppingDistance);
        AIC->StopMovement();
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }

    //# 0.3초마다 강제 repath — MoveTo는 자동 repath를 안 하므로 타겟이 움직이면 따라잡지 못함
    static float RepathAccum = 0.f;
    RepathAccum += DeltaSeconds;
    if (RepathAccum >= 0.3f)
    {
        RepathAccum = 0.f;

        FAIMoveRequest MoveReq(CurTarget);
        MoveReq.SetAcceptanceRadius(StoppingDistance);
        MoveReq.SetReachTestIncludesAgentRadius(false);
        MoveReq.SetReachTestIncludesGoalRadius(false);
        MoveReq.SetUsePathfinding(true);
        const FPathFollowingRequestResult Result = AIC->MoveTo(MoveReq);

        if (Result.Code == EPathFollowingRequestResult::AlreadyAtGoal)
        {
            AIC->StopMovement();
            FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
            return;
        }
    }
}

