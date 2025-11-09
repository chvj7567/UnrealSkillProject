// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/SpyAssetManager.h"
#include "Blueprint/UserWidget.h"
#include "UI/SpyUIDataAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyAssetManager)

USpyAssetManager::USpyAssetManager()
{
}

USpyAssetManager& USpyAssetManager::Get()
{
	check(GEngine);

	if (USpyAssetManager* Singleton = Cast<USpyAssetManager>(GEngine->AssetManager))
	{
		return *Singleton;
	}

	UE_LOG(LogTemp, Fatal, TEXT("Invalid AssetManagerClassName in DefaultEngine.ini. It must be set to USpyAssetManager!"));

	return *NewObject<USpyAssetManager>();
}

UObject* USpyAssetManager::SynchronousLoadAsset(const FSoftObjectPath& AssetPath)
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

void USpyAssetManager::AddLoadedAsset(const UObject* Asset)
{
    if (ensureAlways(Asset))
    {
        FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);
        LoadedAssets.Add(Asset);
    }
}

const USpyAssetData& USpyAssetManager::GetAssetData()
{
    FPrimaryAssetId AssetId = FPrimaryAssetId(TEXT("SpyAssetData"), TEXT("SpyAssetData"));

    USpyAssetManager& AssetManager = USpyAssetManager::Get();

    FSoftObjectPtr AssetPtr(AssetManager.GetPrimaryAssetPath(AssetId));

    if (AssetPtr.IsPending())
    {
        AssetPtr.LoadSynchronous();
    }

    return *CastChecked<USpyAssetData>(AssetPtr.Get());
}