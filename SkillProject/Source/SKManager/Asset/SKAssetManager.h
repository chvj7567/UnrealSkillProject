// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/AssetManager.h"
#include "Templates/SubclassOf.h"
#include "Asset/SKAssetData.h"

#include "SKAssetManager.generated.h"

class USKAssetData;

UCLASS()
class SKMANAGER_API USKAssetManager : public UAssetManager
{
	GENERATED_BODY()
	
public:
	USKAssetManager();

public:
	static USKAssetManager& Get();

	template<typename AssetType>
	static AssetType* GetAssetByPath(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);

	template<typename AssetType>
	static AssetType* GetAssetByName(const FName& AssetName, bool bKeepInMemory = true);

	template<typename AssetType>
	static TSubclassOf<AssetType> GetSubclassByPath(const TSoftClassPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);

	template<typename AssetType>
	static TSubclassOf<AssetType> GetSubclassByName(const FName& AssetName, bool bKeepInMemory = true);

protected:
	static UObject* SynchronousLoadAsset(const FSoftObjectPath& AssetPath);
	void AddLoadedAsset(const UObject* Asset);
	
public:
	const USKAssetData& GetAssetData();

private:
	UPROPERTY()
	TSet<TObjectPtr<const UObject>> LoadedAssets;

	FCriticalSection LoadedAssetsCritical;
};

template<typename AssetType>
AssetType* USKAssetManager::GetAssetByPath(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory)
{
	AssetType* LoadedAsset = nullptr;

	const FSoftObjectPath& AssetPath = AssetPointer.ToSoftObjectPath();

	if (AssetPath.IsValid())
	{
		LoadedAsset = AssetPointer.Get();
		if (!LoadedAsset)
		{
			LoadedAsset = Cast<AssetType>(SynchronousLoadAsset(AssetPath));
			ensureAlwaysMsgf(LoadedAsset, TEXT("Failed to load asset [%s]"), *AssetPointer.ToString());
		}

		if (LoadedAsset && bKeepInMemory)
		{
			Get().AddLoadedAsset(Cast<UObject>(LoadedAsset));
		}
	}

	return LoadedAsset;
}

template <typename AssetType>
AssetType* USKAssetManager::GetAssetByName(const FName& AssetName, bool bKeepInMemory)
{
	const USKAssetData& AssetData = Get().GetAssetData();
	const FSoftObjectPath& AssetPath = AssetData.GetAssetPathByName(AssetName);
	TSoftObjectPtr<AssetType> AssetPtr(AssetPath);
	return GetAssetByPath<AssetType>(AssetPtr, bKeepInMemory);
}

template<typename AssetType>
TSubclassOf<AssetType> USKAssetManager::GetSubclassByPath(const TSoftClassPtr<AssetType>& AssetPointer, bool bKeepInMemory)
{
	TSubclassOf<AssetType> LoadedSubclass;

	const FSoftObjectPath& AssetPath = AssetPointer.ToSoftObjectPath();

	if (AssetPath.IsValid())
	{
		LoadedSubclass = AssetPointer.Get();
		if (!LoadedSubclass)
		{
			LoadedSubclass = Cast<UClass>(SynchronousLoadAsset(AssetPath));
			ensureAlwaysMsgf(LoadedSubclass, TEXT("Failed to load asset class [%s]"), *AssetPointer.ToString());
		}

		if (LoadedSubclass && bKeepInMemory)
		{
			Get().AddLoadedAsset(Cast<UObject>(LoadedSubclass));
		}
	}

	return LoadedSubclass;
}

template <typename AssetType>
TSubclassOf<AssetType> USKAssetManager::GetSubclassByName(const FName& AssetName, bool bKeepInMemory)
{
	const USKAssetData& AssetData = Get().GetAssetData();
	const FSoftObjectPath& AssetPath = AssetData.GetAssetPathByName(AssetName);

	FString AssetPathString = AssetPath.GetAssetPathString();
	AssetPathString.Append(TEXT("_C"));

	FSoftClassPath ClassPath(AssetPathString);
	TSoftClassPtr<AssetType> ClassPtr(ClassPath);
	return GetSubclassByPath<AssetType>(ClassPtr, bKeepInMemory);
}