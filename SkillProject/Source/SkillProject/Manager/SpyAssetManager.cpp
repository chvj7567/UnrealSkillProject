// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/SpyAssetManager.h"
#include "Blueprint/UserWidget.h"

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

UObject* USpyAssetManager::LoadAssetSync(const FSoftObjectPath& AssetPath)
{
	UObject* Asset = nullptr;

	if (AssetPath.IsValid())
	{
		TUniquePtr<FScopeLogTime> LogTimePtr;

		if (UAssetManager::IsInitialized())
		{
			Asset = UAssetManager::GetStreamableManager().LoadSynchronous(AssetPath, false);
		}
		else
		{
			Asset = AssetPath.TryLoad();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("# [SpyAssetManager] Secondary Asset Sync Load Complete %s"), *AssetPath.ToString());

	return Asset;
}

void USpyAssetManager::LoadAssetAsync(const FSoftObjectPath& AssetPath, const FSpyAssetAndDelegate& OnComplete)
{
	//# 경로가 유효한지 확인
	if (AssetPath.IsValid() == false)
	{
		OnComplete.ExecuteIfBound(nullptr);
		return;
	}

	//# 이미 로드되어 있는지 확인
	if (UObject * Asset = AssetPath.ResolveObject())
	{
		OnComplete.ExecuteIfBound(Asset);
		return;
	}

	//# 비동기 로드
	Get().GetStreamableManager().RequestAsyncLoad(AssetPath, FStreamableDelegate::CreateLambda([AssetPath, OnComplete]()
		{
			//# 로드된 에셋이 유효하면 저장
			if (UObject* Asset = AssetPath.ResolveObject())
			{
				Get().AddLoadedAsset(Asset);
				OnComplete.ExecuteIfBound(Asset);
				UE_LOG(LogTemp, Log, TEXT("# [SpyAssetManager] Secondary Asset Async Load Complete: %s"), *AssetPath.ToString());
			}
			else
			{
				OnComplete.ExecuteIfBound(nullptr);
			}
		}));
}

void USpyAssetManager::UnloadAsset(const FSoftObjectPath& AssetPath)
{
	UObject* Asset = AssetPath.ResolveObject();

	Get().RemoveLoadedAsset(Asset);

	UE_LOG(LogTemp, Log, TEXT("# [SpyAssetManager] Primary Asset Unloaded: %s"), *AssetPath.ToString());
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
			UE_LOG(LogTemp, Log, TEXT("# [SpyAssetManager] Scan AssetType: %s, AssetID: %s"), *TypeInfo.PrimaryAssetType.ToString(), *AssetID.ToString());
		}
	}

	const int32 TotalCount = AllAssetIds.Num();
	int32 LoadedCount = 0;

	if (TotalCount == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("# [SpyAssetManager] No Primary Assets"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("# [SpyAssetManager] Primary Asset Sync Load Start (%d)"), TotalCount);

	for (const FPrimaryAssetId& AssetId : AllAssetIds)
	{
		//# 동기 로드
		TSharedPtr<FStreamableHandle> LoadHandle = LoadPrimaryAsset(AssetId, TArray<FName>());
		if (LoadHandle.IsValid())
		{
			LoadHandle->WaitUntilComplete(0.0f, false);

			if (UPrimaryDataAsset* Asset = Cast<UPrimaryDataAsset>(LoadHandle->GetLoadedAsset()))
			{
				AddPrimaryAsset(Asset);

				FString ClassName = Asset->GetClass()->GetName();
				FString AssetName = Asset->GetName();
				UE_LOG(LogTemp, Log, TEXT("# [SpyAssetManager] Primary Asset Sync Load: Class[%s] Asset[%s]"), *ClassName, *AssetName);
			}
		}

		LoadedCount++;
		LogProgress(LoadedCount, TotalCount);
	}

	UE_LOG(LogTemp, Log, TEXT("# [SpyAssetManager] All Primary Asset Sync Load Complete"));
}

