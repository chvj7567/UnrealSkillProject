// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NativeGameplayTags.h"

#include "SpyCharacterAssetData.generated.h"

USTRUCT()
struct FSkillAssetEntry
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	FName Name;

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag SkillTag;

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag InputTag;
};

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
	TArray<FSkillAssetEntry> CharacterSkills;
};

USTRUCT()
struct FCharacterAssetSet
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	TArray<FSkillAssetEntry> CommonSkills;

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
	FName GetCommonSkillAssetName(FGameplayTag InSkillTag) const;
	FName GetCharacterSkillAssetName(FGameplayTag InClassType, FGameplayTag InSkillTag) const;
	TArray<TSubclassOf<UActorComponent>> GetAllComponentClasses(FGameplayTag InClassType) const;

private:
	UPROPERTY(EditDefaultsOnly)
	FCharacterAssetSet CharacterAssets;
};
