// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "SpyAssetManager.generated.h"

class USpyUIDataAsset;

UCLASS()
class SKILLPROJECT_API USpyAssetManager : public UAssetManager
{
	GENERATED_BODY()
	
public:
	USpyAssetManager();

public:
	static USpyAssetManager& Get();

	TObjectPtr<USpyUIDataAsset> LoadUI();
};
