// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyUserWidget.h"
#include "Manager/SpyUIManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyUserWidget)

USpyUserWidget::USpyUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void USpyUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void USpyUserWidget::NativeDestruct()
{
	Super::NativeDestruct();
}

void USpyUserWidget::Close()
{
	FString FullNameStr = FString::Printf(TEXT("ESpyUIType::%s"), *UIName.ToString());
	int EnumValue = StaticEnum<ESpyUIType>()->GetValueByName(FName(*FullNameStr));

	//# 데디 서버·정리 시점에는 UI 매니저가 없다
	USpyUIManager* UIManager = USpyUIManager::Get(this);
	if (UIManager == nullptr)
		return;

	UIManager->CloseSpyUI(static_cast<ESpyUIType>(EnumValue));
}
