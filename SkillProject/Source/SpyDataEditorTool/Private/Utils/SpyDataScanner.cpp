#include "Utils/SpyDataScanner.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Animation/AnimInstance.h"

static IAssetRegistry& GetRegistry()
{
    return FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
}

TArray<FAssetData> FSpyDataScanner::GetAssetsUnderPath(const FString& Path, UClass* FilterClass)
{
    TArray<FAssetData> Result;
    FARFilter Filter;
    Filter.PackagePaths.Add(*Path);
    Filter.bRecursivePaths = true;
    if (FilterClass)
    {
        Filter.ClassPaths.Add(FilterClass->GetClassPathName());
        Filter.bRecursiveClasses = true;
    }
    GetRegistry().GetAssets(Filter, Result);
    return Result;
}

TMap<FName, FSoftObjectPath> FSpyDataScanner::ScanAnimLayers()
{
    TMap<FName, FSoftObjectPath> Result;
    const FString Prefix = TEXT("ABP_AnimLayers_");
    for (const FAssetData& Asset : GetAssetsUnderPath(TEXT("/Game/Spy/Animation"), UAnimInstance::StaticClass()))
    {
        FString Name = Asset.AssetName.ToString();
        if (Name.StartsWith(Prefix))
        {
            FString Key = Name.RightChop(Prefix.Len());
            Result.Add(FName(*Key), FSoftObjectPath(Asset.GetSoftObjectPath()));
        }
    }
    return Result;
}

TMap<FName, TArray<FSoftObjectPath>> FSpyDataScanner::ScanAssetGroups()
{
    TMap<FName, TArray<FSoftObjectPath>> Result;
    const FString SpyBase = TEXT("/Game/Spy/");
    for (const FAssetData& Asset : GetAssetsUnderPath(TEXT("/Game/Spy")))
    {
        FString PackagePath = Asset.PackagePath.ToString();
        FString Remainder = PackagePath.RightChop(SpyBase.Len());
        FString GroupName;
        if (!Remainder.Split(TEXT("/"), &GroupName, nullptr))
        {
            GroupName = Remainder;
        }
        if (!GroupName.IsEmpty())
        {
            Result.FindOrAdd(FName(*GroupName)).Add(FSoftObjectPath(Asset.GetSoftObjectPath()));
        }
    }
    return Result;
}

TArray<FSoftObjectPath> FSpyDataScanner::ScanDataAssetsByClass(const FString& Path, UClass* AssetClass)
{
    TArray<FSoftObjectPath> Result;
    for (const FAssetData& Asset : GetAssetsUnderPath(Path, AssetClass))
    {
        Result.Add(FSoftObjectPath(Asset.GetSoftObjectPath()));
    }
    return Result;
}
