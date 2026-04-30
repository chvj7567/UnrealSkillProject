#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "GameplayTagContainer.h"

class STextBlock;

class SSpyCreateGADialog : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SSpyCreateGADialog) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    FReply OnCreateClicked();
    bool ValidateInputs(FText& OutError) const;
    void BuildClassList();

    static TSharedRef<SWidget> MakeSectionHeader(const FText& Label);
    static TSharedRef<SWidget> MakeLabeledRow(const FText& Label, TSharedRef<SWidget> Content);

    // 클래스 목록
    TArray<UClass*> ClassList;
    UClass* SelectedClass = nullptr;

    // 이름
    FString GAName;

    // Tags
    FGameplayTagContainer AbilityTagsContainer;
    FGameplayTagContainer ActivationOwnedTagsContainer;
    FGameplayTagContainer ActivationRequiredTagsContainer;
    FGameplayTagContainer ActivationBlockedTagsContainer;
    FGameplayTagContainer CancelAbilitiesWithTagContainer;
    FGameplayTagContainer BlockAbilitiesWithTagContainer;

    // Policies
    TArray<TSharedPtr<FString>> NetExecOptions;
    TArray<TSharedPtr<FString>> InstancingOptions;
    TArray<TSharedPtr<FString>> NetSecOptions;
    TSharedPtr<FString> SelectedNetExecPolicy;
    TSharedPtr<FString> SelectedInstancingPolicy;
    TSharedPtr<FString> SelectedNetSecPolicy;

    // GE Classes (SClassPropertyEntryBox returns const UClass*)
    UClass* CostGEClass     = nullptr;
    UClass* CooldownGEClass = nullptr;

    // 경로 실시간 표시
    TSharedPtr<STextBlock> OutputPathText;
};
