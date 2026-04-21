#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "UObject/StrongObjectPtr.h"
#include "Data/SpyCharacterAssetData.h"
#include "Data/SpyAbilityData.h"
#include "Data/SpyComboAssetData.h"

class IDetailsView;

class SSpyAbilityTab : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SSpyAbilityTab) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    FReply OnScanClicked();
    FReply OnApplyClicked();

    TSharedPtr<IDetailsView> CharacterAssetView;
    TSharedPtr<IDetailsView> CommonAbilityView;
    TSharedPtr<IDetailsView> NormalAbilityView;
    TSharedPtr<IDetailsView> ComboAssetView;

    TStrongObjectPtr<USpyCharacterAssetData> CharacterAssetData;
    TStrongObjectPtr<USpyAbilityData>        CommonAbility;
    TStrongObjectPtr<USpyAbilityData>        NormalAbility;
    TStrongObjectPtr<USpyComboAssetData>     ComboAssetData;
};
