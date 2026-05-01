#pragma once
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FSpyTagManagerToolModule : public IModuleInterface
{
public:
    static const FName TabName;
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
private:
    TSharedRef<SDockTab> OnSpawnTab(const FSpawnTabArgs& SpawnTabArgs);
    void RegisterMenus();
};
