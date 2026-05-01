#include "SSpyTagManagerDialog.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"
#include "Misc/MessageDialog.h"
#include "SpyTagFileEditor.h"

#define LOCTEXT_NAMESPACE "SSpyTagManagerDialog"

const FString SSpyTagManagerDialog::NewGroupSentinel = TEXT("__NEW_GROUP__");

void SSpyTagManagerDialog::RebuildData()
{
    ParsedGroups = FSpyTagFileEditor::ParseFiles();

    GroupOptions.Empty();
    for (FSpyTagGroup& G : ParsedGroups)
        GroupOptions.Add(MakeShared<FSpyTagGroup>(G));
    TSharedPtr<FSpyTagGroup> Sentinel = MakeShared<FSpyTagGroup>();
    Sentinel->Comment = NewGroupSentinel;
    GroupOptions.Add(Sentinel);

    if (!GroupOptions.IsEmpty())
        SelectedGroupOption = GroupOptions[0];

    RootNodes = BuildTreeNodes(ParsedGroups);
}

static void PropagateGroupComment(TSharedPtr<FTagTreeNode> Node, const FString& Comment)
{
    Node->GroupComment = Comment;
    for (auto& Child : Node->Children)
        PropagateGroupComment(Child, Comment);
}

TArray<TSharedPtr<FTagTreeNode>> SSpyTagManagerDialog::BuildTreeNodes(
    const TArray<FSpyTagGroup>& Groups)
{
    TArray<TSharedPtr<FTagTreeNode>> Result;
    for (const FSpyTagGroup& Group : Groups)
    {
        TSharedPtr<FTagTreeNode> Header = MakeShared<FTagTreeNode>();
        Header->bIsGroupHeader = true;
        Header->GroupComment   = Group.Comment;
        Header->Segment        = Group.Comment;
        for (const FSpyTagEntry& Entry : Group.Tags)
            if (!Entry.TagString.IsEmpty())
                InsertTagIntoTree(Header, Entry.TagString);
        for (auto& Child : Header->Children)
            PropagateGroupComment(Child, Group.Comment);
        Result.Add(Header);
    }
    return Result;
}

void SSpyTagManagerDialog::InsertTagIntoTree(
    TSharedPtr<FTagTreeNode> Root, const FString& TagString)
{
    TArray<FString> Parts;
    TagString.ParseIntoArray(Parts, TEXT("."), true);

    TSharedPtr<FTagTreeNode> Current = Root;
    FString PathSoFar;
    for (const FString& Part : Parts)
    {
        PathSoFar = PathSoFar.IsEmpty() ? Part : PathSoFar + TEXT(".") + Part;
        TSharedPtr<FTagTreeNode>* Found = Current->Children.FindByPredicate(
            [&Part](const TSharedPtr<FTagTreeNode>& N){ return N->Segment == Part; });
        if (Found)
        {
            Current = *Found;
        }
        else
        {
            TSharedPtr<FTagTreeNode> Node = MakeShared<FTagTreeNode>();
            Node->Segment  = Part;
            Node->FullPath = PathSoFar;
            Current->Children.Add(Node);
            Current = Node;
        }
    }
}

void SSpyTagManagerDialog::Construct(const FArguments& InArgs)
{
    RebuildData();

    ChildSlot
    [
        SNew(SSplitter).Orientation(Orient_Horizontal)
        + SSplitter::Slot().Value(0.45f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(4.f)
            [
                SNew(SButton)
                .HAlign(HAlign_Center)
                .Text(LOCTEXT("Refresh", "새로고침"))
                .OnClicked(this, &SSpyTagManagerDialog::OnRefreshClicked)
            ]
            + SVerticalBox::Slot().FillHeight(1.f)
            [
                SAssignNew(TagTreeView, STreeView<TSharedPtr<FTagTreeNode>>)
                .TreeItemsSource(&RootNodes)
                .OnGetChildren(this, &SSpyTagManagerDialog::OnGetChildren)
                .OnGenerateRow(this, &SSpyTagManagerDialog::OnGenerateRow)
                .OnSelectionChanged(this, &SSpyTagManagerDialog::OnTreeSelectionChanged)
                .SelectionMode(ESelectionMode::Single)
            ]
        ]
        + SSplitter::Slot().Value(0.55f)
        [
            BuildAddPanel()
        ]
    ];

    for (const TSharedPtr<FTagTreeNode>& Node : RootNodes)
        TagTreeView->SetItemExpansion(Node, true);
}

