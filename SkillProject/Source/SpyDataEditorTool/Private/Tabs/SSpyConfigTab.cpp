#include "Tabs/SSpyConfigTab.h"
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

#define LOCTEXT_NAMESPACE "SSpyConfigTab"

static UObject* LoadConfigAsset(UClass* Class)
{
    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
    TArray<FAssetData> Found;
    FARFilter Filter;
    Filter.PackagePaths.Add(TEXT("/Game/Spy/Data/Config"));
    Filter.bRecursivePaths = false;
    Filter.ClassPaths.Add(Class->GetClassPathName());
    Filter.bRecursiveClasses = true;
    Registry.GetAssets(Filter, Found);
    return Found.Num() > 0 ? Found[0].GetAsset() : nullptr;
}

void SSpyConfigTab::LoadAssets()
{
    AIConfig        = TStrongObjectPtr<USpyAIConfig>(Cast<USpyAIConfig>(LoadConfigAsset(USpyAIConfig::StaticClass())));
    CharacterConfig = TStrongObjectPtr<USpyCharacterConfig>(Cast<USpyCharacterConfig>(LoadConfigAsset(USpyCharacterConfig::StaticClass())));
    InputConfig     = TStrongObjectPtr<USpyInputConfig>(Cast<USpyInputConfig>(LoadConfigAsset(USpyInputConfig::StaticClass())));
    MovementConfig  = TStrongObjectPtr<USpyMovementConfig>(Cast<USpyMovementConfig>(LoadConfigAsset(USpyMovementConfig::StaticClass())));

    if (AIConfigView.IsValid())        AIConfigView->SetObject(AIConfig.Get());
    if (CharacterConfigView.IsValid()) CharacterConfigView->SetObject(CharacterConfig.Get());
    if (InputConfigView.IsValid())     InputConfigView->SetObject(InputConfig.Get());
    if (MovementConfigView.IsValid())  MovementConfigView->SetObject(MovementConfig.Get());
}

void SSpyConfigTab::Construct(const FArguments& InArgs)
{
    FPropertyEditorModule& PropModule =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    FDetailsViewArgs Args;
    Args.bHideSelectionTip = true;
    Args.bAllowSearch = false;
    Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;

    AIConfigView        = PropModule.CreateDetailView(Args);
    CharacterConfigView = PropModule.CreateDetailView(Args);
    InputConfigView     = PropModule.CreateDetailView(Args);
    MovementConfigView  = PropModule.CreateDetailView(Args);

    auto PropertyFilter = FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& P) -> bool
    {
        return !P.Property.GetName().EndsWith(TEXT("Cache")) &&
               !P.Property.GetName().EndsWith(TEXT("ToPath"));
    });
    AIConfigView->SetIsPropertyVisibleDelegate(PropertyFilter);
    CharacterConfigView->SetIsPropertyVisibleDelegate(PropertyFilter);
    InputConfigView->SetIsPropertyVisibleDelegate(PropertyFilter);
    MovementConfigView->SetIsPropertyVisibleDelegate(PropertyFilter);

    LoadAssets();

    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(4)
        [
            SNew(SButton)
            .Text(LOCTEXT("Scan", "Scan"))
            .OnClicked(this, &SSpyConfigTab::OnScanClicked)
        ]
        + SVerticalBox::Slot().FillHeight(1.f)
        [
            SNew(SScrollBox)
            + SScrollBox::Slot()
            [ SNew(STextBlock).Text(LOCTEXT("AILabel", "SpyAIConfig")) ]
            + SScrollBox::Slot() [ AIConfigView.ToSharedRef() ]
            + SScrollBox::Slot().Padding(FMargin(0,8,0,0))
            [ SNew(STextBlock).Text(LOCTEXT("CharLabel", "SpyCharacterConfig")) ]
            + SScrollBox::Slot() [ CharacterConfigView.ToSharedRef() ]
            + SScrollBox::Slot().Padding(FMargin(0,8,0,0))
            [ SNew(STextBlock).Text(LOCTEXT("InputLabel", "SpyInputConfig")) ]
            + SScrollBox::Slot() [ InputConfigView.ToSharedRef() ]
            + SScrollBox::Slot().Padding(FMargin(0,8,0,0))
            [ SNew(STextBlock).Text(LOCTEXT("MoveLabel", "SpyMovementConfig")) ]
            + SScrollBox::Slot() [ MovementConfigView.ToSharedRef() ]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(4)
        [
            SNew(SButton)
            .Text(LOCTEXT("Apply", "Apply Config"))
            .OnClicked(this, &SSpyConfigTab::OnApplyClicked)
        ]
    ];

    GEditor->GetTimerManager()->SetTimerForNextTick([this]()
    {
        if (AIConfigView.IsValid())        AIConfigView->ForceRefreshDetails();
        if (CharacterConfigView.IsValid()) CharacterConfigView->ForceRefreshDetails();
        if (InputConfigView.IsValid())     InputConfigView->ForceRefreshDetails();
        if (MovementConfigView.IsValid())  MovementConfigView->ForceRefreshDetails();
    });
}

FReply SSpyConfigTab::OnScanClicked()
{
    LoadAssets();
    return FReply::Handled();
}

FReply SSpyConfigTab::OnApplyClicked()
{
    TArray<FString> Names;
    if (AIConfig)        Names.Add(TEXT("SpyAIConfig"));
    if (CharacterConfig) Names.Add(TEXT("SpyCharacterConfig"));
    if (InputConfig)     Names.Add(TEXT("SpyInputConfig"));
    if (MovementConfig)  Names.Add(TEXT("SpyMovementConfig"));

    if (!SpyEditorUtils::ConfirmApply(Names)) return FReply::Handled();

    int32 Saved = 0, Failed = 0;
    if (SpyEditorUtils::SaveAsset(AIConfig.Get()))        ++Saved; else ++Failed;
    if (SpyEditorUtils::SaveAsset(CharacterConfig.Get())) ++Saved; else ++Failed;
    if (SpyEditorUtils::SaveAsset(InputConfig.Get()))     ++Saved; else ++Failed;
    if (SpyEditorUtils::SaveAsset(MovementConfig.Get()))  ++Saved; else ++Failed;

    FMessageDialog::Open(EAppMsgType::Ok,
        FText::FromString(FString::Printf(TEXT("저장 완료: %d개 / 실패: %d개"), Saved, Failed)));
    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
