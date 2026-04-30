#include "SpyGACreatorTool.h"
#include "SSpyCreateGADialog.h"
#include "Framework/Docking/TabManager.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FSpyGACreatorToolModule"

const FName FSpyGACreatorToolModule::TabName = TEXT("SpyGACreatorTool");

void FSpyGACreatorToolModule::StartupModule()
{
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        TabName,
        FOnSpawnTab::CreateRaw(this, &FSpyGACreatorToolModule::OnSpawnTab))
        .SetDisplayName(LOCTEXT("TabTitle", "Spy GA Creator"))
        .SetTooltipText(LOCTEXT("TabTooltip", "Gameplay Ability Blueprint 생성 도구"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);

    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(
            this, &FSpyGACreatorToolModule::RegisterMenus));
}

void FSpyGACreatorToolModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);
}

TSharedRef<SDockTab> FSpyGACreatorToolModule::OnSpawnTab(const FSpawnTabArgs& SpawnTabArgs)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SSpyCreateGADialog)
        ];
}

void FSpyGACreatorToolModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);
    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
    if (!Menu) return;
    FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
    Section.AddMenuEntry(
        "SpyGACreatorOpen",
        LOCTEXT("MenuEntry", "Spy GA Creator"),
        LOCTEXT("MenuEntryTooltip", "Gameplay Ability Blueprint 생성 도구 열기"),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateLambda([]()
        {
            FGlobalTabmanager::Get()->TryInvokeTab(FSpyGACreatorToolModule::TabName);
        }))
    );
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSpyGACreatorToolModule, SpyGACreatorTool)
