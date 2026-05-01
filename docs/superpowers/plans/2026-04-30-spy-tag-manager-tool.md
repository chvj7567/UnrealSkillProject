# Spy Tag Manager Tool Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** SpyGameplayTags.h/.cpp를 파싱해 태그를 트리로 시각화하고 UI에서 직접 추가하는 독립 에디터 모듈을 만든다.

**Architecture:** 독립 Editor 모듈 `SpyTagManagerTool`. `SpyTagFileEditor` 클래스가 h/cpp 파싱과 쓰기를 전담하고, `SSpyTagManagerDialog` Slate 위젯이 좌(트리 뷰) / 우(추가 폼) 분할 패널을 렌더링한다. Spy Tools 최상단 메뉴에 등록한다.

**Tech Stack:** Unreal Engine 5.7 C++, Slate UI, FFileHelper (파일 I/O), STreeView, SComboBox, SSplitter

---

## 파일 구조

```
SkillProject/Source/SpyTagManagerTool/
├── SpyTagManagerTool.Build.cs
├── Public/
│   ├── SpyTagManagerTool.h
│   ├── SpyTagFileEditor.h        ← 파싱/쓰기 전담, Public 배치 (SSpyTagManagerDialog.h가 포함)
│   └── SSpyTagManagerDialog.h
└── Private/
    ├── SpyTagManagerTool.cpp
    ├── SpyTagFileEditor.cpp
    └── SSpyTagManagerDialog.cpp
```

대상 소스 파일 (하드코딩):
- `SkillProject/Source/SkillProject/Util/SpyGameplayTags.h`
- `SkillProject/Source/SkillProject/Util/SpyGameplayTags.cpp`

참고: h 파일에 `//# 그룹명` 주석과 `UE_DECLARE_GAMEPLAY_TAG_EXTERN` 선언이 있고, cpp 파일에 `UE_DEFINE_GAMEPLAY_TAG(VarName, "tag.string")` 정의가 있다. cpp에는 그룹 주석이 없으므로 그룹 구조는 h에서, TagString은 cpp에서 파싱해 합친다.

---

### Task 1: 모듈 스캐폴드

**Files:**
- Create: `SkillProject/Source/SpyTagManagerTool/SpyTagManagerTool.Build.cs`
- Create: `SkillProject/Source/SpyTagManagerTool/Public/SpyTagManagerTool.h`
- Create: `SkillProject/Source/SpyTagManagerTool/Private/SpyTagManagerTool.cpp`
- Create: `SkillProject/Source/SpyTagManagerTool/Public/SSpyTagManagerDialog.h` (빈 스텁)
- Create: `SkillProject/Source/SpyTagManagerTool/Private/SSpyTagManagerDialog.cpp` (빈 스텁)
- Modify: `SkillProject/SkillProject.uproject`

- [ ] **Step 1: Build.cs 작성**

```csharp
// SkillProject/Source/SpyTagManagerTool/SpyTagManagerTool.Build.cs
using UnrealBuildTool;

public class SpyTagManagerTool : ModuleRules
{
    public SpyTagManagerTool(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate", "SlateCore", "InputCore",
            "UnrealEd", "ToolMenus", "EditorFramework"
        });
    }
}
```

- [ ] **Step 2: SpyTagManagerTool.h 작성**

```cpp
// Public/SpyTagManagerTool.h
#pragma once
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FSpyTagManagerToolModule : public IModuleInterface
{
public:
    static const FName TabName;
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
private:
    TSharedRef<SDockTab> OnSpawnTab(const FSpawnTabArgs& SpawnTabArgs);
    void RegisterMenus();
};
```

- [ ] **Step 3: SpyTagManagerTool.cpp 작성**

