// Fill out your copyright notice in the Description page of Project Settings.


#include "Asset/SKAssetManager.h"
#include "Asset/SKAssetData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKAssetManager)

USKAssetManager::USKAssetManager()
{
}

USKAssetManager& USKAssetManager::Get()
{
	check(GEngine);

	if (USKAssetManager* Singleton = Cast<USKAssetManager>(GEngine->AssetManager))
	{
		return *Singleton;
	}

	UE_LOG(LogTemp, Fatal, TEXT("Invalid AssetManagerClassName in DefaultEngine.ini. It must be set to USKAssetManager!"));

	return *NewObject<USKAssetManager>();
}

UObject* USKAssetManager::SynchronousLoadAsset(const FSoftObjectPath& AssetPath)
{
    if (AssetPath.IsValid())
    {
        TUniquePtr<FScopeLogTime> LogTimePtr;

        if (UAssetManager::IsInitialized())
        {
            return UAssetManager::GetStreamableManager().LoadSynchronous(AssetPath, false);
        }

        return AssetPath.TryLoad();
    }

    return nullptr;
}

void USKAssetManager::AddLoadedAsset(const UObject* Asset)
{
    if (ensureAlways(Asset))
    {
        FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);
        LoadedAssets.Add(Asset);
    }
}

const USKAssetData& USKAssetManager::GetAssetData()
{
    FPrimaryAssetId AssetId = FPrimaryAssetId(TEXT("SpyAssetData"), TEXT("SpyAssetData"));
    FSoftObjectPtr AssetPtr(Get().GetPrimaryAssetPath(AssetId));

    if (AssetPtr.IsPending())
    {
        AssetPtr.LoadSynchronous();
    }

    return *CastChecked<USKAssetData>(AssetPtr.Get());
}