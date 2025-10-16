// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SpyMainHUD.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"

void USpyMainHUD::NativeConstruct()
{
    Super::NativeConstruct();

    if (Btn_Start)
    {
        Btn_Start->OnClicked.AddDynamic(this, &USpyMainHUD::OnIncreaseButtonClicked);
    }

    if (PB_Timer)
    {
        PB_Timer->SetPercent(0.0f);
    }
}

void USpyMainHUD::OnIncreaseButtonClicked()
{
    bShouldIncrease = true;
}

void USpyMainHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (bShouldIncrease && PB_Timer)
    {
        CurrentProgress += 0.1f * InDeltaTime * 10.f; // 약 0.1씩 증가 (속도 조절 가능)
        CurrentProgress = FMath::Clamp(CurrentProgress, 0.0f, 1.0f);
        PB_Timer->SetPercent(CurrentProgress);

        if (CurrentProgress >= 1.0f)
        {
            bShouldIncrease = false;
        }
    }
}