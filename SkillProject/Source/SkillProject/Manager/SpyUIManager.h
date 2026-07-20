// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SKUIManager.h"
#include "Util/DefineEnum.h"

#include "SpyUIManager.generated.h"

class UWidgetComponent;

UCLASS()
class SKILLPROJECT_API USpyUIManager : public USKUIManager
{
	GENERATED_BODY()

public:
	//# leaf 서브시스템 인스턴스 접근 (호출부가 쓰는 형태)
	static USpyUIManager* Get(const UObject* WorldContextObject);

public:
	UFUNCTION(BlueprintCallable)
	void OpenSpyUI(ESpyUIType UIType);

	UFUNCTION(BlueprintCallable)
	void CloseSpyUI(ESpyUIType UIType);

	UFUNCTION(BlueprintCallable)
	void OpenSubSpyUI(ESpyUIType UIType, UWidgetComponent* WidgetComponent, EWidgetSpace Space);
};
