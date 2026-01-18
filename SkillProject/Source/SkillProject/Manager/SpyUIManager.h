// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SKUIManager.h"
#include "Util/DefineEnum.h"

#include "SpyUIManager.generated.h"

UCLASS()
class SKILLPROJECT_API USpyUIManager : public USKUIManager
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	static USpyUIManager* Get(const UObject* WorldContextObject);

public:
	UFUNCTION(BlueprintCallable)
	void OpenSpyUI(ESpyUIType UIType);

	UFUNCTION(BlueprintCallable)
	void CloseSpyUI(ESpyUIType UIType);

	UFUNCTION(BlueprintCallable)
	void CloseLastSpyUI();

	UFUNCTION(BlueprintCallable)
	void OpenSubSpyUI(ESpyUIType UIType, UWidgetComponent* WidgetComponent, EWidgetSpace Space);
};
