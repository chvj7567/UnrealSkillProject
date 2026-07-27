// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class AAIController;
class ACharacter;
class UBlackboardComponent;

namespace SpyAIUtils
{
bool CanMove(AAIController* InAIController);
//# 사망 상태만 판정 — CanMove 는 사망과 Lock_Input_Move 를 함께 묶으므로 둘을 구분해야 할 때 사용
bool IsDead(AAIController* InAIController);
bool CanTargetAttack(ACharacter* InTarget, UBlackboardComponent* InBlackboard, FName TargetKeyName);
}
