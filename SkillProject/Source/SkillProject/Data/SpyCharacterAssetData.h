// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NativeGameplayTags.h"

#include "SpyCharacterAssetData.generated.h"

class USpyAbilityData;
class USpyInputConfig;
class UInputMappingContext;

USTRUCT()
struct FCharacterAssetEntry
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag ClassType;

	UPROPERTY(EditDefaultsOnly)
	TArray<TSubclassOf<UActorComponent>> CharacterComponentClasses;

	UPROPERTY(EditDefaultsOnly)
	TArray<TObjectPtr<USpyAbilityData>> ClassSkills;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USpyInputConfig> InputConfig;

	UPROPERTY(EditDefaultsOnly)
	TMap<TObjectPtr<UInputMappingContext>, int32> InputMappingContexts;
};

USTRUCT()
struct FCharacterAssetSet
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	TArray<TObjectPtr<USpyAbilityData>> CommonSkills;

	UPROPERTY(EditDefaultsOnly)
	TArray<TSubclassOf<UActorComponent>> CommonComponentClasses;

	UPROPERTY(EditDefaultsOnly)
	TArray<FCharacterAssetEntry> AssetEntries;
};

UCLASS()
class SKILLPROJECT_API USpyCharacterAssetData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const USpyCharacterAssetData& Get();

protected:
#if WITH_EDITOR
	virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

public:
	TArray<TSubclassOf<UActorComponent>> GetAllComponentClasses(FGameplayTag InClassType) const;

public:
	UPROPERTY(EditDefaultsOnly)
	FCharacterAssetSet CharacterAssets;
};
