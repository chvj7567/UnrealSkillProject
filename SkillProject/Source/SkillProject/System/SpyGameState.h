// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ModularGameState.h"

#include "SpyGameState.generated.h"

class USpySpawnBotManagerComponent;

UCLASS()
class SKILLPROJECT_API ASpyGameState : public AModularGameStateBase
{
	GENERATED_BODY()
	
public:
	ASpyGameState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USpySpawnBotManagerComponent> BotSpawnerComponent;
};
