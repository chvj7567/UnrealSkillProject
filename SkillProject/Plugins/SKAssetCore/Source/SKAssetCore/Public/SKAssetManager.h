#pragma once

#include "Engine/AssetManager.h"
#include "SKAssetData.h"

#include "SKAssetManager.generated.h"

DECLARE_DELEGATE_OneParam(FSKAssetAndDelegate, UObject*);

UCLASS(Config = Game)
class SKASSETCORE_API USKAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	USKAssetManager();

	virtual void StartInitialLoading() override;

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

public:
	static UObject* LoadAssetSync(const FSoftObjectPath& AssetPath);
	static void LoadAssetAsync(const FSoftObjectPath& AssetPath, const FSKAssetAndDelegate& OnComplete);
	static void UnloadAsset(const FSoftObjectPath& AssetPath);

protected:
	UPrimaryDataAsset* LoadPrimaryAssetSync(TSubclassOf<UPrimaryDataAsset> DataClass, const TSoftObjectPtr<UPrimaryDataAsset>& DataClassPath, FPrimaryAssetType PrimaryAssetType);
	void LoadAllPrimaryAssetsSync();
	void LoadPrimaryAssetsAsync(const TArray<FPrimaryAssetId>& AssetIds, const FSimpleDelegate& OnComplete);
	void UnloadAllPrimaryAssets();
	void UnloadAllAssets();

protected:
	void LogProgress(int32 Loaded, int32 Total);
	//# 진행률 훅 — 프로젝트 서브클래스가 오버라이드해 로딩스크린 % 등에 연결
	virtual void OnLoadProgress(int32 Loaded, int32 Total) {}
	void AddLoadedAsset(UObject* Asset);
	void RemoveLoadedAsset(UObject* Asset);
	void AddPrimaryAsset(UPrimaryDataAsset* Asset);
	void RemovePrimaryAsset(UPrimaryDataAsset* Asset);

protected:
	template <typename GameDataClass>
	const GameDataClass& GetOrLoadTypedGameData(const TSoftObjectPtr<GameDataClass>& DataPath)
	{
		if (TObjectPtr<UPrimaryDataAsset> const* pResult = GameDataMap.Find(GameDataClass::StaticClass()))
		{
			return *CastChecked<GameDataClass>(*pResult);
		}

		return *CastChecked<const GameDataClass>(LoadPrimaryAssetSync(GameDataClass::StaticClass(), DataPath, GameDataClass::StaticClass()->GetFName()));
	}

public:
	const USKAssetData& GetAssetData();

private:
	UPROPERTY()
	TSet<TObjectPtr<const UObject>> LoadedAssets;

	UPROPERTY(Transient)
	TMap<TObjectPtr<UClass>, TObjectPtr<UPrimaryDataAsset>> GameDataMap;

	FCriticalSection LoadedAssetsCritical;

private:
	UPROPERTY(Config)
	TSoftObjectPtr<USKAssetData> AssetDataPath;
};

template<typename AssetType>
AssetType* USKAssetManager::GetAssetByPath(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory)
{
	AssetType* LoadedAsset = nullptr;

	const FSoftObjectPath& AssetPath = AssetPointer.ToSoftObjectPath();

	if (AssetPath.IsValid())
	{
		LoadedAsset = AssetPointer.Get();
		if (LoadedAsset == nullptr)
		{
			LoadedAsset = Cast<AssetType>(LoadAssetSync(AssetPath));
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
		if (LoadedSubclass == nullptr)
		{
			LoadedSubclass = Cast<UClass>(LoadAssetSync(AssetPath));
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
	if (AssetPathString.EndsWith("_C") == false)
	{
		AssetPathString.Append(TEXT("_C"));
	}

	FSoftClassPath ClassPath(AssetPathString);
	TSoftClassPtr<AssetType> ClassPtr(ClassPath);
	return GetSubclassByPath<AssetType>(ClassPtr, bKeepInMemory);
}
