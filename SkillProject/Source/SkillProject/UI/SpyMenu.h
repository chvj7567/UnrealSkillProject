// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SpyUserWidget.h"
#include "SpyMenu.generated.h"

class UButton;

UCLASS()
class SKILLPROJECT_API USpyMenu : public USpyUserWidget
{
	GENERATED_BODY()

protected:
	void NativeConstruct() override;
	void NativeDestruct() override;

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UButton> Btn_Close;
};
