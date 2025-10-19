// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SpyAssetManager.h"
#include "Blueprint/UserWidget.h"
#include "UI/SpyUIDataAsset.h"

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

TObjectPtr<USpyUIDataAsset> USpyAssetManager::LoadUI()
{
    // 로드할 에셋의 PrimaryAssetId 지정
    FPrimaryAssetId AssetId = FPrimaryAssetId(TEXT("SpyUIDataAsset"), TEXT("SpyUIDataAsset"));

    // AssetManager 가져오기
    USpyAssetManager& AssetManager = USpyAssetManager::Get();

    TArray<FPrimaryAssetTypeInfo> TypeInfos;
    AssetManager.GetPrimaryAssetTypeInfoList(TypeInfos);

    for (FPrimaryAssetTypeInfo& Info : TypeInfos)
    {
        UE_LOG(LogTemp, Warning, TEXT("# Type: %s"), *Info.PrimaryAssetType.ToString());
    }

    TArray<FPrimaryAssetId> AssetIds;
    AssetManager.GetPrimaryAssetIdList(TEXT("SpyUIDataAsset"), AssetIds);

    for (const FPrimaryAssetId& Id : AssetIds)
    {
        FSoftObjectPtr AssetPtr(AssetManager.GetPrimaryAssetPath(Id));

        if (AssetPtr.IsPending())
        {
            AssetPtr.LoadSynchronous();
        }

        return Cast<USpyUIDataAsset>(AssetPtr.Get());
    }

    return nullptr;

    // 비동기 로드 요청
    //AssetManager.LoadPrimaryAsset(
    //    AssetId,
    //    TArray<FName>(), // Optional Bundles
    //    FStreamableDelegate::CreateLambda([AssetId]()
    //        {
    //            // 로드 완료 시 호출되는 콜백
    //            USpyAssetManager& AM = USpyAssetManager::Get();
    //            UObject* LoadedAsset = AM.GetPrimaryAssetObject(AssetId);
    //            if (LoadedAsset)
    //            {
    //                UE_LOG(LogTemp, Log, TEXT("Success Asset Load: %s"), *LoadedAsset->GetName());
    //            }
    //            else
    //            {
    //                UE_LOG(LogTemp, Warning, TEXT("Failed Asset Load: %s"), *AssetId.ToString());
    //            }
    //        })
    //);
}