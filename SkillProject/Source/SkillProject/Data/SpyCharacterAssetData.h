// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Util/DefineEnum.h"

#include "SpyCharacterAssetData.generated.h"

USTRUCT()
struct FSkillAssetEntry
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	FName Name;

	UPROPERTY(EditDefaultsOnly)
	ESpySkillType SkillType;
};

USTRUCT()
struct FCharacterAssetEntry
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	ESpyCharacterType CharacterType;

	UPROPERTY(EditDefaultsOnly)
	TArray<FSkillAssetEntry> Skills;
};

USTRUCT()
struct FCharacterAssetSet
{
	GENERATED_BODY()

public:
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
	FName GetSkillAssetNameByType(ESpyCharacterType InCharacterType, ESpySkillType InSkillType) const;

private:
	UPROPERTY(EditDefaultsOnly)
	FCharacterAssetSet CharacterAssets;
};
