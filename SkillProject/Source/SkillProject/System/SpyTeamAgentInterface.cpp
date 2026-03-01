#include "SpyTeamAgentInterface.h"
#include "UObject/ScriptInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyTeamAgentInterface)

USpyTeamAgentInterface::USpyTeamAgentInterface(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
}

ETeamAttitude::Type ISpyTeamAgentInterface::GetTeamAttitudeTowards(const AActor& Other) const
{
	const APawn* OtherPawn = Cast<APawn>(&Other);
	if (OtherPawn == nullptr)
		return ETeamAttitude::Neutral;

	const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(OtherPawn->GetController());
	if (TeamAgent == nullptr)
		return ETeamAttitude::Neutral;

	if (TeamAgent->GetGenericTeamId() == GetGenericTeamId())
	{
		return ETeamAttitude::Friendly;
	}
	else
	{
		return ETeamAttitude::Hostile;
	}
}