```cpp
// Private/SpyTagManagerTool.cpp
#include "SpyTagManagerTool.h"
#include "SSpyTagManagerDialog.h"
#include "Framework/Docking/TabManager.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FSpyTagManagerToolModule"

const FName FSpyTagManagerToolModule::TabName = TEXT("SpyTagManagerTool");

void FSpyTagManagerToolModule::StartupModule()
{
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        TabName,
        FOnSpawnTab::CreateRaw(this, &FSpyTagManagerToolModule::OnSpawnTab))
        .SetDisplayName(LOCTEXT("TabTitle", "Spy Tag Manager"))
        .SetTooltipText(LOCTEXT("TabTooltip", "SpyGameplayTags 태그 관리 도구"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);

    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(
            this, &FSpyTagManagerToolModule::RegisterMenus));
}

void FSpyTagManagerToolModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);
}

TSharedRef<SDockTab> FSpyTagManagerToolModule::OnSpawnTab(const FSpawnTabArgs& SpawnTabArgs)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SSpyTagManagerDialog)
        ];
}

void FSpyTagManagerToolModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);

    UToolMenu* MainMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu");
    if (MainMenu)
    {
        FToolMenuSection& Section = MainMenu->FindOrAddSection("SpyTools");
        Section.AddSubMenu(
            "SpyTools",
            LOCTEXT("SpyToolsMenu", "Spy Tools"),
            LOCTEXT("SpyToolsMenuTooltip", "Spy 개발 도구 모음"),
            FNewToolMenuDelegate::CreateLambda([](UToolMenu*) {})
        );
    }

    UToolMenu* SpyMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.SpyTools");
    if (SpyMenu)
    {
        FToolMenuSection& Section = SpyMenu->FindOrAddSection("SpyToolsSection");
        Section.AddMenuEntry(
            "SpyTagManagerOpen",
            LOCTEXT("MenuEntry", "Spy Tag Manager"),
            LOCTEXT("MenuEntryTooltip", "SpyGameplayTags 태그 관리 도구 열기"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateLambda([]()
            {
                FGlobalTabmanager::Get()->TryInvokeTab(FSpyTagManagerToolModule::TabName);
            }))
        );
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSpyTagManagerToolModule, SpyTagManagerTool)
```

- [ ] **Step 4: SSpyTagManagerDialog 빈 스텁 작성**

```cpp
// Public/SSpyTagManagerDialog.h
#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SSpyTagManagerDialog : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SSpyTagManagerDialog) {}
    SLATE_END_ARGS()
    void Construct(const FArguments& InArgs);
};
```

```cpp
// Private/SSpyTagManagerDialog.cpp
#include "SSpyTagManagerDialog.h"
#include "Widgets/Text/STextBlock.h"

void SSpyTagManagerDialog::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(STextBlock).Text(FText::FromString(TEXT("Spy Tag Manager - WIP")))
    ];
}
```

- [ ] **Step 5: uproject에 모듈 추가**

`SkillProject/SkillProject.uproject`의 `"Modules"` 배열에 추가:
```json
{
    "Name": "SpyTagManagerTool",
    "Type": "Editor",
    "LoadingPhase": "PostEngineInit"
}
```

- [ ] **Step 6: Generate Visual Studio project files 후 빌드**

우클릭 > Generate Visual Studio project files → VS에서 `SkillProjectEditor | Development Editor | Win64` 빌드.  
Expected: 컴파일 성공. 에디터 실행 후 Spy Tools 메뉴에 "Spy Tag Manager" 항목 표시.

- [ ] **Step 7: 커밋**

```
[Feature] SpyTagManagerTool — 모듈 스캐폴드 및 Spy Tools 메뉴 등록
```

---

### Task 2: SpyTagFileEditor — 파싱

**Files:**
- Create: `SkillProject/Source/SpyTagManagerTool/Public/SpyTagFileEditor.h`
- Create: `SkillProject/Source/SpyTagManagerTool/Private/SpyTagFileEditor.cpp`

배경: h 파일에서 그룹 구조(`//# 그룹명` + `UE_DECLARE_GAMEPLAY_TAG_EXTERN(VarName)`)를 파싱하고, cpp 파일에서 VarName→TagString 맵을 파싱해 합친다.

- [ ] **Step 1: SpyTagFileEditor.h 작성**

