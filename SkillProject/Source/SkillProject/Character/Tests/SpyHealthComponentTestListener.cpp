// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/Tests/SpyHealthComponentTestListener.h"

void USpyHealthComponentTestListener::HandleDeath(AActor* InOwningActor, AActor* InCauserActor)
{
	++DeathCallCount;
	LastOwningActor = InOwningActor;
	LastCauserActor = InCauserActor;
}
