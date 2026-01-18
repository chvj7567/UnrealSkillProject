// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SpyUserWidget.h"

#include "SpyMainHUD.generated.h"

class UProgressBar;
class UButton;

UCLASS()
class SKILLPROJECT_API USpyMainHUD : public USpyUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UButton> Btn_Menu;

protected:
	UFUNCTION(BlueprintCallable)
	void ShowMenu();
};
