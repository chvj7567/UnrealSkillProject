#include "Customizations/SpyGameplayTagCustomization.h"
#include "DetailWidgetRow.h"
#include "IPropertyHandle.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SVerticalBox.h"
#include "Styling/CoreStyle.h"

TSharedRef<IPropertyTypeCustomization> FSpyGameplayTagCustomization::MakeInstance()
{
    return MakeShareable(new FSpyGameplayTagCustomization());
}

void FSpyGameplayTagCustomization::CustomizeHeader(
    TSharedRef<IPropertyHandle> PropertyHandle,
    FDetailWidgetRow& HeaderRow,
    IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    FString FilterCategory;
    if (const FProperty* Prop = PropertyHandle->GetProperty())
    {
        FilterCategory = Prop->GetMetaData(TEXT("Categories"));
    }

    HeaderRow
    .NameContent()
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()
        [ PropertyHandle->CreatePropertyNameWidget() ]
        + SVerticalBox::Slot().AutoHeight()
        [
            SNew(STextBlock)
            .Text(FText::FromString(FString::Printf(TEXT("범위: %s"),
                FilterCategory.IsEmpty() ? TEXT("전체") : *FilterCategory)))
            .Font(FCoreStyle::GetDefaultFontStyle("Italic", 8))
            .ColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.8f, 1.f)))
        ]
    ]
    .ValueContent()
    [
        PropertyHandle->CreatePropertyValueWidget()
    ];
}
