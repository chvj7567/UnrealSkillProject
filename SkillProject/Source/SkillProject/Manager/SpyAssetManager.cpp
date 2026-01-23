// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/SpyAssetManager.h"
#include "Blueprint/UserWidget.h"
#include "Data/SpyCharacterAssetData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyAssetManager)

USpyAssetManager::USpyAssetManager()
{
}

void USpyAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	LoadAllPrimaryAssetsSync();
}

USpyAssetManager& USpyAssetManager::Get()
{
	check(GEngine);

	if (USpyAssetManager* Singleton = Cast<USpyAssetManager>(GEngine->AssetManager))
	{
		return *Singleton;
	}

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

void USpyAssetManager::AsynchronousLoadAsset(const FSoftObjectPath& AssetPath, const FSimpleDelegate& OnComplete)
{
	//# 경로가 유효한지 확인
	if (AssetPath.IsValid() == false)
	{
		OnComplete.ExecuteIfBound();
		return;
	}

	//# 이미 로드되어 있는지 확인
	if (AssetPath.ResolveObject())
	{
		OnComplete.ExecuteIfBound();
		return;
	}

	//# 비동기 로드
	Get().GetStreamableManager().RequestAsyncLoad(AssetPath, FStreamableDelegate::CreateLambda([this, AssetPath, OnComplete]()
		{
			//# 로드된 에셋이 유효하면 저장
			if (UObject* LoadedAsset = AssetPath.ResolveObject())
			{
				Get().AddLoadedAsset(LoadedAsset);
				UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Async Load Complete: %s"), *AssetPath.ToString());
			}

			OnComplete.ExecuteIfBound();
		}));
}

void USpyAssetManager::AddLoadedAsset(UObject* Asset)
{
	//# 로드된 에셋이 유효하면 저장(GC에 의해 삭제되지 않도록 참조 유지)
	if (ensureAlways(Asset))
	{
		FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);
		LoadedAssets.Add(Asset);
	}
}

void USpyAssetManager::LoadAllPrimaryAssetsSync()
{
	//# 에디터 설정 창에서 정한 타입 정보 가져옴
	TArray<FPrimaryAssetTypeInfo> TypeInfos;
	GetPrimaryAssetTypeInfoList(TypeInfos);

	TArray<FPrimaryAssetId> AllAssetIds;
	for (const FPrimaryAssetTypeInfo& TypeInfo : TypeInfos)
	{
		//# 에셋 스캔 전이므로 강제로 스캔
		ScanPathsForPrimaryAssets(TypeInfo.PrimaryAssetType, TypeInfo.AssetScanPaths, TypeInfo.AssetBaseClassLoaded, false);

		//# 에셋 아이디 가져옴
		TArray<FPrimaryAssetId> AssetIds;
		GetPrimaryAssetIdList(TypeInfo.PrimaryAssetType, AssetIds);
		AllAssetIds.Append(AssetIds);

		for (const FPrimaryAssetId& AssetID : AssetIds)
		{
			UE_LOG(LogTemp, Warning, TEXT("# [SKAssetManager] Scan AssetType: %s, AssetID: %s"), *TypeInfo.PrimaryAssetType.ToString(), *AssetID.ToString());
		}
	}

	const int32 TotalCount = AllAssetIds.Num();
	int32 LoadedCount = 0;

	if (TotalCount == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("# [SKAssetManager] No Primary Assets"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Sync Load Start (%d)"), TotalCount);

	for (const FPrimaryAssetId& AssetId : AllAssetIds)
	{
		//# 동기 로드
		TSharedPtr<FStreamableHandle> LoadHandle = LoadPrimaryAsset(AssetId, TArray<FName>());
		if (LoadHandle.IsValid())
		{
			UObject* LoadedAsset = LoadHandle->GetLoadedAsset();
			if (LoadedAsset != nullptr)
			{
				Get().AddLoadedAsset(LoadedAsset);
			}
		}

		LoadedCount++;
		LogProgress(LoadedCount, TotalCount);
	}

	UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Sync Load Complete"));
}

void USpyAssetManager::LoadPrimaryAssetsAsync(const TArray<FPrimaryAssetId>& AssetIds, const FSimpleDelegate& OnComplete)
{
	if (AssetIds.Num() == 0)
	{
		OnComplete.ExecuteIfBound();
		return;
	}

	//# 비동기 로드
	LoadPrimaryAssets(AssetIds, TArray<FName>(), FStreamableDelegate::CreateLambda([this, AssetIds, OnComplete]()
		{
			for (const FPrimaryAssetId& AssetID : AssetIds)
			{
				if (UObject* LoadedAsset = GetPrimaryAssetObject(AssetID))
				{
					Get().AddLoadedAsset(LoadedAsset);
				}
			}

			UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Async Load Complete"));
			OnComplete.ExecuteIfBound();
		}));
}

int32 USpyAssetManager::UnloadPrimaryAssets(const TArray<FPrimaryAssetId>& AssetIds)
{
	//# 에셋 언로딩
	int32 UnloadAssetCount = Super::UnloadPrimaryAssets(AssetIds);

	UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Assets %d Unloaded Complete"), UnloadAssetCount);

	return UnloadAssetCount;
}

void USpyAssetManager::LogProgress(int32 Loaded, int32 Total)
{
	const int32 Percent = FMath::Clamp(FMath::RoundToInt((float)Loaded / (float)Total * 100.f), 1, 100);
	UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Loading %d%% (%d / %d)"), Percent, Loaded, Total);
}

const USKAssetData* USpyAssetManager::GetAssetData()
{
	if (TObjectPtr<UPrimaryDataAsset> const* pResult = GameDataMap.Find(USKAssetData::StaticClass()))
	{
		return CastChecked<USKAssetData>(*pResult);
	}

	return CastChecked<USKAssetData>(SynchronousLoadAsset(AssetDataPath.ToSoftObjectPath()));
}

const USpyCharacterAssetData* USpyAssetManager::GetCharacterAssetData()
{
	if (TObjectPtr<UPrimaryDataAsset> const* pResult = GameDataMap.Find(USKAssetData::StaticClass()))
	{
		return CastChecked<USpyCharacterAssetData>(*pResult);
	}

	return CastChecked<USpyCharacterAssetData>(SynchronousLoadAsset(AssetDataPath.ToSoftObjectPath()));
}

FName USpyAssetManager::GetSkillAssetNameByType(FGameplayTag InCharacterType, FGameplayTag InSkillTag)
{
	return Get().GetCharacterAssetData()->GetCharacterSkillAssetName(InCharacterType, InSkillTag);
}
