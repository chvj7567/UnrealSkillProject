// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SpyHPBar.h"
#include "Components/ProgressBar.h"

void USpyHPBar::NativeConstruct()
{
    Super::NativeConstruct();

    if (PB_HPBar)
    {
        PB_HPBar->SetPercent(1.0f);
    }
}

void USpyHPBar::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (bShouldChange && PB_HPBar)
    {
        ElapsedTime += InDeltaTime;
        float Alpha = FMath::Clamp(ElapsedTime / InterpDuration, 0.f, 1.f);
        float NewPercent = FMath::Lerp(PB_HPBar->Percent, TargetPercent, Alpha);

        PB_HPBar->SetPercent(NewPercent);

        if (FMath::IsNearlyEqual(PB_HPBar->Percent, TargetPercent))
        {
            bShouldChange = false;

            if (FMath::IsNearlyEqual(CurrentTargetHP, LastTargetHP) == false)
            {
                UpdateHP(LastTargetHP, MaxHP);
            }
        }
    }
}

void USpyHPBar::UpdateHP(float InTargetHP, float InMaxHP)
{
    if (bShouldChange)
    {
        LastTargetHP = InTargetHP;
        return;
    }

    bShouldChange = true;
    CurrentTargetHP = InTargetHP;
    MaxHP = InMaxHP;
    ElapsedTime = 0.f;
    InterpDuration = .5f;

    TargetPercent = FMath::Clamp(InTargetHP / InMaxHP, 0.f, 1.f);
}
