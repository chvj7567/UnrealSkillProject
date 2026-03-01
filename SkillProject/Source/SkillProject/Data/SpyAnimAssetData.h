// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "SpyAnimAssetData.generated.h"

UCLASS()
class SKILLPROJECT_API USpyAnimAssetData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FName, TSoftClassPtr<UAnimInstance>> AnimLayerMap;
};
