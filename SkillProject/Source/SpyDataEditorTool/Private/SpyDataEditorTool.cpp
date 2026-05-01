#include "SpyDataEditorTool.h"
#include "Framework/Docking/TabManager.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Text/STextBlock.h"
#include "Tabs/SSpyAssetsTab.h"
#include "Tabs/SSpyAbilityTab.h"
#include "Tabs/SSpyConfigTab.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Customizations/SpyAssetPathCustomization.h"
#include "Customizations/SpyGameplayTagCustomization.h"
#include "Customizations/SpyArrayCopyCustomization.h"
#include "PropertyEditorModule.h"
#include "Data/SpyCharacterAssetData.h"
#include "Data/SpyAbilityData.h"
#include "Data/SpyComboAssetData.h"

#define LOCTEXT_NAMESPACE "FSpyDataEditorToolModule"

const FName FSpyDataEditorToolModule::TabName = TEXT("SpyDataEditorTool");

void FSpyDataEditorToolModule::StartupModule()
{
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        TabName,
        FOnSpawnTab::CreateRaw(this, &FSpyDataEditorToolModule::OnSpawnTab))
        .SetDisplayName(LOCTEXT("TabTitle", "Spy Data Editor"))
        .SetTooltipText(LOCTEXT("TabTooltip", "Spy Data 에셋 일괄 편집 도구"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);

    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FSpyDataEditorToolModule::RegisterMenus));

    FPropertyEditorModule& PropModule =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    PropModule.RegisterCustomPropertyTypeLayout(
        TEXT("SoftObjectPath"),
        FOnGetPropertyTypeCustomizationInstance::CreateStatic(
            &FSpyAssetPathCustomization::MakeInstance));
    PropModule.RegisterCustomPropertyTypeLayout(
        TEXT("GameplayTag"),
        FOnGetPropertyTypeCustomizationInstance::CreateStatic(
            &FSpyGameplayTagCustomization::MakeInstance));

    const auto CopyFactory = FOnGetDetailCustomizationInstance::CreateStatic(
        &FSpyArrayCopyCustomization::MakeInstance);
    PropModule.RegisterCustomClassLayout(USpyCharacterAssetData::StaticClass()->GetFName(), CopyFactory);
    PropModule.RegisterCustomClassLayout(USpyAbilityData::StaticClass()->GetFName(), CopyFactory);
    PropModule.RegisterCustomClassLayout(USpyComboAssetData::StaticClass()->GetFName(), CopyFactory);

    PropModule.NotifyCustomizationModuleChanged();
}

void FSpyDataEditorToolModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);

    if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        FPropertyEditorModule& PropModule =
            FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
        PropModule.UnregisterCustomPropertyTypeLayout(TEXT("SoftObjectPath"));
        PropModule.UnregisterCustomPropertyTypeLayout(TEXT("GameplayTag"));
        PropModule.UnregisterCustomClassLayout(USpyCharacterAssetData::StaticClass()->GetFName());
        PropModule.UnregisterCustomClassLayout(USpyAbilityData::StaticClass()->GetFName());
        PropModule.UnregisterCustomClassLayout(USpyComboAssetData::StaticClass()->GetFName());
    }

    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);
}

TSharedRef<SDockTab> FSpyDataEditorToolModule::OnSpawnTab(const FSpawnTabArgs& SpawnTabArgs)
{
    TSharedRef<SSpyAssetsTab>  AssetsTab  = SNew(SSpyAssetsTab);
    TSharedRef<SSpyAbilityTab> AbilityTab = SNew(SSpyAbilityTab);
    TSharedRef<SSpyConfigTab>  ConfigTab  = SNew(SSpyConfigTab);

    TSharedRef<SWidgetSwitcher> Switcher = SNew(SWidgetSwitcher)
        + SWidgetSwitcher::Slot() [ AssetsTab ]
        + SWidgetSwitcher::Slot() [ AbilityTab ]
        + SWidgetSwitcher::Slot() [ ConfigTab ];

    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SButton)
                    .Text(LOCTEXT("TabAssets", "Assets"))
                    .ToolTipText(LOCTEXT("TabAssetsTip", "Spy/Data/ — SpyAssetData, SpyAnimAssetData"))
                    .OnClicked_Lambda([Switcher]() { Switcher->SetActiveWidgetIndex(0); return FReply::Handled(); })
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SButton)
                    .Text(LOCTEXT("TabAbility", "Ability"))
                    .ToolTipText(LOCTEXT("TabAbilityTip", "Spy/Data/ — CharacterAssetData, Ability, Combo"))
                    .OnClicked_Lambda([Switcher]() { Switcher->SetActiveWidgetIndex(1); return FReply::Handled(); })
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SButton)
                    .Text(LOCTEXT("TabConfig", "Config"))
                    .ToolTipText(LOCTEXT("TabConfigTip", "Spy/Data/Config/ — AI/Character/Input/Movement"))
                    .OnClicked_Lambda([Switcher]() { Switcher->SetActiveWidgetIndex(2); return FReply::Handled(); })
                ]
            ]
            + SVerticalBox::Slot().FillHeight(1.f)
            [
                Switcher
            ]
        ];
}

void FSpyDataEditorToolModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);

    UToolMenu* MainMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu");
    if (MainMenu)
    {
        FToolMenuSection& Section = MainMenu->FindOrAddSection("SpyTools");
        Section.AddSubMenu(
            "SpyTools",
            LOCTEXT("SpyToolsMenu", "Spy Tools"),
            LOCTEXT("SpyToolsMenuTooltip", "Spy 개발 도구 모음"),
            FNewToolMenuDelegate::CreateLambda([](UToolMenu*) {})
        );
    }

    UToolMenu* SpyMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.SpyTools");
    if (SpyMenu)
    {
        FToolMenuSection& Section = SpyMenu->FindOrAddSection("SpyToolsSection");
        Section.AddMenuEntry(
            "SpyDataEditorOpen",
            LOCTEXT("MenuEntry", "Spy Data Editor"),
            LOCTEXT("MenuEntryTooltip", "Spy Data 에셋 일괄 편집 도구 열기"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateLambda([]()
            {
                FGlobalTabmanager::Get()->TryInvokeTab(FSpyDataEditorToolModule::TabName);
            }))
        );
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSpyDataEditorToolModule, SpyDataEditorTool)
