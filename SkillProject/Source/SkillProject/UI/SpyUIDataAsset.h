// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SpyUIDataAsset.generated.h"

class USpyUserWidget;

UCLASS()
class SKILLPROJECT_API USpyUIDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<USpyUserWidget> WidgetClass;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("SpyUIDataAsset"), GetFName());
	}
};