```cpp
// Public/SpyTagFileEditor.h
#pragma once
#include "CoreMinimal.h"

struct FSpyTagEntry
{
    FString VarName;    // "Skill_Action_A"
    FString TagString;  // "Skill.Action.A"
};

struct FSpyTagGroup
{
    FString Comment;            // "액션 스킬"
    TArray<FSpyTagEntry> Tags;
};

class FSpyTagFileEditor
{
public:
    static FString GetHeaderPath();
    static FString GetCppPath();

    // h에서 그룹+VarName, cpp에서 TagString을 파싱해 합친 결과 반환
    static TArray<FSpyTagGroup> ParseFiles();

    // "Skill.Action.G" -> "Skill_Action_G"
    static FString TagStringToVarName(const FString& TagString);

    // 파싱된 Groups에 VarName이 존재하면 true
    static bool DoesVarNameExist(const TArray<FSpyTagGroup>& Groups, const FString& VarName);

    // h/cpp 파일에 NewEntries를 추가. bIsNewGroup=true면 새 그룹 생성
    static bool AppendTags(
        const TArray<FSpyTagGroup>& ParsedGroups,
        const FSpyTagGroup& TargetGroup,
        const TArray<FSpyTagEntry>& NewEntries,
        bool bIsNewGroup);

private:
    static TArray<FSpyTagGroup> ParseHeaderGroups();
    static TMap<FString, FString> ParseCppTagMap();
};
```

- [ ] **Step 2: SpyTagFileEditor.cpp — 경로 + 파싱 구현**

```cpp
// Private/SpyTagFileEditor.cpp
#include "SpyTagFileEditor.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

FString FSpyTagFileEditor::GetHeaderPath()
{
    return FPaths::ConvertRelativePathToFull(
        FPaths::ProjectDir() / TEXT("Source/SkillProject/Util/SpyGameplayTags.h"));
}

FString FSpyTagFileEditor::GetCppPath()
{
    return FPaths::ConvertRelativePathToFull(
        FPaths::ProjectDir() / TEXT("Source/SkillProject/Util/SpyGameplayTags.cpp"));
}

// h 파일: //# GroupName 과 UE_DECLARE_GAMEPLAY_TAG_EXTERN(VarName) 파싱
TArray<FSpyTagGroup> FSpyTagFileEditor::ParseHeaderGroups()
{
    TArray<FSpyTagGroup> Groups;
    TArray<FString> Lines;
    if (!FFileHelper::LoadFileToStringArray(Lines, *GetHeaderPath()))
        return Groups;

    FSpyTagGroup* Current = nullptr;
    for (const FString& Line : Lines)
    {
        FString T = Line.TrimStartAndEnd();
        if (T.StartsWith(TEXT("//#")))
        {
            FSpyTagGroup G;
            G.Comment = T.RightChop(3).TrimStartAndEnd();
            Groups.Add(G);
            Current = &Groups.Last();
            continue;
        }
        if (Current && T.Contains(TEXT("UE_DECLARE_GAMEPLAY_TAG_EXTERN(")))
        {
            FString Inner = T;
            Inner.RemoveFromStart(TEXT("SKILLPROJECT_API "));
            Inner.RemoveFromStart(TEXT("UE_DECLARE_GAMEPLAY_TAG_EXTERN("));
            Inner.RemoveFromEnd(TEXT(");"));
            FSpyTagEntry Entry;
            Entry.VarName = Inner.TrimStartAndEnd();
            Current->Tags.Add(Entry);
        }
    }
    return Groups;
}

// cpp 파일: UE_DEFINE_GAMEPLAY_TAG(VarName, "tag.string") 파싱 → VarName->TagString 맵
TMap<FString, FString> FSpyTagFileEditor::ParseCppTagMap()
{
    TMap<FString, FString> Result;
    TArray<FString> Lines;
    if (!FFileHelper::LoadFileToStringArray(Lines, *GetCppPath()))
        return Result;

    for (const FString& Line : Lines)
    {
        FString T = Line.TrimStartAndEnd();
        if (!T.StartsWith(TEXT("UE_DEFINE_GAMEPLAY_TAG("))) continue;

        FString Inner = T;
        Inner.RemoveFromStart(TEXT("UE_DEFINE_GAMEPLAY_TAG("));
        Inner.RemoveFromEnd(TEXT(");"));

        TArray<FString> Parts;
        Inner.ParseIntoArray(Parts, TEXT(","), true);
        if (Parts.Num() < 2) continue;

        FString VarName = Parts[0].TrimStartAndEnd();
        FString TagStr  = Parts[1].TrimStartAndEnd();
        TagStr.RemoveFromStart(TEXT("\""));
        TagStr.RemoveFromEnd(TEXT("\""));
        Result.Add(VarName, TagStr);
    }
    return Result;
}

TArray<FSpyTagGroup> FSpyTagFileEditor::ParseFiles()
{
    TArray<FSpyTagGroup> Groups = ParseHeaderGroups();
    TMap<FString, FString> CppMap = ParseCppTagMap();
    for (FSpyTagGroup& Group : Groups)
        for (FSpyTagEntry& Entry : Group.Tags)
            if (FString* S = CppMap.Find(Entry.VarName))
                Entry.TagString = *S;
    return Groups;
}

FString FSpyTagFileEditor::TagStringToVarName(const FString& TagString)
{
    return TagString.Replace(TEXT("."), TEXT("_"));
}

bool FSpyTagFileEditor::DoesVarNameExist(const TArray<FSpyTagGroup>& Groups, const FString& VarName)
{
    for (const FSpyTagGroup& G : Groups)
        for (const FSpyTagEntry& E : G.Tags)
            if (E.VarName == VarName) return true;
    return false;
}

bool FSpyTagFileEditor::AppendTags(
    const TArray<FSpyTagGroup>&, const FSpyTagGroup&,
    const TArray<FSpyTagEntry>&, bool)
{
    return false; // Task 3에서 구현
}
```

