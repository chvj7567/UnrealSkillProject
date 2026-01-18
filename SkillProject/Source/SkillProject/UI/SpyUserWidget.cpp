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

	USpyUIManager::Get(this)->CloseSpyUI(static_cast<ESpyUIType>(EnumValue));
}
