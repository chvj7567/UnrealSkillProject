// Fill out your copyright notice in the Description page of Project Settings.


#include "SKUserWidget.h"
#include "UI/SKUIManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKUserWidget)

USKUserWidget::USKUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void USKUserWidget::OnWidgetRebuilt()
{
	Super::OnWidgetRebuilt();
}

void USKUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void USKUserWidget::NativeDestruct()
{
	Super::NativeDestruct();
}

FReply USKUserWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return bConsumePointerInput ? FReply::Handled() : Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply USKUserWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return bConsumePointerInput ? FReply::Handled() : Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply USKUserWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return bConsumePointerInput ? FReply::Handled() : Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

FReply USKUserWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return bConsumePointerInput ? FReply::Handled() : Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

FReply USKUserWidget::NativeOnTouchGesture(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	return bConsumePointerInput ? FReply::Handled() : Super::NativeOnTouchGesture(InGeometry, InGestureEvent);
}

FReply USKUserWidget::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	return bConsumePointerInput ? FReply::Handled() : Super::NativeOnTouchStarted(InGeometry, InGestureEvent);
}

FReply USKUserWidget::NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	return bConsumePointerInput ? FReply::Handled() : Super::NativeOnTouchMoved(InGeometry, InGestureEvent);
}

FReply USKUserWidget::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	return bConsumePointerInput ? FReply::Handled() : Super::NativeOnTouchEnded(InGeometry, InGestureEvent);
}

void USKUserWidget::SetConsumePointerInput(bool bInConsumePointerInput)
{
	bConsumePointerInput = bInConsumePointerInput;
}

FName USKUserWidget::GetUIName()
{
	return UIName;
}

void USKUserWidget::SetUIName(FName InUIName)
{
	UIName = InUIName;
}

void USKUserWidget::Close()
{
	USKUIManager::Get(this)->CloseUI(UIName);
}