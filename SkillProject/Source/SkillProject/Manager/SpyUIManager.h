// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Util/DefineEnum.h"
#include "Components/WidgetComponent.h"

#include "SpyUIManager.generated.h"

class UWidgetComponent;
class USKUserWidget;

UCLASS()
class SKILLPROJECT_API USpyUIManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	static USpyUIManager* Get(const UObject* WorldContextObject);

public:
	UFUNCTION(BlueprintCallable)
	void OpenUI(FName InUIName);

	UFUNCTION(BlueprintCallable)
	void CloseUI(FName InUIName);

	UFUNCTION(BlueprintCallable)
	void CloseLastUI();

	UFUNCTION(BlueprintCallable)
	void OpenSubUI(FName InUIName, UWidgetComponent* WidgetComponent, EWidgetSpace Space);

	UFUNCTION(BlueprintCallable)
	void AddCashingUI(USpyUserWidget* UserWidget);

public:
	UFUNCTION(BlueprintCallable)
	void OpenSpyUI(ESpyUIType UIType);

	UFUNCTION(BlueprintCallable)
	void CloseSpyUI(ESpyUIType UIType);

	UFUNCTION(BlueprintCallable)
	void OpenSubSpyUI(ESpyUIType UIType, UWidgetComponent* WidgetComponent, EWidgetSpace Space);

protected:
	const int MaxCashingUICount = 5;

protected:
	UPROPERTY()
	TArray<TObjectPtr<USpyUserWidget>> OpenUIList;

	UPROPERTY()
	TArray<TObjectPtr<USpyUserWidget>> CashingUIList;

};