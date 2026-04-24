#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "DetailCategoryBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"

class FSpyArrayCopyCustomization : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();
    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
    static void AddCopyButtonToArrayProperty(
        TSharedRef<IPropertyHandle> ArrayHandle,
        IDetailCategoryBuilder& Category);

    static void GenerateElementWidget(
        TSharedRef<IPropertyHandle> ElementHandle,
        int32 ElementIndex,
        IDetailChildrenBuilder& ChildrenBuilder,
        TSharedRef<IPropertyHandle> ArrayHandle);
};
