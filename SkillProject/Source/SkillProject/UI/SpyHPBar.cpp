// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SpyHPBar.h"
#include "Components/ProgressBar.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyHPBar)

void USpyHPBar::NativeConstruct()
{
    Super::NativeConstruct();

    if (PB_HPBar)
    {
        PB_HPBar->SetPercent(1.0f);
    }
}

void USpyHPBar::UpdateHP(float InTargetHP, float InMaxHP)
{
    if (PB_HPBar)
    {
        if (InTargetHP == 0 || InMaxHP == 0)
        {
            PB_HPBar->SetPercent(0);
        }
        else
        {
            PB_HPBar->SetPercent(InTargetHP / InMaxHP);
        }
    }
}
