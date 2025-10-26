// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SpyMenu.h"
#include "Components/Button.h"

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