- [ ] **Step 3: 빌드 확인**

VS에서 빌드. Expected: 컴파일 성공.

- [ ] **Step 4: 커밋**

```
[Feature] SpyTagFileEditor — h/cpp 파싱 구현 (ParseFiles, DoesVarNameExist)
```

---

### Task 3: SpyTagFileEditor — AppendTags (파일 쓰기)

**Files:**
- Modify: `SkillProject/Source/SpyTagManagerTool/Private/SpyTagFileEditor.cpp`

전략:
- **h 기존 그룹**: `//# Comment` 줄을 찾고, 다음 `//# ` 또는 `}` 전까지 마지막 `UE_DECLARE` 줄 뒤에 삽입
- **h 새 그룹**: 마지막 `}` (namespace 닫기) 직전에 삽입
- **cpp 기존 그룹**: TargetGroup의 마지막 VarName을 cpp에서 찾아 그 뒤에 삽입
- **cpp 새 그룹**: 마지막 `UE_DEFINE_GAMEPLAY_TAG` 줄 뒤에 삽입

- [ ] **Step 1: AppendTags 전체 구현으로 교체**

```cpp
bool FSpyTagFileEditor::AppendTags(
    const TArray<FSpyTagGroup>& ParsedGroups,
    const FSpyTagGroup& TargetGroup,
    const TArray<FSpyTagEntry>& NewEntries,
    bool bIsNewGroup)
{
    if (NewEntries.IsEmpty()) return true;

    // ── h 파일 수정 ──────────────────────────────────────────────
    TArray<FString> HLines;
    if (!FFileHelper::LoadFileToStringArray(HLines, *GetHeaderPath()))
        return false;

    if (bIsNewGroup)
    {
        // namespace 닫기 } 직전에 삽입
        int32 CloseIdx = INDEX_NONE;
        for (int32 i = HLines.Num() - 1; i >= 0; --i)
        {
            if (HLines[i].TrimStartAndEnd() == TEXT("}"))
            { CloseIdx = i; break; }
        }
        if (CloseIdx == INDEX_NONE) return false;

        int32 Pos = CloseIdx;
        HLines.Insert(TEXT(""), Pos++);
        HLines.Insert(FString::Printf(TEXT("\t//# %s"), *TargetGroup.Comment), Pos++);
        for (const FSpyTagEntry& E : NewEntries)
            HLines.Insert(
                FString::Printf(TEXT("\tSKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(%s);"), *E.VarName),
                Pos++);
    }
    else
    {
        // 대상 그룹 헤더 줄 찾기
        int32 HeaderIdx = INDEX_NONE;
        for (int32 i = 0; i < HLines.Num(); ++i)
        {
            FString T = HLines[i].TrimStartAndEnd();
            if (T.StartsWith(TEXT("//#")) && T.RightChop(3).TrimStartAndEnd() == TargetGroup.Comment)
            { HeaderIdx = i; break; }
        }
        if (HeaderIdx == INDEX_NONE) return false;

        // 그룹 범위 내 마지막 UE_DECLARE 줄
        int32 LastTag = HeaderIdx;
        for (int32 i = HeaderIdx + 1; i < HLines.Num(); ++i)
        {
            FString T = HLines[i].TrimStartAndEnd();
            if (T.StartsWith(TEXT("//#")) || T == TEXT("}")) break;
            if (T.Contains(TEXT("UE_DECLARE_GAMEPLAY_TAG_EXTERN("))) LastTag = i;
        }

        int32 Pos = LastTag + 1;
        for (const FSpyTagEntry& E : NewEntries)
            HLines.Insert(
                FString::Printf(TEXT("\tSKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(%s);"), *E.VarName),
                Pos++);
    }

    FString HContent = FString::Join(HLines, TEXT("\n")) + TEXT("\n");
    if (!FFileHelper::SaveStringToFile(HContent, *GetHeaderPath()))
        return false;

    // ── cpp 파일 수정 ─────────────────────────────────────────────
    TArray<FString> CppLines;
    if (!FFileHelper::LoadFileToStringArray(CppLines, *GetCppPath()))
        return false;

    if (bIsNewGroup)
    {
        // 마지막 UE_DEFINE_GAMEPLAY_TAG 줄 뒤에 삽입
        int32 LastDef = INDEX_NONE;
        for (int32 i = 0; i < CppLines.Num(); ++i)
            if (CppLines[i].TrimStartAndEnd().StartsWith(TEXT("UE_DEFINE_GAMEPLAY_TAG(")))
                LastDef = i;
        if (LastDef == INDEX_NONE) return false;

        int32 Pos = LastDef + 1;
        CppLines.Insert(TEXT(""), Pos++);
        CppLines.Insert(FString::Printf(TEXT("\t//# %s"), *TargetGroup.Comment), Pos++);
        for (const FSpyTagEntry& E : NewEntries)
            CppLines.Insert(
                FString::Printf(TEXT("\tUE_DEFINE_GAMEPLAY_TAG(%s, \"%s\");"), *E.VarName, *E.TagString),
                Pos++);
    }
    else
    {
        // 대상 그룹의 마지막 VarName을 cpp에서 찾기
        FString LastVar = TargetGroup.Tags.IsEmpty() ? TEXT("") : TargetGroup.Tags.Last().VarName;
        int32 InsertIdx = INDEX_NONE;
        if (!LastVar.IsEmpty())
            for (int32 i = 0; i < CppLines.Num(); ++i)
                if (CppLines[i].Contains(TEXT("UE_DEFINE_GAMEPLAY_TAG(")) &&
                    CppLines[i].Contains(LastVar))
                    InsertIdx = i;

        // Fallback: 마지막 UE_DEFINE 줄
        if (InsertIdx == INDEX_NONE)
            for (int32 i = 0; i < CppLines.Num(); ++i)
                if (CppLines[i].TrimStartAndEnd().StartsWith(TEXT("UE_DEFINE_GAMEPLAY_TAG(")))
                    InsertIdx = i;

        if (InsertIdx == INDEX_NONE) return false;

        int32 Pos = InsertIdx + 1;
        for (const FSpyTagEntry& E : NewEntries)
            CppLines.Insert(
                FString::Printf(TEXT("\tUE_DEFINE_GAMEPLAY_TAG(%s, \"%s\");"), *E.VarName, *E.TagString),
                Pos++);
    }

    FString CppContent = FString::Join(CppLines, TEXT("\n")) + TEXT("\n");
    return FFileHelper::SaveStringToFile(CppContent, *GetCppPath());
}
```

