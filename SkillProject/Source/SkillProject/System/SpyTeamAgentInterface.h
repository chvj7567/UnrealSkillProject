#pragma once

#include "GenericTeamAgentInterface.h"
#include "UObject/Object.h"
#include "UObject/WeakObjectPtr.h"
#include "SpyTeamAgentInterface.generated.h"

UINTERFACE(meta=(CannotImplementInterfaceInBlueprint))
class USpyTeamAgentInterface : public UGenericTeamAgentInterface
{
	GENERATED_UINTERFACE_BODY()
};

class SKILLPROJECT_API ISpyTeamAgentInterface : public IGenericTeamAgentInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

};
