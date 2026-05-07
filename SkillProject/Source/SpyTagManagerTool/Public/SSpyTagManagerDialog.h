#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"
#include "SpyTagFileEditor.h"

struct FTagTreeNode
{
    FString Segment;
    FString FullPath;
    TArray<TSharedPtr<FTagTreeNode>> Children;
    bool bIsGroupHeader = false;
    FString GroupComment;
};

class SSpyTagManagerDialog : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SSpyTagManagerDialog) {}
    SLATE_END_ARGS()
    void Construct(const FArguments& InArgs);

private:
    //# ── Data ──
    TArray<FSpyTagGroup> ParsedGroups;
    TArray<TSharedPtr<FTagTreeNode>> RootNodes;
    TArray<TSharedPtr<FSpyTagGroup>> GroupOptions;
    TSharedPtr<FSpyTagGroup> SelectedGroupOption;

    //# ── Add panel state ──
    FString ParentPath;
    FString NewGroupComment;
    TArray<TSharedPtr<FString>> LeafInputs;

    //# ── Widgets ──
    TSharedPtr<STreeView<TSharedPtr<FTagTreeNode>>> TagTreeView;
    TSharedPtr<SComboBox<TSharedPtr<FSpyTagGroup>>> GroupComboBox;
    TSharedPtr<SEditableTextBox> ParentPathBox;
    TSharedPtr<SEditableTextBox> GroupCommentBox;
    TSharedPtr<SVerticalBox> LeafInputBox;

    //# ── Helpers ──
    void RebuildData();
    TArray<TSharedPtr<FTagTreeNode>> BuildTreeNodes(const TArray<FSpyTagGroup>& Groups);
    void InsertTagIntoTree(TSharedPtr<FTagTreeNode> Root, const FString& TagString);
    TSharedRef<SWidget> BuildAddPanel();
    void RebuildLeafInputs();

    //# ── Tree callbacks ──
    TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FTagTreeNode> Node, const TSharedRef<STableViewBase>& Owner);
    void OnGetChildren(TSharedPtr<FTagTreeNode> Node, TArray<TSharedPtr<FTagTreeNode>>& Out);
    void OnTreeSelectionChanged(TSharedPtr<FTagTreeNode> Node, ESelectInfo::Type Type);

    //# ── Button / combo callbacks ──
    FReply OnRefreshClicked();
    FReply OnAddTagsClicked();
    FReply OnAddLeafRowClicked();
    FReply OnSaveGroupCommentClicked();
    TSharedRef<SWidget> OnGenerateGroupWidget(TSharedPtr<FSpyTagGroup> Item);
    FText GetGroupComboText() const;

    static const FString NewGroupSentinel;
};
