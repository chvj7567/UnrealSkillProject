// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SpyMainHUD.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Manager/SpyUIManager.h"

void USpyMainHUD::NativeConstruct()
{
    Super::NativeConstruct();

    if (Btn_Menu)
    {
        Btn_Menu->OnClicked.AddDynamic(this, &USpyMainHUD::ShowMenu);
    }
}

void USpyMainHUD::NativeDestruct()
{
    Super::NativeDestruct();

    if (Btn_Menu)
    {
        Btn_Menu->OnClicked.Clear();
    }
}

void USpyMainHUD::ShowMenu()
{
    USpyUIManager::Get(this)->OpenUI(ESpyUIType::Menu);
}
