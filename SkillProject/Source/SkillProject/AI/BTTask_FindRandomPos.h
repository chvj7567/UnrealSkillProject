// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"

#include "BTTask_FindRandomPos.generated.h"

UCLASS()
class SKILLPROJECT_API UBTTask_FindRandomPos : public UBTTaskNode
{
	GENERATED_BODY()

public:
    UBTTask_FindRandomPos();

protected:
    virtual EBTNodeResult::Type ExecuteTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory) override;

    UPROPERTY(EditAnywhere, Category = "AI")
    float SearchRadius = 1000.f;

    UPROPERTY(EditAnywhere, Category = "AI")
    FBlackboardKeySelector TargetLocationKey;
};
