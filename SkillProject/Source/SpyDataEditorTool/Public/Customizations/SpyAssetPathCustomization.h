#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

class FSpyAssetPathCustomization : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle,
        FDetailWidgetRow& HeaderRow,
        IPropertyTypeCustomizationUtils& CustomizationUtils) override;

    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle,
        IDetailChildrenBuilder& ChildBuilder,
        IPropertyTypeCustomizationUtils& CustomizationUtils) override {}

    static void MarkAsChanged(const FString& PropertyPath);
    static void ClearChangedMarks();

private:
    static TSet<FString> ChangedPaths;
};
