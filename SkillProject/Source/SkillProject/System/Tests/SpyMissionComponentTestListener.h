// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "SpyMissionComponentTestListener.generated.h"

class USpyMissionComponent;

//# OnMissionAccepted/OnMissionCompleted 발화 횟수·인자를 기록하는 테스트 전용 리스너.
//# 동적 멀티캐스트 델리게이트는 UFUNCTION 바인딩 대상이 필요해 순수 람다로 검증할 수 없다.
UCLASS()
class USpyMissionComponentTestListener : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void HandleMissionAccepted(USpyMissionComponent* MissionComponent, int32 MissionIndex);

	UFUNCTION()
	void HandleMissionCompleted(USpyMissionComponent* MissionComponent, int32 CompletedIndex);

	int32 AcceptedCallCount = 0;
	int32 LastAcceptedIndex = -1;
	int32 AcceptedOrder = -1; //# NextOrder 스냅샷 — Completed 와의 상대 순서를 검증하기 위함(§2-3 순서 고정 회귀)

	int32 CompletedCallCount = 0;
	int32 LastCompletedIndex = -1;
	int32 CompletedOrder = -1;

	int32 NextOrder = 0;
};