TSharedRef<ITableRow> SSpyTagManagerDialog::OnGenerateRow(
    TSharedPtr<FTagTreeNode> Node, const TSharedRef<STableViewBase>& Owner)
{
    if (Node->bIsGroupHeader)
    {
        return SNew(STableRow<TSharedPtr<FTagTreeNode>>, Owner)
            [
                SNew(SBorder)
                .BorderBackgroundColor(FLinearColor(0.08f, 0.08f, 0.08f, 1.f))
                .Padding(FMargin(4.f, 3.f))
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(FString::Printf(TEXT("# %s"), *Node->GroupComment)))
                    .Font(FAppStyle::GetFontStyle("BoldFont"))
                ]
            ];
    }
    return SNew(STableRow<TSharedPtr<FTagTreeNode>>, Owner)
        [
            SNew(STextBlock)
            .Text(FText::FromString(Node->Segment))
            .Margin(FMargin(2.f, 1.f))
        ];
}

void SSpyTagManagerDialog::OnGetChildren(
    TSharedPtr<FTagTreeNode> Node, TArray<TSharedPtr<FTagTreeNode>>& Out)
{
    Out = Node->Children;
}

void SSpyTagManagerDialog::OnTreeSelectionChanged(
    TSharedPtr<FTagTreeNode> Node, ESelectInfo::Type)
{
    if (!Node) return;

    // 그룹 콤보박스 & 주석 동기화
    const FString& GroupComment = Node->GroupComment;
    TSharedPtr<FSpyTagGroup>* Found = GroupOptions.FindByPredicate(
        [&GroupComment](const TSharedPtr<FSpyTagGroup>& G)
        { return G.IsValid() && G->Comment == GroupComment; });

    if (Found)
    {
        SelectedGroupOption = *Found;
        if (GroupComboBox.IsValid())
            GroupComboBox->SetSelectedItem(SelectedGroupOption);
        if (GroupCommentBox.IsValid())
        {
            GroupCommentBox->SetEnabled(false);
            GroupCommentBox->SetText(FText::FromString(GroupComment));
        }
    }

    // 태그 노드일 때만 부모 경로 갱신
    if (!Node->bIsGroupHeader)
    {
        ParentPath = Node->FullPath;
        if (ParentPathBox.IsValid())
            ParentPathBox->SetText(FText::FromString(ParentPath));
    }
}

FReply SSpyTagManagerDialog::OnRefreshClicked()
{
    RebuildData();
    if (TagTreeView.IsValid())
    {
        TagTreeView->RequestTreeRefresh();
        for (const TSharedPtr<FTagTreeNode>& Node : RootNodes)
            TagTreeView->SetItemExpansion(Node, true);
    }
    if (GroupComboBox.IsValid())
        GroupComboBox->SetSelectedItem(SelectedGroupOption);
    return FReply::Handled();
}