- [ ] **Step 2: 빌드 확인**

VS에서 빌드. Expected: 컴파일 성공.

- [ ] **Step 3: 커밋**

```
[Feature] SpyTagFileEditor — AppendTags 파일 쓰기 구현
```

---

### Task 4: SSpyTagManagerDialog — 왼쪽 패널 (트리 뷰)

**Files:**
- Modify: `SkillProject/Source/SpyTagManagerTool/Public/SSpyTagManagerDialog.h`
- Modify: `SkillProject/Source/SpyTagManagerTool/Private/SSpyTagManagerDialog.cpp`

`FTagTreeNode` 구조: 그룹 헤더 노드(`bIsGroupHeader=true`)와 경로 노드. `FullPath`에 dot-notation 전체 경로 저장.

- [ ] **Step 1: SSpyTagManagerDialog.h 전체 교체**

```cpp
// Public/SSpyTagManagerDialog.h
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
    // ── Data ──
    TArray<FSpyTagGroup> ParsedGroups;
    TArray<TSharedPtr<FTagTreeNode>> RootNodes;
    TArray<TSharedPtr<FSpyTagGroup>> GroupOptions;
    TSharedPtr<FSpyTagGroup> SelectedGroupOption;

    // ── Add panel state ──
    FString ParentPath;
    FString NewGroupComment;
    TArray<TSharedPtr<FString>> LeafInputs;

    // ── Widgets ──
    TSharedPtr<STreeView<TSharedPtr<FTagTreeNode>>> TagTreeView;
    TSharedPtr<SComboBox<TSharedPtr<FSpyTagGroup>>> GroupComboBox;
    TSharedPtr<SEditableTextBox> ParentPathBox;
    TSharedPtr<SEditableTextBox> GroupCommentBox;
    TSharedPtr<SVerticalBox> LeafInputBox;

    // ── Helpers ──
    void RebuildData();
    TArray<TSharedPtr<FTagTreeNode>> BuildTreeNodes(const TArray<FSpyTagGroup>& Groups);
    void InsertTagIntoTree(TSharedPtr<FTagTreeNode> Root, const FString& TagString);
    TSharedRef<SWidget> BuildAddPanel();
    void RebuildLeafInputs();

    // ── Tree callbacks ──
    TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FTagTreeNode> Node, const TSharedRef<STableViewBase>& Owner);
    void OnGetChildren(TSharedPtr<FTagTreeNode> Node, TArray<TSharedPtr<FTagTreeNode>>& Out);
    void OnTreeSelectionChanged(TSharedPtr<FTagTreeNode> Node, ESelectInfo::Type Type);

    // ── Button / combo callbacks ──
    FReply OnRefreshClicked();
    FReply OnAddTagsClicked();
    FReply OnAddLeafRowClicked();
    TSharedRef<SWidget> OnGenerateGroupWidget(TSharedPtr<FSpyTagGroup> Item);
    FText GetGroupComboText() const;

    static const FString NewGroupSentinel;
};
```

