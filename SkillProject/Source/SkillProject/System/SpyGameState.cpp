// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyGameState.h"
#include "ManagerComponent/SpySpawnBotManagerComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGameState)

ASpyGameState::ASpyGameState()
{
	BotSpawnerComponent = CreateDefaultSubobject<USpySpawnBotManagerComponent>(TEXT("SpySpawnBotManagerComponent"));
}
