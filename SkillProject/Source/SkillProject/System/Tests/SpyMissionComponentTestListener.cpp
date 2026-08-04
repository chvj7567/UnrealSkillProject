// Fill out your copyright notice in the Description page of Project Settings.

#include "System/Tests/SpyMissionComponentTestListener.h"

void USpyMissionComponentTestListener::HandleMissionAccepted(USpyMissionComponent* MissionComponent, int32 MissionIndex)
{
	++AcceptedCallCount;
	LastAcceptedIndex = MissionIndex;
	AcceptedOrder = NextOrder++;
}

void USpyMissionComponentTestListener::HandleMissionCompleted(USpyMissionComponent* MissionComponent, int32 CompletedIndex)
{
	++CompletedCallCount;
	LastCompletedIndex = CompletedIndex;
	CompletedOrder = NextOrder++;
}