TSharedRef<SWidget> SSpyTagManagerDialog::BuildAddPanel()
{
    LeafInputs.Empty();
    LeafInputs.Add(MakeShared<FString>());

    TSharedPtr<SVerticalBox> Panel;
    SAssignNew(Panel, SVerticalBox);

    auto MakeRow = [](const FText& Label, TSharedRef<SWidget> Content) -> TSharedRef<SWidget>
    {
        return SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
            [
                SNew(SBox).WidthOverride(130.f)[ SNew(STextBlock).Text(Label) ]
            ]
            + SHorizontalBox::Slot().FillWidth(1.f).Padding(4.f, 0.f)
            [ Content ];
    };

    // 그룹 선택
    Panel->AddSlot().AutoHeight().Padding(4.f, 4.f)
    [
        MakeRow(LOCTEXT("Group", "그룹"),
            SAssignNew(GroupComboBox, SComboBox<TSharedPtr<FSpyTagGroup>>)
            .OptionsSource(&GroupOptions)
            .InitiallySelectedItem(SelectedGroupOption)
            .OnSelectionChanged_Lambda([this](TSharedPtr<FSpyTagGroup> Item, ESelectInfo::Type)
            {
                SelectedGroupOption = Item;
                bool bNew = Item.IsValid() && Item->Comment == NewGroupSentinel;
                if (GroupCommentBox.IsValid())
                {
                    GroupCommentBox->SetEnabled(bNew);
                    GroupCommentBox->SetText(FText::FromString(
                        bNew ? TEXT("") : (Item.IsValid() ? Item->Comment : TEXT(""))));
                }
            })
            .OnGenerateWidget(this, &SSpyTagManagerDialog::OnGenerateGroupWidget)
            [ SNew(STextBlock).Text(this, &SSpyTagManagerDialog::GetGroupComboText) ]
        )
    ];

    // 그룹 주석
    Panel->AddSlot().AutoHeight().Padding(4.f, 2.f)
    [
        MakeRow(LOCTEXT("Comment", "그룹 주석"),
            SAssignNew(GroupCommentBox, SEditableTextBox)
            .IsReadOnly(true)
            .Text(FText::FromString(
                SelectedGroupOption.IsValid() ? SelectedGroupOption->Comment : TEXT("")))
            .OnTextChanged_Lambda([this](const FText& T){ NewGroupComment = T.ToString(); })
        )
    ];

    // 부모 경로
    Panel->AddSlot().AutoHeight().Padding(4.f, 2.f)
    [
        MakeRow(LOCTEXT("Parent", "부모 경로"),
            SAssignNew(ParentPathBox, SEditableTextBox)
            .HintText(LOCTEXT("ParentHint", "예: Skill.Action  (트리 클릭 시 자동 입력)"))
            .OnTextChanged_Lambda([this](const FText& T){ ParentPath = T.ToString(); })
        )
    ];

    // 리프 이름 헤더
    Panel->AddSlot().AutoHeight().Padding(4.f, 10.f, 4.f, 2.f)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
        [ SNew(STextBlock).Text(LOCTEXT("Leaves", "리프 이름")) ]
        + SHorizontalBox::Slot().AutoWidth()
        [
            SNew(SButton).Text(LOCTEXT("AddRow", "+"))
            .OnClicked(this, &SSpyTagManagerDialog::OnAddLeafRowClicked)
        ]
    ];

    // 리프 입력 박스
    Panel->AddSlot().AutoHeight().Padding(4.f, 0.f)
    [ SAssignNew(LeafInputBox, SVerticalBox) ];
    RebuildLeafInputs();

    // 미리보기
    Panel->AddSlot().AutoHeight().Padding(4.f, 10.f, 4.f, 2.f)
    [ SNew(STextBlock).Text(LOCTEXT("Preview", "미리보기")) ];

    Panel->AddSlot().AutoHeight().Padding(4.f, 0.f)
    [
        SNew(SBorder)
        .BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.05f, 1.f))
        .Padding(FMargin(6.f, 4.f))
        [
            SNew(STextBlock)
            .AutoWrapText(true)
            .Text_Lambda([this]() -> FText
            {
                TArray<FString> Lines;
                for (const TSharedPtr<FString>& Leaf : LeafInputs)
                {
                    if (Leaf.IsValid() && !Leaf->IsEmpty())
                    {
                        FString Tag = ParentPath.IsEmpty()
                            ? *Leaf : ParentPath + TEXT(".") + *Leaf;
                        Lines.Add(FString::Printf(TEXT("%s  →  %s"),
                            *Tag, *FSpyTagFileEditor::TagStringToVarName(Tag)));
                    }
                }
                return Lines.IsEmpty()
                    ? LOCTEXT("PreviewEmpty", "경로와 리프 이름을 입력하세요")
                    : FText::FromString(FString::Join(Lines, TEXT("\n")));
            })
        ]
    ];

    // Add Tags 버튼
    Panel->AddSlot().AutoHeight().Padding(4.f, 12.f)
    [
        SNew(SButton).HAlign(HAlign_Center)
        .Text(LOCTEXT("AddTags", "Add Tags"))
        .OnClicked(this, &SSpyTagManagerDialog::OnAddTagsClicked)
    ];

    return Panel.ToSharedRef();
}

