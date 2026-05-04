#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_TargetActor.generated.h"

//# Querier(AI)의 Blackboard.TargetActor를 EQS Context로 제공한다.
//# Generator의 Line From/Line To, Center, Test의 Distance 등에서 사용 가능.
UCLASS()
class SKILLPROJECT_API UEnvQueryContext_TargetActor : public UEnvQueryContext
{
	GENERATED_BODY()

public:
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance,
	                            FEnvQueryContextData& ContextData) const override;

	UPROPERTY(EditDefaultsOnly, Category = "EQS")
	FName TargetKeyName = TEXT("TargetActor");
};
