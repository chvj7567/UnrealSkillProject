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
class SKASSETCORE_API USKAssetData : public UPrimaryDataAsset
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

	//# 등록된 모든 에셋 경로를 열거한다 (로딩 화면 프리로드 등 배치 로드용)
	void GetAllAssetPaths(TArray<FSoftObjectPath>& OutPaths) const;

private:
	UPROPERTY(EditDefaultsOnly)
	TMap<FName, FAssetSet> AssetGroupNameToSet;

	UPROPERTY()
	TMap<FName, FSoftObjectPath> AssetNameToPath;
};
