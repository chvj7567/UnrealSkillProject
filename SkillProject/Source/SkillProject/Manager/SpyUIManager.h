// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Util/DefineEnum.h"

#include "SpyUIManager.generated.h"

class USpyUserWidget;
class ASkillProjectCharacter;
class USpyUIDataAsset;

UCLASS()
class SKILLPROJECT_API USpyUIManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
protected:
	const int MaxCashingUICount = 5;

protected:
	UPROPERTY()
	TObjectPtr<USpyUIDataAsset> UIDataAsset;

	UPROPERTY()
	TArray<TObjectPtr<USpyUserWidget>> OpenUIList;

	UPROPERTY()
	TArray<TObjectPtr<USpyUserWidget>> CashingUIList;

public:
	static USpyUIManager* Get(const UObject* WorldContextObject);

public:
	UFUNCTION(BlueprintCallable)
	void OpenUI(ESpyUIType UIType);

	UFUNCTION(BlueprintCallable)
	void CloseUI(ESpyUIType UIType);

	UFUNCTION(BlueprintCallable)
	void CloseLastUI();

	UFUNCTION(BlueprintCallable)
	void OpenSubUI(ESpyUIType UIType, UWidgetComponent* WidgetComponent, EWidgetSpace Space);

protected:
	UFUNCTION(BlueprintCallable)
	void AddCashingUI(USpyUserWidget* UserWidget);
};
