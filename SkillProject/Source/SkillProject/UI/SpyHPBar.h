// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SKUserWidget.h"
#include "SpyHPBar.generated.h"

class UProgressBar;

UCLASS()
class SKILLPROJECT_API USpyHPBar : public USKUserWidget
{
	GENERATED_BODY()
	
protected:
	void NativeConstruct() override;

public:
	void UpdateHP(float InTargetHP, float InMaxHP);

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_HPBar;
};
