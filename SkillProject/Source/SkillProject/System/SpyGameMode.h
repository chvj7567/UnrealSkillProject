// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ModularGameMode.h"

#include "SpyGameMode.generated.h"

class USpyCharacterAssetData;

UCLASS(minimalapi)
class ASpyGameMode : public AModularGameModeBase
{
	GENERATED_BODY()

public:
	ASpyGameMode();

protected:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void GenericPlayerInitialization(AController* NewPlayer) override;
	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;

protected:
	void HandleGameStartInitialization();

public:
	UFUNCTION(BlueprintCallable)
	const USpyCharacterAssetData* GetCharacterDataForController(const AController* InController) const;
};



