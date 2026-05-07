#include "EnvQueryContext_StrafeDirection.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnvQueryContext_StrafeDirection)

void UEnvQueryContext_StrafeDirection::ProvideContext(FEnvQueryInstance& QueryInstance,
                                                      FEnvQueryContextData& ContextData) const
{
    APawn* QuerierPawn = Cast<APawn>(QueryInstance.Owner.Get());
    if (IsValid(QuerierPawn) == false)
    {
        return;
    }

    AAIController* AIController = Cast<AAIController>(QuerierPawn->GetController());
    if (IsValid(AIController) == false)
    {
        return;
    }

    UBlackboardComponent* BB = AIController->GetBlackboardComponent();
    if (IsValid(BB) == false)
    {
        return;
    }

    AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(TargetKeyName));
    if (IsValid(TargetActor) == false)
    {
        return;
    }

    const bool bStrafeLeft = BB->GetValueAsBool(StrafeLeftKeyName);

    const FVector QuerierLoc = QuerierPawn->GetActorLocation();
    const FVector TargetLoc  = TargetActor->GetActorLocation();

    FVector Forward = (TargetLoc - QuerierLoc).GetSafeNormal2D();
    if (Forward.IsNearlyZero())
    {
        return;
    }

    FVector RightVec  = FVector::CrossProduct(FVector::UpVector, Forward);
    FVector StrafeDir = bStrafeLeft ? -RightVec : RightVec;

    FVector ContextLocation = QuerierLoc + StrafeDir * ContextOffset;

    UEnvQueryItemType_Point::SetContextHelper(ContextData, ContextLocation);
}
