#include "EnvQueryGenerator_ArcAwayFromTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnvQueryGenerator_ArcAwayFromTarget)

UEnvQueryGenerator_ArcAwayFromTarget::UEnvQueryGenerator_ArcAwayFromTarget()
{
	ItemType = UEnvQueryItemType_Point::StaticClass();
}

void UEnvQueryGenerator_ArcAwayFromTarget::GenerateItems(FEnvQueryInstance& QueryInstance) const
{
	APawn* QuerierPawn = Cast<APawn>(QueryInstance.Owner.Get());
	if (!IsValid(QuerierPawn))
	{
		return;
	}

	AAIController* AIController = Cast<AAIController>(QuerierPawn->GetController());
	if (!IsValid(AIController))
	{
		return;
	}

	UBlackboardComponent* BB = AIController->GetBlackboardComponent();
	if (!IsValid(BB))
	{
		return;
	}

	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(TargetKeyName));
	if (!IsValid(TargetActor))
	{
		return;
	}

	const FVector QuerierLoc = QuerierPawn->GetActorLocation();
	const FVector TargetLoc = TargetActor->GetActorLocation();

	//# 타겟 → Querier 방향 = 타겟 반대 방향
	FVector AwayDir = QuerierLoc - TargetLoc;
	AwayDir.Z = 0.f;
	if (!AwayDir.Normalize())
	{
		return;
	}

	const int32 ClampedNum = FMath::Max(1, NumPoints);
	TArray<FNavLocation> NavLocations;
	NavLocations.Reserve(ClampedNum);

	if (ClampedNum == 1)
	{
		NavLocations.Add(FNavLocation(QuerierLoc + AwayDir * Radius));
	}
	else
	{
		const float HalfArc = ArcAngleDegrees * 0.5f;
		const float Step = ArcAngleDegrees / static_cast<float>(ClampedNum - 1);
		for (int32 i = 0; i < ClampedNum; ++i)
		{
			const float Angle = -HalfArc + Step * static_cast<float>(i);
			const FVector Dir = FRotator(0.f, Angle, 0.f).RotateVector(AwayDir);
			NavLocations.Add(FNavLocation(QuerierLoc + Dir * Radius));
		}
	}

	ProjectAndFilterNavPoints(NavLocations, QueryInstance);
	StoreNavPoints(NavLocations, QueryInstance);
}
