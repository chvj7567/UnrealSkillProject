// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SpyUserWidget.h"
#include "SpyHPBar.generated.h"

class UProgressBar;

UCLASS()
class SKILLPROJECT_API USpyHPBar : public USpyUserWidget
{
	GENERATED_BODY()
	
protected:
	void NativeConstruct() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	void UpdateHP(float InTargetHP, float InMaxHP);

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_HPBar;

private:
	bool bShouldChange = false;
	float LastTargetHP = 0.f;
	float MaxHP = 0.f;
	float CurrentTargetHP = 0.f;
	float ElapsedTime = 0.f;
	float InterpDuration = 0.f;
	float TargetPercent = 0.f;
};
