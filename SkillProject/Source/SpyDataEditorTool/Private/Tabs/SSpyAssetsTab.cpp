#include "Tabs/SSpyAssetsTab.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/MessageDialog.h"
#include "Utils/SpyDataScanner.h"
#include "Utils/SpyEditorUtils.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "SSpyAssetsTab"

static UObject* LoadDataAssetByClass(UClass* Class, const FString& SearchPath = TEXT("/Game/Spy/Data"))
{
    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
    TArray<FAssetData> Found;
    FARFilter Filter;
    Filter.PackagePaths.Add(*SearchPath);
    Filter.bRecursivePaths = false;
    Filter.ClassPaths.Add(Class->GetClassPathName());
    Filter.bRecursiveClasses = true;
    Registry.GetAssets(Filter, Found);
    return Found.Num() > 0 ? Found[0].GetAsset() : nullptr;
}

void SSpyAssetsTab::Construct(const FArguments& InArgs)
{
    FPropertyEditorModule& PropModule =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    FDetailsViewArgs Args;
    Args.bHideSelectionTip = true;
    Args.bAllowSearch = false;
    Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;

    AssetDataView    = PropModule.CreateDetailView(Args);
    AnimAssetDataView = PropModule.CreateDetailView(Args);

    auto PropertyFilter = FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& P) -> bool
    {
        return !P.Property.GetName().EndsWith(TEXT("Cache")) &&
               !P.Property.GetName().EndsWith(TEXT("ToPath"));
    });
    AssetDataView->SetIsPropertyVisibleDelegate(PropertyFilter);
    AnimAssetDataView->SetIsPropertyVisibleDelegate(PropertyFilter);

    AssetData     = TStrongObjectPtr<USpyAssetData>(Cast<USpyAssetData>(LoadDataAssetByClass(USpyAssetData::StaticClass())));
    AnimAssetData = TStrongObjectPtr<USpyAnimAssetData>(Cast<USpyAnimAssetData>(LoadDataAssetByClass(USpyAnimAssetData::StaticClass())));

    if (AssetData)     AssetDataView->SetObject(AssetData.Get());
    if (AnimAssetData) AnimAssetDataView->SetObject(AnimAssetData.Get());

    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(4)
        [
            SNew(SButton)
            .Text(LOCTEXT("Scan", "Scan"))
            .OnClicked(this, &SSpyAssetsTab::OnScanClicked)
        ]
        + SVerticalBox::Slot().FillHeight(1.f)
        [
            SNew(SScrollBox)
            + SScrollBox::Slot()
            [ SNew(STextBlock).Text(LOCTEXT("AssetDataLabel", "SpyAssetData")) ]
            + SScrollBox::Slot()
            [ AssetDataView.ToSharedRef() ]
            + SScrollBox::Slot().Padding(FMargin(0, 8, 0, 0))
            [ SNew(STextBlock).Text(LOCTEXT("AnimDataLabel", "SpyAnimAssetData")) ]
            + SScrollBox::Slot()
            [ AnimAssetDataView.ToSharedRef() ]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(4)
        [
            SNew(SButton)
            .Text(LOCTEXT("Apply", "Apply Assets"))
            .OnClicked(this, &SSpyAssetsTab::OnApplyClicked)
        ]
    ];

    GEditor->GetTimerManager()->SetTimerForNextTick([this]()
    {
        if (AssetDataView.IsValid())     AssetDataView->ForceRefreshDetails();
        if (AnimAssetDataView.IsValid()) AnimAssetDataView->ForceRefreshDetails();
    });
}

FReply SSpyAssetsTab::OnScanClicked()
{
    if (!AnimAssetData) return FReply::Handled();

    TMap<FName, FSoftObjectPath> AnimLayers = FSpyDataScanner::ScanAnimLayers();

    AnimAssetData->Modify();
    AnimAssetData->AnimLayerMap.Empty();
    for (auto& Pair : AnimLayers)
    {
        TSoftClassPtr<UAnimInstance> SoftClass(Pair.Value);
        AnimAssetData->AnimLayerMap.Add(Pair.Key, SoftClass);
    }
    AnimAssetDataView->ForceRefreshDetails();

    if (AssetData)
    {
        AssetData->Modify();
        AssetDataView->ForceRefreshDetails();
    }

    return FReply::Handled();
}

FReply SSpyAssetsTab::OnApplyClicked()
{
    TArray<FString> Names;
    if (AssetData)     Names.Add(TEXT("SpyAssetData"));
    if (AnimAssetData) Names.Add(TEXT("SpyAnimAssetData"));

    if (!SpyEditorUtils::ConfirmApply(Names)) return FReply::Handled();

    int32 Saved = 0, Failed = 0;
    if (SpyEditorUtils::SaveAsset(AssetData.Get()))     ++Saved; else ++Failed;
    if (SpyEditorUtils::SaveAsset(AnimAssetData.Get())) ++Saved; else ++Failed;

    FMessageDialog::Open(EAppMsgType::Ok,
        FText::FromString(FString::Printf(TEXT("저장 완료: %d개 / 실패: %d개"), Saved, Failed)));
    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
