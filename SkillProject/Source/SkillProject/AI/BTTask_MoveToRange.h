// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "BTTask_MoveToRange.generated.h"

UCLASS()
class SKILLPROJECT_API UBTTask_MoveToRange : public UBTTask_MoveTo
{
	GENERATED_BODY()
	
public:
	UBTTask_MoveToRange();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	bool CanMove(AAIController* AIController);

protected:
	UPROPERTY(EditAnywhere, Category = "Config")
	float StoppingDistance = 200.f;
};
