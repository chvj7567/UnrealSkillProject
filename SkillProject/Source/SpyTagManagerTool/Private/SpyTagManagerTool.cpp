#include "SpyTagManagerTool.h"
#include "Framework/Docking/TabManager.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "SSpyTagManagerDialog.h"

#define LOCTEXT_NAMESPACE "FSpyTagManagerToolModule"

const FName FSpyTagManagerToolModule::TabName = TEXT("SpyTagManagerTool");

void FSpyTagManagerToolModule::StartupModule()
{
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        TabName,
        FOnSpawnTab::CreateRaw(this, &FSpyTagManagerToolModule::OnSpawnTab))
        .SetDisplayName(LOCTEXT("TabTitle", "Spy Tag Manager"))
        .SetTooltipText(LOCTEXT("TabTooltip", "SpyGameplayTags 태그 관리 도구"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);

    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(
            this, &FSpyTagManagerToolModule::RegisterMenus));
}

void FSpyTagManagerToolModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);
}

TSharedRef<SDockTab> FSpyTagManagerToolModule::OnSpawnTab(const FSpawnTabArgs& SpawnTabArgs)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SSpyTagManagerDialog)
        ];
}

void FSpyTagManagerToolModule::RegisterMenus()
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
            "SpyTagManagerOpen",
            LOCTEXT("MenuEntry", "Spy Tag Manager"),
            LOCTEXT("MenuEntryTooltip", "SpyGameplayTags 태그 관리 도구 열기"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateLambda([]()
            {
                FGlobalTabmanager::Get()->TryInvokeTab(FSpyTagManagerToolModule::TabName);
            }))
        );
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSpyTagManagerToolModule, SpyTagManagerTool)
