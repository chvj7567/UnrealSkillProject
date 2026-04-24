#pragma once

#include "CoreMinimal.h"
#include "Misc/MessageDialog.h"
#include "UObject/SavePackage.h"

namespace SpyEditorUtils
{
    inline bool SaveAsset(UObject* Asset)
    {
        if (!Asset) return false;
        UPackage* Package = Asset->GetOutermost();
        Package->MarkPackageDirty();
        FString PackageFilename;
        if (!FPackageName::TryConvertLongPackageNameToFilename(
                Package->GetName(), PackageFilename, FPackageName::GetAssetPackageExtension()))
        {
            return false;
        }
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        return UPackage::SavePackage(Package, Asset, *PackageFilename, SaveArgs);
    }

    inline bool ConfirmApply(const TArray<FString>& AssetNames)
    {
        FString Message = TEXT("다음 에셋을 저장합니다:\n\n");
        for (const FString& Name : AssetNames)
        {
            Message += TEXT("  • ") + Name + TEXT("\n");
        }
        Message += TEXT("\n계속하시겠습니까?");
        return FMessageDialog::Open(EAppMsgType::OkCancel,
            FText::FromString(Message)) == EAppReturnType::Ok;
    }
}
