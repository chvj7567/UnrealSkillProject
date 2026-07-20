#include "SKAssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKAssetManager)

USKAssetManager::USKAssetManager()
{
}

void USKAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	LoadAllPrimaryAssetsSync();
}

USKAssetManager& USKAssetManager::Get()
{
	check(GEngine);

	if (USKAssetManager* Singleton = Cast<USKAssetManager>(GEngine->AssetManager))
	{
		return *Singleton;
	}

	return *NewObject<USKAssetManager>();
}

UObject* USKAssetManager::LoadAssetSync(const FSoftObjectPath& AssetPath)
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

	UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Secondary Asset Sync Load Complete %s"), *AssetPath.ToString());

	return Asset;
}

void USKAssetManager::LoadAssetAsync(const FSoftObjectPath& AssetPath, const FSKAssetAndDelegate& OnComplete)
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
				UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Secondary Asset Async Load Complete: %s"), *AssetPath.ToString());
			}
			else
			{
				OnComplete.ExecuteIfBound(nullptr);
			}
		}));
}

void USKAssetManager::UnloadAsset(const FSoftObjectPath& AssetPath)
{
	UObject* Asset = AssetPath.ResolveObject();

	Get().RemoveLoadedAsset(Asset);

	UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Primary Asset Unloaded: %s"), *AssetPath.ToString());
}

void USKAssetManager::LoadAllPrimaryAssetsSync()
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
			UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Scan AssetType: %s, AssetID: %s"), *TypeInfo.PrimaryAssetType.ToString(), *AssetID.ToString());
		}
	}

	const int32 TotalCount = AllAssetIds.Num();
	int32 LoadedCount = 0;

	if (TotalCount == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("# [SKAssetManager] No Primary Assets"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Primary Asset Sync Load Start (%d)"), TotalCount);

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
				UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Primary Asset Sync Load: Class[%s] Asset[%s]"), *ClassName, *AssetName);
			}
		}

		LoadedCount++;
		LogProgress(LoadedCount, TotalCount);
	}

	UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] All Primary Asset Sync Load Complete"));
}

UPrimaryDataAsset* USKAssetManager::LoadPrimaryAssetSync(TSubclassOf<UPrimaryDataAsset> DataClass, const TSoftObjectPtr<UPrimaryDataAsset>& DataClassPath, FPrimaryAssetType PrimaryAssetType)
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
		UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Primary Asset Sync Load: Class[%s] Asset[%s]"), *ClassName, *AssetName);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Primary Asset Sync Load Failed: %s"), *DataClass->GetClass()->GetName());
	}

	return Asset;
}

void USKAssetManager::LoadPrimaryAssetsAsync(const TArray<FPrimaryAssetId>& AssetIds, const FSimpleDelegate& OnComplete)
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

			UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Primary Asset Async Load Complete"));
			OnComplete.ExecuteIfBound();
		}));
}

void USKAssetManager::UnloadAllPrimaryAssets()
{
	//# 캐싱된 모든 PrimaryData 언로드
	for (auto& Data : GameDataMap)
	{
		UPrimaryDataAsset* Asset = Data.Value;
		UnloadPrimaryAsset(Asset->GetPrimaryAssetId());

		FString ClassName = Asset->GetClass()->GetName();
		FString AssetName = Asset->GetName();
		UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Primary Assets Unloaded AssetType: %s, AssetID: %s"), *ClassName, *AssetName);
	}

	GameDataMap.Empty();

	UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Primary Assets Unloaded Complete"));
}

void USKAssetManager::UnloadAllAssets()
{
	//# 일반 에셋은 참조만 끊어주면 다음 GC에 의해 제거됨
	LoadedAssets.Empty();

	UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Secondary Assets Unloaded Complete"));
}

void USKAssetManager::LogProgress(int32 Loaded, int32 Total)
{
	const int32 Percent = FMath::Clamp(FMath::RoundToInt((float)Loaded / (float)Total * 100.f), 1, 100);
	UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Loading %d%% (%d / %d)"), Percent, Loaded, Total);

	//# 진행률 훅 — 서브클래스가 오버라이드
	OnLoadProgress(Loaded, Total);
}

void USKAssetManager::AddLoadedAsset(UObject* Asset)
{
	//# 로드된 에셋이 유효하면 저장(GC에 의해 삭제되지 않도록 참조 유지)
	if (ensureAlways(Asset))
	{
		FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);
		LoadedAssets.Add(Asset);
	}
}

void USKAssetManager::RemoveLoadedAsset(UObject* Asset)
{
	if (ensureAlways(Asset))
	{
		FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);
		LoadedAssets.Remove(Asset);
	}
}

void USKAssetManager::AddPrimaryAsset(UPrimaryDataAsset* Asset)
{
	//# 로드된 에셋이 유효하면 저장(GC에 의해 삭제되지 않도록 참조 유지)
	if (ensureAlways(Asset))
	{
		FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);
		GameDataMap.Add(Asset->GetClass(), Asset);
	}
}

void USKAssetManager::RemovePrimaryAsset(UPrimaryDataAsset* Asset)
{
	if (ensureAlways(Asset))
	{
		FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);
		GameDataMap.Remove(Asset->GetClass());
	}
}

const USKAssetData& USKAssetManager::GetAssetData()
{
	//# 프로젝트 서브클래스(예: USpyAssetManager)가 concrete 타입으로 오버라이드해야 한다.
	checkf(false, TEXT("USKAssetManager::GetAssetData 는 프로젝트 서브클래스에서 오버라이드해야 합니다."));
	static const USKAssetData* Fallback = NewObject<USKAssetData>();
	return *Fallback;
}
