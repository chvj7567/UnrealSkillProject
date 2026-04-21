#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"

struct FSpyDataScanner
{
    static TMap<FName, FSoftObjectPath> ScanAnimLayers();
    static TMap<FName, TArray<FSoftObjectPath>> ScanAssetGroups();
    static TArray<FSoftObjectPath> ScanDataAssetsByClass(const FString& Path, UClass* AssetClass);

private:
    static TArray<FAssetData> GetAssetsUnderPath(const FString& Path, UClass* FilterClass = nullptr);
};
