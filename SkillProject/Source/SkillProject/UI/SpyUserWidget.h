// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Util//DefineEnum.h"

#include "SpyUserWidget.generated.h"

class USpyUIManager;

UCLASS()
class SKILLPROJECT_API USpyUserWidget : public UUserWidget
{
	GENERATED_BODY()

private:
	friend USpyUIManager;

public:
	USpyUserWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
protected:
	virtual void OnWidgetRebuilt() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnTouchGesture(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	bool bConsumePointerInput = false;

	UPROPERTY()
	ESpyUIType UIType;

public:
	UFUNCTION(BlueprintCallable)
	void SetConsumePointerInput(bool bInConsumePointerInput);

	UFUNCTION(BlueprintCallable)
	ESpyUIType GetUIType();

	UFUNCTION(BlueprintCallable)
	virtual void Close();
};
