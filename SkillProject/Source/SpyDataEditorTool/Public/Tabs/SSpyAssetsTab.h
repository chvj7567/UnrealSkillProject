#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "UObject/StrongObjectPtr.h"
#include "Data/SpyAssetData.h"
#include "Data/SpyAnimAssetData.h"

class IDetailsView;

class SSpyAssetsTab : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SSpyAssetsTab) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    FReply OnScanClicked();
    FReply OnApplyClicked();

    TSharedPtr<IDetailsView> AssetDataView;
    TSharedPtr<IDetailsView> AnimAssetDataView;

    TStrongObjectPtr<USpyAssetData>     AssetData;
    TStrongObjectPtr<USpyAnimAssetData> AnimAssetData;
};
