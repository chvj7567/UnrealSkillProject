// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "SpyComboAssetData.generated.h"

USTRUCT(BlueprintType)
struct FSpyComboSet
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	int ComboStep;

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag ComboTag;
};

UCLASS()
class SKILLPROJECT_API USpyComboAssetData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	FGameplayTag GetComboTag(int ComboStep);

protected:
	UPROPERTY(EditDefaultsOnly)
	TArray<FSpyComboSet> ComboSets;
};
