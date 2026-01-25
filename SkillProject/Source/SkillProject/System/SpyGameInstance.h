// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

#include "SpyGameInstance.generated.h"

class ASpyPlayerController;

UCLASS()
class SKILLPROJECT_API USpyGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	USpyGameInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	ASpyPlayerController* GetPrimaryPlayerController() const;

protected:
	virtual void Init() override;
	virtual void Shutdown() override;
};
