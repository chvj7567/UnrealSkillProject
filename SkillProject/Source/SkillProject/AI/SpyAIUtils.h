// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class AAIController;
class ACharacter;
class UBlackboardComponent;

namespace SpyAIUtils
{
    bool CanMove(AAIController* InAIController);
    bool CanTargetAttack(ACharacter* InTarget, UBlackboardComponent* InBlackboard, FName TargetKeyName);
}
