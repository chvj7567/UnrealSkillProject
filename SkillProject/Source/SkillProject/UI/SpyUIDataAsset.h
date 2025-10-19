// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Util//DefineEnum.h"
#include "SpyUIDataAsset.generated.h"

class USpyUserWidget;

USTRUCT(BlueprintType)
struct FSpyUIData
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ESpyUIType UIType = ESpyUIType::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSubclassOf<UUserWidget> UIWidgetClass;
};

UCLASS()
class SKILLPROJECT_API USpyUIDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
    UPROPERTY(EditDefaultsOnly)
    TArray<FSpyUIData> UIDatas;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("SpyUIDataAsset"), GetFName());
	}
};
