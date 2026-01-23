// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ModularGameMode.h"

#include "SpyGameMode.generated.h"

UCLASS(minimalapi)
class ASpyGameMode : public AModularGameModeBase
{
	GENERATED_BODY()

public:
	ASpyGameMode();

protected:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
};



