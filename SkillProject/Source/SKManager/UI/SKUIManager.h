// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "SKUIManager.generated.h"

class USKUserWidget;

UCLASS()
class SKMANAGER_API USKUIManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	static USKUIManager* Get(const UObject* WorldContextObject);

public:
	UFUNCTION(BlueprintCallable)
	void OpenUI(FName InUIName);

	UFUNCTION(BlueprintCallable)
	void CloseUI(FName InUIName);

	UFUNCTION(BlueprintCallable)
	void CloseLastUI();

	UFUNCTION(BlueprintCallable)
	void OpenSubUI(FName InUIName, UWidgetComponent* WidgetComponent, EWidgetSpace Space);

protected:
	UFUNCTION(BlueprintCallable)
	void AddCashingUI(USKUserWidget* UserWidget);

protected:
	const int MaxCashingUICount = 5;

protected:
	UPROPERTY()
	TArray<TObjectPtr<USKUserWidget>> OpenUIList;

	UPROPERTY()
	TArray<TObjectPtr<USKUserWidget>> CashingUIList;
};