void SSpyTagManagerDialog::RebuildLeafInputs()
{
    if (!LeafInputBox.IsValid()) return;
    LeafInputBox->ClearChildren();
    for (const TSharedPtr<FString>& Input : LeafInputs)
    {
        TSharedPtr<FString> Cap = Input;
        LeafInputBox->AddSlot().AutoHeight().Padding(0.f, 2.f)
        [
            SNew(SEditableTextBox)
            .Text(FText::FromString(*Cap))
            .OnTextChanged_Lambda([Cap](const FText& T){ *Cap = T.ToString(); })
        ];
    }
}

FReply SSpyTagManagerDialog::OnAddLeafRowClicked()
{
    LeafInputs.Add(MakeShared<FString>());
    RebuildLeafInputs();
    return FReply::Handled();
}

TSharedRef<SWidget> SSpyTagManagerDialog::OnGenerateGroupWidget(TSharedPtr<FSpyTagGroup> Item)
{
    FString Label = Item.IsValid()
        ? (Item->Comment == NewGroupSentinel ? TEXT("새 그룹 추가...") : Item->Comment)
        : TEXT("");
    return SNew(STextBlock).Text(FText::FromString(Label));
}

FText SSpyTagManagerDialog::GetGroupComboText() const
{
    if (!SelectedGroupOption.IsValid()) return FText::GetEmpty();
    return FText::FromString(
        SelectedGroupOption->Comment == NewGroupSentinel
        ? TEXT("새 그룹 추가...") : SelectedGroupOption->Comment);
}

FReply SSpyTagManagerDialog::OnAddTagsClicked()
{
    if (ParentPath.IsEmpty())
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ErrNoParent", "부모 경로를 입력하세요."));
        return FReply::Handled();
    }

    bool bIsNew = !SelectedGroupOption.IsValid()
        || SelectedGroupOption->Comment == NewGroupSentinel;

    FSpyTagGroup TargetGroup;
    if (bIsNew)
    {
        if (NewGroupComment.IsEmpty())
        {
            FMessageDialog::Open(EAppMsgType::Ok,
                LOCTEXT("ErrNoComment", "새 그룹 주석을 입력하세요."));
            return FReply::Handled();
        }
        TargetGroup.Comment = NewGroupComment;
    }
    else
    {
        TargetGroup = *SelectedGroupOption;
    }

    TArray<FSpyTagEntry> NewEntries;
    for (const TSharedPtr<FString>& Leaf : LeafInputs)
    {
        if (!Leaf.IsValid() || Leaf->IsEmpty()) continue;
        FSpyTagEntry E;
        E.TagString = ParentPath + TEXT(".") + *Leaf;
        E.VarName   = FSpyTagFileEditor::TagStringToVarName(E.TagString);
        NewEntries.Add(E);
    }

    if (NewEntries.IsEmpty())
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("ErrNoLeaves", "리프 이름을 하나 이상 입력하세요."));
        return FReply::Handled();
    }

    for (const FSpyTagEntry& E : NewEntries)
    {
        if (FSpyTagFileEditor::DoesVarNameExist(ParsedGroups, E.VarName))
        {
            FMessageDialog::Open(EAppMsgType::Ok,
                FText::Format(LOCTEXT("ErrDup", "'{0}' 태그가 이미 존재합니다."),
                    FText::FromString(E.VarName)));
            return FReply::Handled();
        }
    }

    if (!FSpyTagFileEditor::AppendTags(ParsedGroups, TargetGroup, NewEntries, bIsNew))
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("ErrWrite", "파일 쓰기에 실패했습니다."));
        return FReply::Handled();
    }

    // 성공: 재파싱 + 트리 갱신 + 입력 초기화
    RebuildData();
    if (TagTreeView.IsValid())
    {
        TagTreeView->RequestTreeRefresh();
        for (const TSharedPtr<FTagTreeNode>& Node : RootNodes)
            TagTreeView->SetItemExpansion(Node, true);
    }
    if (GroupComboBox.IsValid())
        GroupComboBox->SetSelectedItem(SelectedGroupOption);

    LeafInputs.Empty();
    LeafInputs.Add(MakeShared<FString>());
    RebuildLeafInputs();
    ParentPath.Empty();
    if (ParentPathBox.IsValid()) ParentPathBox->SetText(FText::GetEmpty());

    FMessageDialog::Open(EAppMsgType::Ok,
        FText::Format(LOCTEXT("Success", "{0}개 태그가 추가되었습니다."),
            FText::AsNumber(NewEntries.Num())));

    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