- [ ] **Step 2: SSpyTagManagerDialog.cpp — includes + 상수 + RebuildData + 트리 빌드**

```cpp
// Private/SSpyTagManagerDialog.cpp
#include "SSpyTagManagerDialog.h"
#include "SpyTagFileEditor.h"
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
```

- [ ] **Step 3: Construct + 왼쪽 패널 + Tree 콜백**

```cpp
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
    if (!Node || Node->bIsGroupHeader) return;
    ParentPath = Node->FullPath;
    if (ParentPathBox.IsValid())
        ParentPathBox->SetText(FText::FromString(ParentPath));
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
```

- [ ] **Step 4: 빌드 확인**

VS 빌드. Expected: 컴파일 성공. 에디터에서 Spy Tag Manager 탭 열면 왼쪽에 트리가 표시됨.

- [ ] **Step 5: 커밋**

```
[Feature] SSpyTagManagerDialog — 좌측 태그 트리 뷰 구현
```

---

### Task 5: SSpyTagManagerDialog — 오른쪽 패널 + 연결

**Files:**
- Modify: `SkillProject/Source/SpyTagManagerTool/Private/SSpyTagManagerDialog.cpp`

- [ ] **Step 1: BuildAddPanel + RebuildLeafInputs + OnAddLeafRowClicked 추가**

```cpp
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
```

- [ ] **Step 2: OnAddTagsClicked 추가**

```cpp
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
```

- [ ] **Step 3: 빌드 확인**

VS 빌드. Expected: 컴파일 성공.

- [ ] **Step 4: 에디터에서 기능 검증**

1. 에디터 실행 → Spy Tools > Spy Tag Manager 탭 열기
2. 왼쪽 트리에 기존 태그 그룹이 표시되는지 확인 (`# 락 태그`, `# 액션 스킬` 등)
3. 트리 노드 클릭 시 오른쪽 "부모 경로"에 dot-notation 자동 입력 확인
4. 리프 이름 입력 → 미리보기에 `Skill.Action.G  →  Skill_Action_G` 형태 표시 확인
5. Add Tags 클릭 → `SpyGameplayTags.h` / `.cpp` 파일이 수정되었는지 텍스트 에디터로 확인
6. 새로고침 버튼 클릭 → 추가된 태그가 트리에 표시 확인
7. "새 그룹 추가..." 선택 → 그룹 주석 필드 활성화 확인

- [ ] **Step 5: 커밋**

```
[Feature] SSpyTagManagerDialog — 오른쪽 추가 패널 및 AppendTags 연결 완성
```
