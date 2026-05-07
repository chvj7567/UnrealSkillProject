#include "EnvQueryContext_TargetActor.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnvQueryContext_TargetActor)

void UEnvQueryContext_TargetActor::ProvideContext(FEnvQueryInstance& QueryInstance,
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

	UEnvQueryItemType_Actor::SetContextHelper(ContextData, TargetActor);
}
