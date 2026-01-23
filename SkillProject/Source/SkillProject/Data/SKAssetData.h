#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "SKAssetData.generated.h"

USTRUCT()
struct FAssetEntry
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	FName AssetName;
	
	UPROPERTY(EditDefaultsOnly)
	FSoftObjectPath AssetPath;
};

USTRUCT()
struct FAssetSet
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	TArray<FAssetEntry> AssetEntries;
};

UCLASS(Const, CollapseCategories, meta=(DisplayName="SK Asset Data"))
class SKILLPROJECT_API USKAssetData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const USKAssetData& Get();

protected:
#if WITH_EDITOR
	virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

public:
	FSoftObjectPath GetAssetPathByName(const FName& AssetName) const;
	

private:
	UPROPERTY(EditDefaultsOnly)
	TMap<FName, FAssetSet> AssetGroupNameToSet;
	
	UPROPERTY()
	TMap<FName, FSoftObjectPath> AssetNameToPath;
};
