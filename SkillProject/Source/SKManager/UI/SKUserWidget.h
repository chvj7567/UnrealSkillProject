// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "SKUserWidget.generated.h"

UCLASS()
class SKMANAGER_API USKUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	USKUserWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
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

public:
	UFUNCTION(BlueprintCallable)
	void SetConsumePointerInput(bool bInConsumePointerInput);

	UFUNCTION(BlueprintCallable)
	FName GetUIName();

	UFUNCTION(BlueprintCallable)
	void SetUIName(FName InUIName);

	UFUNCTION(BlueprintCallable)
	virtual void Close();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	bool bConsumePointerInput = false;

	UPROPERTY()
	FName UIName;
};
