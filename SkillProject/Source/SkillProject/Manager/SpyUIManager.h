// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Util/DefineEnum.h"
#include "SpyUIManager.generated.h"

class USpyUserWidget;

UCLASS()
class SKILLPROJECT_API USpyUIManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
protected:
	TArray<TObjectPtr<USpyUserWidget>> UIWidgets;
	TObjectPtr<USpyUserWidget> CurrentWidget;

public:
	static USpyUIManager* Get(const UObject* WorldContextObject);

public:
	void OpenWidget(ESpyUIType UIType);
};
