// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SpyMenu.h"
#include "Components/Button.h"
#include "UI/SpyUserWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyMenu)

void USpyMenu::NativeConstruct()
{
	Super::NativeConstruct();

    if (Btn_Close)
    {
        Btn_Close->OnClicked.AddDynamic(this, &USpyUserWidget::Close);
    }
}

void USpyMenu::NativeDestruct()
{
    Super::NativeDestruct();

    if (Btn_Close)
    {
        Btn_Close->OnClicked.Clear();
    }
}
