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
	const int MaxCashingUICount = 5;

protected:
	UPROPERTY()
	TArray<TObjectPtr<USpyUserWidget>> OpenUIList;

	UPROPERTY()
	TArray<TObjectPtr<USpyUserWidget>> CashingUIList;

	UPROPERTY()
	ESpyUIType LastUIType;

public:
	static USpyUIManager* Get(const UObject* WorldContextObject);

public:
	UFUNCTION(BlueprintCallable)
	void OpenUI(ESpyUIType UIType);

	UFUNCTION(BlueprintCallable)
	void CloseUI(ESpyUIType UIType);

	UFUNCTION(BlueprintCallable)
	void CloseLastUI();

protected:
	UFUNCTION(BlueprintCallable)
	void AddCashingUI(USpyUserWidget* UserWidget);
};