UPrimaryDataAsset* USpyAssetManager::LoadPrimaryAssetSync(TSubclassOf<UPrimaryDataAsset> DataClass, const TSoftObjectPtr<UPrimaryDataAsset>& DataClassPath, FPrimaryAssetType PrimaryAssetType)
{
	//# LoadAllPrimaryAssetsSync에서 일괄 로드 함
	//# 로드 못한 경우 개별 다운로드 진행
	UPrimaryDataAsset* Asset = nullptr;

	if (DataClassPath.IsNull() == false)
	{
		if (GIsEditor)
		{
			Asset = DataClassPath.LoadSynchronous();
			LoadPrimaryAssetsWithType(PrimaryAssetType);
		}
		else
		{
			TSharedPtr<FStreamableHandle> Handle = LoadPrimaryAssetsWithType(PrimaryAssetType);
			if (Handle.IsValid())
			{
				Handle->WaitUntilComplete(0.0f, false);

				Asset = Cast<UPrimaryDataAsset>(Handle->GetLoadedAsset());
			}
		}
	}

	if (Asset)
	{
		AddPrimaryAsset(Asset);

		FString ClassName = Asset->GetClass()->GetName();
		FString AssetName = Asset->GetName();
		UE_LOG(LogTemp, Log, TEXT("# [SpyAssetManager] Primary Asset Sync Load: Class[%s] Asset[%s]"), *ClassName, *AssetName);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("# [SpyAssetManager] Primary Asset Sync Load Failed: %s"), *DataClass->GetClass()->GetName());
	}

	return Asset;
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
				if (UObject* Asset = GetPrimaryAssetObject(AssetID))
				{
					AddPrimaryAsset(Cast<UPrimaryDataAsset>(Asset));
				}
			}

			UE_LOG(LogTemp, Log, TEXT("# [SpyAssetManager] Primary Asset Async Load Complete"));
			OnComplete.ExecuteIfBound();
		}));
}

void USpyAssetManager::UnloadAllPrimaryAssets()
{
	//# 캐싱된 모든 PrimaryData 언로드
	for (auto& Data : GameDataMap)
	{
		UPrimaryDataAsset* Asset = Data.Value;
		UnloadPrimaryAsset(Asset->GetPrimaryAssetId());

		FString ClassName = Asset->GetClass()->GetName();
		FString AssetName = Asset->GetName();
		UE_LOG(LogTemp, Log, TEXT("# [SpyAssetManager] Primary Assets Unloaded AssetType: %s, AssetID: %s"), *ClassName, *AssetName);
	}

	GameDataMap.Empty();

	UE_LOG(LogTemp, Log, TEXT("# [SpyAssetManager] Primary Assets Unloaded Complete"));
}

void USpyAssetManager::UnloadAllAssets()
{
	//# 일반 에셋은 참조만 끊어주면 다음 GC에 의해 제거됨
	LoadedAssets.Empty();

	UE_LOG(LogTemp, Log, TEXT("# [SpyAssetManager] Secondary Assets Unloaded Complete"));
}

void USpyAssetManager::LogProgress(int32 Loaded, int32 Total)
{
	const int32 Percent = FMath::Clamp(FMath::RoundToInt((float)Loaded / (float)Total * 100.f), 1, 100);
	UE_LOG(LogTemp, Log, TEXT("# [SpyAssetManager] Loading %d%% (%d / %d)"), Percent, Loaded, Total);
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

void USpyAssetManager::RemoveLoadedAsset(UObject* Asset)
{
	if (ensureAlways(Asset))
	{
		FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);
		LoadedAssets.Remove(Asset);
	}
}

void USpyAssetManager::AddPrimaryAsset(UPrimaryDataAsset* Asset)
{
	//# 로드된 에셋이 유효하면 저장(GC에 의해 삭제되지 않도록 참조 유지)
	if (ensureAlways(Asset))
	{
		FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);
		GameDataMap.Add(Asset->GetClass(), Asset);
	}
}

void USpyAssetManager::RemovePrimaryAsset(UPrimaryDataAsset* Asset)
{
	if (ensureAlways(Asset))
	{
		FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);
		GameDataMap.Remove(Asset->GetClass());
	}
}

const USpyAssetData& USpyAssetManager::GetAssetData()
{
	return GetOrLoadTypedGameData<USpyAssetData>(AssetDataPath);
}