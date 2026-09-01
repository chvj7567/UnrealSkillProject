// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SKUIManager.h"
#include "Util/DefineEnum.h"

#include "SpyUIManager.generated.h"

class UWidgetComponent;
class USKUserWidget;

UCLASS()
class SKILLPROJECT_API USpyUIManager : public USKUIManager
{
	GENERATED_BODY()

public:
	//# leaf 서브시스템 인스턴스 접근 (호출부가 쓰는 형태)
	static USpyUIManager* Get(const UObject* WorldContextObject);

public:
	//# ZOrder 는 base 로 그대로 전달된다 — persistent UI(100) 위에 띄우려면 그보다 큰 값
	UFUNCTION(BlueprintCallable)
	void OpenSpyUI(ESpyUIType UIType, int32 ZOrder = 0);

	UFUNCTION(BlueprintCallable)
	void CloseSpyUI(ESpyUIType UIType);

	UFUNCTION(BlueprintCallable)
	void OpenSubSpyUI(ESpyUIType UIType, UWidgetComponent* WidgetComponent, EWidgetSpace Space);

	UFUNCTION(BlueprintCallable)
	bool IsSpyUIOpen(ESpyUIType UIType) const;

public:
	//# 트래블을 넘어 유지되는 UI (로딩 화면 등)
	UFUNCTION(BlueprintCallable)
	USKUserWidget* OpenPersistentSpyUI(ESpyUIType UIType, int32 ZOrder = 100);

	UFUNCTION(BlueprintCallable)
	void ClosePersistentSpyUI(ESpyUIType UIType);
};
