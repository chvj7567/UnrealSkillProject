// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SKUserWidget.h"

#include "SpyUserWidget.generated.h"

UCLASS()
class SKILLPROJECT_API USpyUserWidget : public USKUserWidget
{
	GENERATED_BODY()

public:
	USpyUserWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	virtual void Close() override;
};
