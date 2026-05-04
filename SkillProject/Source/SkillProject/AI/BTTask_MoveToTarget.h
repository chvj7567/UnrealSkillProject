// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "BTTask_MoveToTarget.generated.h"

namespace BTTaskMessages
{
	inline const FName MoveFinished = TEXT("MoveFinished");
}

UCLASS()
class SKILLPROJECT_API UBTTask_MoveToTarget : public UBTTask_MoveTo
{
	GENERATED_BODY()
	
public:
	UBTTask_MoveToTarget();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Config")
	float StoppingDistance = 200.f;

protected:
	UPROPERTY()
	TObjectPtr<AAIController> AIController;

	UPROPERTY()
	TObjectPtr<UBlackboardComponent> BlackBoardComp;
	
	UPROPERTY()
	FName Key;

	UPROPERTY()
	ACharacter* Target;
};
