#include "Tabs/SSpyAbilityTab.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/MessageDialog.h"
#include "Utils/SpyEditorUtils.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "SSpyAbilityTab"

static UObject* LoadAssetByClassAndName(UClass* Class, const FString& AssetName)
{
    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
    TArray<FAssetData> Found;
    FARFilter Filter;
    Filter.PackagePaths.Add(TEXT("/Game/Spy/Data"));
    Filter.bRecursivePaths = false;
    Filter.ClassPaths.Add(Class->GetClassPathName());
    Filter.bRecursiveClasses = true;
    Registry.GetAssets(Filter, Found);
    for (const FAssetData& A : Found)
    {
        if (A.AssetName.ToString() == AssetName) return A.GetAsset();
    }
    return nullptr;
}

void SSpyAbilityTab::Construct(const FArguments& InArgs)
{
    FPropertyEditorModule& PropModule =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    FDetailsViewArgs Args;
    Args.bHideSelectionTip = true;
    Args.bAllowSearch = false;
    Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;

    CharacterAssetView = PropModule.CreateDetailView(Args);
    CommonAbilityView  = PropModule.CreateDetailView(Args);
    NormalAbilityView  = PropModule.CreateDetailView(Args);
    ComboAssetView     = PropModule.CreateDetailView(Args);

    auto PropertyFilter = FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& P) -> bool
    {
        return P.Property.GetName().EndsWith(TEXT("Cache")) == false &&
               P.Property.GetName().EndsWith(TEXT("ToPath")) == false;
    });
    CharacterAssetView->SetIsPropertyVisibleDelegate(PropertyFilter);
    CommonAbilityView->SetIsPropertyVisibleDelegate(PropertyFilter);
    NormalAbilityView->SetIsPropertyVisibleDelegate(PropertyFilter);
    ComboAssetView->SetIsPropertyVisibleDelegate(PropertyFilter);

    CharacterAssetData = TStrongObjectPtr<USpyCharacterAssetData>(Cast<USpyCharacterAssetData>(
        LoadAssetByClassAndName(USpyCharacterAssetData::StaticClass(), TEXT("SpyCharacterAssetData"))));
    CommonAbility = TStrongObjectPtr<USpyAbilityData>(Cast<USpyAbilityData>(
        LoadAssetByClassAndName(USpyAbilityData::StaticClass(), TEXT("SpyCommonCharacterAbility"))));
    NormalAbility = TStrongObjectPtr<USpyAbilityData>(Cast<USpyAbilityData>(
        LoadAssetByClassAndName(USpyAbilityData::StaticClass(), TEXT("SpyNormalAbility"))));
    ComboAssetData = TStrongObjectPtr<USpyComboAssetData>(Cast<USpyComboAssetData>(
        LoadAssetByClassAndName(USpyComboAssetData::StaticClass(), TEXT("SpyNormalComboAssetData"))));

    if (CharacterAssetData) CharacterAssetView->SetObject(CharacterAssetData.Get());
    if (CommonAbility)      CommonAbilityView->SetObject(CommonAbility.Get());
    if (NormalAbility)      NormalAbilityView->SetObject(NormalAbility.Get());
    if (ComboAssetData)     ComboAssetView->SetObject(ComboAssetData.Get());

    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(4)
        [
            SNew(SButton)
            .Text(LOCTEXT("Scan", "Scan"))
            .OnClicked(this, &SSpyAbilityTab::OnScanClicked)
        ]
        + SVerticalBox::Slot().FillHeight(1.f)
        [
            SNew(SScrollBox)
            + SScrollBox::Slot()
            [ SNew(STextBlock).Text(LOCTEXT("CharLabel", "SpyCharacterAssetData")) ]
            + SScrollBox::Slot() [ CharacterAssetView.ToSharedRef() ]
            + SScrollBox::Slot().Padding(FMargin(0,8,0,0))
            [ SNew(STextBlock).Text(LOCTEXT("CommonLabel", "SpyCommonCharacterAbility")) ]
            + SScrollBox::Slot() [ CommonAbilityView.ToSharedRef() ]
            + SScrollBox::Slot().Padding(FMargin(0,8,0,0))
            [ SNew(STextBlock).Text(LOCTEXT("NormalLabel", "SpyNormalAbility")) ]
            + SScrollBox::Slot() [ NormalAbilityView.ToSharedRef() ]
            + SScrollBox::Slot().Padding(FMargin(0,8,0,0))
            [ SNew(STextBlock).Text(LOCTEXT("ComboLabel", "SpyNormalComboAssetData")) ]
            + SScrollBox::Slot() [ ComboAssetView.ToSharedRef() ]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(4)
        [
            SNew(SButton)
            .Text(LOCTEXT("Apply", "Apply Abilities"))
            .OnClicked(this, &SSpyAbilityTab::OnApplyClicked)
        ]
    ];

    GEditor->GetTimerManager()->SetTimerForNextTick([this]()
    {
        if (CharacterAssetView.IsValid()) CharacterAssetView->ForceRefresh();
        if (CommonAbilityView.IsValid())  CommonAbilityView->ForceRefresh();
        if (NormalAbilityView.IsValid())  NormalAbilityView->ForceRefresh();
        if (ComboAssetView.IsValid())     ComboAssetView->ForceRefresh();
    });
}

FReply SSpyAbilityTab::OnScanClicked() { return FReply::Handled(); }

FReply SSpyAbilityTab::OnApplyClicked()
{
    TArray<FString> Names;
    if (CharacterAssetData) Names.Add(TEXT("SpyCharacterAssetData"));
    if (CommonAbility)      Names.Add(TEXT("SpyCommonCharacterAbility"));
    if (NormalAbility)      Names.Add(TEXT("SpyNormalAbility"));
    if (ComboAssetData)     Names.Add(TEXT("SpyNormalComboAssetData"));

    if (SpyEditorUtils::ConfirmApply(Names) == false) return FReply::Handled();

    int32 Saved = 0, Failed = 0;
    if (SpyEditorUtils::SaveAsset(CharacterAssetData.Get())) ++Saved; else ++Failed;
    if (SpyEditorUtils::SaveAsset(CommonAbility.Get()))      ++Saved; else ++Failed;
    if (SpyEditorUtils::SaveAsset(NormalAbility.Get()))      ++Saved; else ++Failed;
    if (SpyEditorUtils::SaveAsset(ComboAssetData.Get()))     ++Saved; else ++Failed;

    FMessageDialog::Open(EAppMsgType::Ok,
        FText::FromString(FString::Printf(TEXT("저장 완료: %d개 / 실패: %d개"), Saved, Failed)));
    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
