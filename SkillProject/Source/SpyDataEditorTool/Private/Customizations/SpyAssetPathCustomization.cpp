#include "Customizations/SpyAssetPathCustomization.h"
#include "DetailWidgetRow.h"
#include "IPropertyHandle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "PropertyCustomizationHelpers.h"
#include "AssetRegistry/AssetData.h"

TSet<FString> FSpyAssetPathCustomization::ChangedPaths;

TSharedRef<IPropertyTypeCustomization> FSpyAssetPathCustomization::MakeInstance()
{
    return MakeShareable(new FSpyAssetPathCustomization());
}

void FSpyAssetPathCustomization::MarkAsChanged(const FString& PropertyPath)
{
    ChangedPaths.Add(PropertyPath);
}

void FSpyAssetPathCustomization::ClearChangedMarks()
{
    ChangedPaths.Empty();
}

void FSpyAssetPathCustomization::CustomizeHeader(
    TSharedRef<IPropertyHandle> PropertyHandle,
    FDetailWidgetRow& HeaderRow,
    IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    FString PropPath = PropertyHandle->GeneratePathToProperty();
    bool bChanged = ChangedPaths.Contains(PropPath);

    // 기본 값 위젯을 사용하되 하이라이트만 오버레이
    TSharedRef<SWidget> ValueWidget = PropertyHandle->CreatePropertyValueWidget();

    HeaderRow
    .NameContent()
    [
        PropertyHandle->CreatePropertyNameWidget()
    ]
    .ValueContent()
    [
        SNew(SBorder)
        .BorderBackgroundColor(bChanged
            ? FLinearColor(1.f, 0.9f, 0.f, 0.4f)
            : FLinearColor::Transparent)
        .Padding(bChanged ? FMargin(2.f) : FMargin(0.f))
        [
            ValueWidget
        ]
    ];
}
