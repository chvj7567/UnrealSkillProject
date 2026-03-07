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
	FGameplayTag StartSkillTag;

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag ComboTag;
};

UCLASS()
class SKILLPROJECT_API USpyComboAssetData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	FGameplayTag GetComboTag(FGameplayTag InStartSkillTag);

protected:
	UPROPERTY(EditDefaultsOnly)
	TArray<FSpyComboSet> ComboSets;
};
