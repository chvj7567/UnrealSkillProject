#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "UObject/StrongObjectPtr.h"
#include "Data/SpyAIConfig.h"
#include "Data/SpyCharacterConfig.h"
#include "Data/SpyMovementConfig.h"
#include "Input/SpyInputConfig.h"

class IDetailsView;

class SSpyConfigTab : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SSpyConfigTab) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    void LoadAssets();
    FReply OnScanClicked();
    FReply OnApplyClicked();

    TSharedPtr<IDetailsView> AIConfigView;
    TSharedPtr<IDetailsView> CharacterConfigView;
    TSharedPtr<IDetailsView> InputConfigView;
    TSharedPtr<IDetailsView> MovementConfigView;

    TStrongObjectPtr<USpyAIConfig>        AIConfig;
    TStrongObjectPtr<USpyCharacterConfig> CharacterConfig;
    TStrongObjectPtr<USpyInputConfig>     InputConfig;
    TStrongObjectPtr<USpyMovementConfig>  MovementConfig;
};
