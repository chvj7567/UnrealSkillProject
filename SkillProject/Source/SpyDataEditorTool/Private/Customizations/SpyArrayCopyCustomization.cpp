#include "Customizations/SpyArrayCopyCustomization.h"
#include "PropertyCustomizationHelpers.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "UObject/UnrealType.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FSpyArrayCopyCustomization"

TSharedRef<IDetailCustomization> FSpyArrayCopyCustomization::MakeInstance()
{
    return MakeShareable(new FSpyArrayCopyCustomization());
}

void FSpyArrayCopyCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    TArray<TWeakObjectPtr<UObject>> Objects;
    DetailBuilder.GetObjectsBeingCustomized(Objects);
    if (Objects.Num() == 0 || Objects[0].IsValid() == false) return;

    UClass* Class = Objects[0]->GetClass();

    for (TFieldIterator<FArrayProperty> It(Class); It; ++It)
    {
        FArrayProperty* ArrayProp = *It;
        TSharedRef<IPropertyHandle> ArrayHandle =
            DetailBuilder.GetProperty(ArrayProp->GetFName(), Class);

        if (ArrayHandle->IsValidHandle() == false) continue;

        DetailBuilder.HideProperty(ArrayHandle);

        FString CategoryName = ArrayProp->GetMetaData(TEXT("Category"));
        FString TopLevel;
        if (CategoryName.Split(TEXT("|"), &TopLevel, nullptr))
        {
            CategoryName = TopLevel;
        }
        if (CategoryName.IsEmpty()) CategoryName = TEXT("Default");

        IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(*CategoryName);
        AddCopyButtonToArrayProperty(ArrayHandle, Category);
    }
}

void FSpyArrayCopyCustomization::AddCopyButtonToArrayProperty(
    TSharedRef<IPropertyHandle> ArrayHandle,
    IDetailCategoryBuilder& Category)
{
    TSharedRef<FDetailArrayBuilder> ArrayBuilder =
        MakeShareable(new FDetailArrayBuilder(ArrayHandle));

    ArrayBuilder->OnGenerateArrayElementWidget(
        FOnGenerateArrayElementWidget::CreateStatic(
            &FSpyArrayCopyCustomization::GenerateElementWidget,
            ArrayHandle));

    Category.AddCustomBuilder(ArrayBuilder);
}

void FSpyArrayCopyCustomization::GenerateElementWidget(
    TSharedRef<IPropertyHandle> ElementHandle,
    int32 ElementIndex,
    IDetailChildrenBuilder& ChildrenBuilder,
    TSharedRef<IPropertyHandle> ArrayHandle)
{
    ChildrenBuilder.AddProperty(ElementHandle);

    ChildrenBuilder.AddCustomRow(LOCTEXT("CopyRow", "Copy"))
    .ValueContent()
    [
        SNew(SButton)
        .Text(LOCTEXT("CopyBtn", "Copy Entry"))
        .ToolTipText(LOCTEXT("CopyTooltip", "이 엔트리를 복제해 배열 끝에 추가합니다"))
        .OnClicked_Lambda([ElementHandle, ArrayHandle]() -> FReply
        {
            FString ExportedValue;
            ElementHandle->GetValueAsFormattedString(ExportedValue, PPF_Copy);

            TSharedPtr<IPropertyHandleArray> ArrayHandleArray = ArrayHandle->AsArray();
            if (ArrayHandleArray.IsValid() == false) return FReply::Handled();

            ArrayHandleArray->AddItem();

            uint32 NumElements = 0;
            ArrayHandleArray->GetNumElements(NumElements);
            if (NumElements > 0)
            {
                TSharedRef<IPropertyHandle> NewElement =
                    ArrayHandleArray->GetElement(NumElements - 1);
                NewElement->SetValueFromFormattedString(ExportedValue);
            }

            return FReply::Handled();
        })
    ];
}

#undef LOCTEXT_NAMESPACE
