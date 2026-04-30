# SpyGACreatorTool Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Unreal Editor Window 메뉴에 독립 탭 "Spy GA Creator"를 추가하고, 부모 클래스·GAS 설정을 입력해 `/Game/Spy/Blueprints/GameplayAbilities/GA_<Name>` Blueprint를 생성 후 에디터를 자동으로 여는 에디터 툴 구현.

**Architecture:** `SpyGACreatorTool` 독립 에디터 모듈 신규 생성. `StartupModule`에서 NomadTab Spawner와 Window 메뉴 항목 등록. 탭 콘텐츠는 `SSpyCreateGADialog` Slate 위젯. "Create GA" 클릭 시 `FKismetEditorUtilities::CreateBlueprint` → CDO 설정 → Save → 에디터 오픈. `SpyDataEditorTool`과 코드 의존성 전혀 없음.

**Tech Stack:** UE 5.7 C++, Slate, GAS (GameplayAbilities), FKismetEditorUtilities, SGameplayTagContainerCombo, SClassPropertyEntryBox

---

## File Map

| 경로 | 역할 |
|------|------|
| `SkillProject/Source/SpyGACreatorTool/SpyGACreatorTool.Build.cs` | 모듈 빌드 설정 |
| `SkillProject/Source/SpyGACreatorTool/Public/SpyGACreatorTool.h` | 모듈 클래스 선언 |
| `SkillProject/Source/SpyGACreatorTool/Private/SpyGACreatorTool.cpp` | 모듈 등록, NomadTab Spawner, Window 메뉴 |
| `SkillProject/Source/SpyGACreatorTool/Public/SSpyCreateGADialog.h` | 탭 콘텐츠 위젯 선언 (멤버 변수 포함) |
| `SkillProject/Source/SpyGACreatorTool/Private/SSpyCreateGADialog.cpp` | 위젯 구현 (UI 구성 + BP 생성 로직) |
| `SkillProject/SkillProject.uproject` | 모듈 등록 추가 |

---

## Task 1: 모듈 스캐폴딩

**Files:**
- Create: `SkillProject/Source/SpyGACreatorTool/SpyGACreatorTool.Build.cs`
- Modify: `SkillProject/SkillProject.uproject`

- [ ] **Step 1: Build.cs 생성**

`SkillProject/Source/SpyGACreatorTool/SpyGACreatorTool.Build.cs`:

```csharp
using UnrealBuildTool;

public class SpyGACreatorTool : ModuleRules
{
    public SpyGACreatorTool(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine",
            "GameplayAbilities", "GameplayTags"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate", "SlateCore", "InputCore",
            "UnrealEd", "Kismet",
            "GameplayTagsEditor", "EditorWidgets",
            "ToolMenus", "EditorFramework"
        });
    }
}
```

- [ ] **Step 2: uproject 모듈 등록**

`SkillProject/SkillProject.uproject` 의 `"Modules"` 배열 맨 끝에 추가:

```json
{
    "Name": "SpyGACreatorTool",
    "Type": "Editor",
    "LoadingPhase": "PostEngineInit"
}
```

변경 후 `"Modules"` 배열 전체:

```json
"Modules": [
    {
        "Name": "SkillProject",
        "Type": "Runtime",
        "LoadingPhase": "Default",
        "AdditionalDependencies": ["SKGAS", "MotionWarping", "Engine"]
    },
    {
        "Name": "SKGAS",
        "Type": "Runtime",
        "LoadingPhase": "Default"
    },
    {
        "Name": "SpyDataEditorTool",
        "Type": "Editor",
        "LoadingPhase": "PostEngineInit"
    },
    {
        "Name": "SpyGACreatorTool",
        "Type": "Editor",
        "LoadingPhase": "PostEngineInit"
    }
]
```

---

## Task 2: 모듈 등록 + 스텁 UI

**Files:**
- Create: `SkillProject/Source/SpyGACreatorTool/Public/SpyGACreatorTool.h`
- Create: `SkillProject/Source/SpyGACreatorTool/Private/SpyGACreatorTool.cpp`
- Create: `SkillProject/Source/SpyGACreatorTool/Public/SSpyCreateGADialog.h`
- Create: `SkillProject/Source/SpyGACreatorTool/Private/SSpyCreateGADialog.cpp`

- [ ] **Step 1: 모듈 헤더 생성**

`Public/SpyGACreatorTool.h`:

```cpp
#pragma once
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FSpyGACreatorToolModule : public IModuleInterface
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

- [ ] **Step 2: 스텁 위젯 헤더 생성**

`Public/SSpyCreateGADialog.h` (스텁 — Task 3에서 전체 교체):

```cpp
#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SSpyCreateGADialog : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SSpyCreateGADialog) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
};
```

- [ ] **Step 3: 스텁 위젯 구현 생성**

`Private/SSpyCreateGADialog.cpp` (스텁):

```cpp
#include "SSpyCreateGADialog.h"
#include "Widgets/Text/STextBlock.h"

void SSpyCreateGADialog::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(STextBlock).Text(FText::FromString(TEXT("Spy GA Creator")))
    ];
}
```

- [ ] **Step 4: 모듈 구현 생성**

`Private/SpyGACreatorTool.cpp`:

```cpp
#include "SpyGACreatorTool.h"
#include "SSpyCreateGADialog.h"
#include "Framework/Docking/TabManager.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FSpyGACreatorToolModule"

const FName FSpyGACreatorToolModule::TabName = TEXT("SpyGACreatorTool");

void FSpyGACreatorToolModule::StartupModule()
{
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        TabName,
        FOnSpawnTab::CreateRaw(this, &FSpyGACreatorToolModule::OnSpawnTab))
        .SetDisplayName(LOCTEXT("TabTitle", "Spy GA Creator"))
        .SetTooltipText(LOCTEXT("TabTooltip", "Gameplay Ability Blueprint 생성 도구"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);

    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(
            this, &FSpyGACreatorToolModule::RegisterMenus));
}

void FSpyGACreatorToolModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);
}

TSharedRef<SDockTab> FSpyGACreatorToolModule::OnSpawnTab(const FSpawnTabArgs& SpawnTabArgs)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SSpyCreateGADialog)
        ];
}

void FSpyGACreatorToolModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);
    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
    if (!Menu) return;
    FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
    Section.AddMenuEntry(
        "SpyGACreatorOpen",
        LOCTEXT("MenuEntry", "Spy GA Creator"),
        LOCTEXT("MenuEntryTooltip", "Gameplay Ability Blueprint 생성 도구 열기"),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateLambda([]()
        {
            FGlobalTabmanager::Get()->TryInvokeTab(FSpyGACreatorToolModule::TabName);
        }))
    );
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSpyGACreatorToolModule, SpyGACreatorTool)
```

- [ ] **Step 5: VS 프로젝트 파일 재생성 후 빌드**

uproject 우클릭 → "Generate Visual Studio project files" 실행.
Visual Studio에서 `Development Editor` 구성으로 빌드.

- [ ] **Step 6: 에디터에서 검증**

에디터 실행 → Window 메뉴에 "Spy GA Creator" 항목 확인 → 클릭 시 "Spy GA Creator" 텍스트가 있는 탭이 열리면 성공.

- [ ] **Step 7: 커밋 준비**

```
git add SkillProject/Source/SpyGACreatorTool/
git add SkillProject/SkillProject.uproject
```

커밋 메시지: `[Feature] SpyGACreatorTool — 모듈 등록 및 스텁 UI`

---

## Task 3: 전체 UI 구현

**Files:**
- Modify: `SkillProject/Source/SpyGACreatorTool/Public/SSpyCreateGADialog.h`
- Modify: `SkillProject/Source/SpyGACreatorTool/Private/SSpyCreateGADialog.cpp`

- [ ] **Step 1: 헤더 전체 교체**

`Public/SSpyCreateGADialog.h`:

```cpp
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

    // GE Classes (SClassPropertyEntryBox 가 const UClass* 를 반환)
    UClass* CostGEClass     = nullptr;
    UClass* CooldownGEClass = nullptr;

    // 경로 실시간 표시
    TSharedPtr<STextBlock> OutputPathText;
};
```

- [ ] **Step 2: 구현 전체 교체**

`Private/SSpyCreateGADialog.cpp`:

```cpp
#include "SSpyCreateGADialog.h"

#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SComboBox.h"
#include "SGameplayTagContainerCombo.h"
#include "SClassPropertyEntryBox.h"
#include "Misc/MessageDialog.h"
#include "Styling/AppStyle.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"

#define LOCTEXT_NAMESPACE "SSpyCreateGADialog"

// ──────────────────────────────────────────────────────────
// 헬퍼 — 회색 배경 섹션 헤더
// ──────────────────────────────────────────────────────────
TSharedRef<SWidget> SSpyCreateGADialog::MakeSectionHeader(const FText& Label)
{
    return SNew(SBorder)
        .BorderBackgroundColor(FLinearColor(0.08f, 0.08f, 0.08f, 1.f))
        .Padding(FMargin(6.f, 3.f))
        [
            SNew(STextBlock)
            .Text(Label)
            .Font(FAppStyle::GetFontStyle("BoldFont"))
        ];
}

// ──────────────────────────────────────────────────────────
// 헬퍼 — 레이블(200px) + 콘텐츠 한 줄
// ──────────────────────────────────────────────────────────
TSharedRef<SWidget> SSpyCreateGADialog::MakeLabeledRow(
    const FText& Label, TSharedRef<SWidget> Content)
{
    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
          .AutoWidth()
          .VAlign(VAlign_Center)
          .Padding(FMargin(4.f, 2.f))
          [
              SNew(SBox).WidthOverride(200.f)
              [
                  SNew(STextBlock).Text(Label)
              ]
          ]
        + SHorizontalBox::Slot()
          .FillWidth(1.f)
          .VAlign(VAlign_Center)
          .Padding(FMargin(0.f, 2.f, 4.f, 2.f))
          [
              Content
          ];
}

// ──────────────────────────────────────────────────────────
// UGameplayAbility 서브클래스 스캔
// ──────────────────────────────────────────────────────────
void SSpyCreateGADialog::BuildClassList()
{
    ClassList.Empty();
    ClassList.Add(UGameplayAbility::StaticClass());

    TArray<UClass*> Derived;
    GetDerivedClasses(UGameplayAbility::StaticClass(), Derived, true);
    Derived.RemoveAll([](UClass* C)
    {
        return C->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated)
            || C->GetName().StartsWith(TEXT("SKEL_"))
            || C->GetName().StartsWith(TEXT("REINST_"));
    });
    Derived.Sort([](const UClass& A, const UClass& B)
    {
        return A.GetName() < B.GetName();
    });
    ClassList.Append(Derived);
    SelectedClass = UGameplayAbility::StaticClass();
}

// ──────────────────────────────────────────────────────────
// Construct
// ──────────────────────────────────────────────────────────
void SSpyCreateGADialog::Construct(const FArguments& InArgs)
{
    BuildClassList();

    // Policy 옵션 초기화
    NetExecOptions = {
        MakeShared<FString>(TEXT("LocalPredicted")),
        MakeShared<FString>(TEXT("LocalOnly")),
        MakeShared<FString>(TEXT("ServerInitiated")),
        MakeShared<FString>(TEXT("ServerOnly")),
    };
    InstancingOptions = {
        MakeShared<FString>(TEXT("NonInstanced")),
        MakeShared<FString>(TEXT("InstancedPerActor")),
        MakeShared<FString>(TEXT("InstancedPerExecution")),
    };
    NetSecOptions = {
        MakeShared<FString>(TEXT("ClientOrServer")),
        MakeShared<FString>(TEXT("ServerOnlyExecution")),
        MakeShared<FString>(TEXT("ClientOnlyExecution")),
        MakeShared<FString>(TEXT("ServerOnly")),
    };
    SelectedNetExecPolicy    = NetExecOptions[0];    // LocalPredicted
    SelectedInstancingPolicy = InstancingOptions[1]; // InstancedPerActor
    SelectedNetSecPolicy     = NetSecOptions[0];     // ClientOrServer

    ChildSlot
    [
        SNew(SScrollBox)

        // ── 기본 설정 ─────────────────────────────────────
        + SScrollBox::Slot().Padding(FMargin(0.f, 4.f))
        [ MakeSectionHeader(LOCTEXT("SectionHeader", "기본 설정")) ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("ParentClass", "Parent Class"),
                SNew(SComboBox<UClass*>)
                    .OptionsSource(&ClassList)
                    .InitiallySelectedItem(SelectedClass)
                    .OnSelectionChanged_Lambda([this](UClass* Item, ESelectInfo::Type)
                    {
                        SelectedClass = Item;
                    })
                    .OnGenerateWidget_Lambda([](UClass* C) -> TSharedRef<SWidget>
                    {
                        return SNew(STextBlock)
                            .Text(FText::FromString(C ? C->GetName() : TEXT("None")));
                    })
                    [
                        SNew(STextBlock).Text_Lambda([this]() -> FText
                        {
                            return FText::FromString(
                                SelectedClass ? SelectedClass->GetName() : TEXT("선택..."));
                        })
                    ]
            )
        ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("GAName", "GA Name"),
                SNew(SEditableTextBox)
                    .HintText(LOCTEXT("GANameHint", "예: MySkill  →  GA_MySkill"))
                    .OnTextChanged_Lambda([this](const FText& Text)
                    {
                        GAName = Text.ToString();
                        if (OutputPathText.IsValid())
                        {
                            OutputPathText->SetText(FText::FromString(FString::Printf(
                                TEXT("/Game/Spy/Blueprints/GameplayAbilities/GA_%s"), *GAName)));
                        }
                    })
            )
        ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("OutputPath", "Output Path"),
                SAssignNew(OutputPathText, STextBlock)
                    .Text(FText::FromString(TEXT("/Game/Spy/Blueprints/GameplayAbilities/GA_")))
                    .ColorAndOpacity(FSlateColor::UseSubduedForeground())
            )
        ]

        // ── Tags ─────────────────────────────────────────
        + SScrollBox::Slot().Padding(FMargin(0.f, 8.f, 0.f, 0.f))
        [ MakeSectionHeader(LOCTEXT("SectionTags", "Tags")) ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("AbilityTags", "Ability Tags"),
                SNew(SGameplayTagContainerCombo)
                    .TagContainer_Lambda([this]() { return AbilityTagsContainer; })
                    .OnTagContainerChanged(FOnGameplayTagContainerChanged::CreateLambda(
                        [this](const FGameplayTagContainer& C) { AbilityTagsContainer = C; }))
            )
        ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("ActivationOwnedTags", "Activation Owned Tags"),
                SNew(SGameplayTagContainerCombo)
                    .TagContainer_Lambda([this]() { return ActivationOwnedTagsContainer; })
                    .OnTagContainerChanged(FOnGameplayTagContainerChanged::CreateLambda(
                        [this](const FGameplayTagContainer& C) { ActivationOwnedTagsContainer = C; }))
            )
        ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("ActivationRequiredTags", "Activation Required Tags"),
                SNew(SGameplayTagContainerCombo)
                    .TagContainer_Lambda([this]() { return ActivationRequiredTagsContainer; })
                    .OnTagContainerChanged(FOnGameplayTagContainerChanged::CreateLambda(
                        [this](const FGameplayTagContainer& C) { ActivationRequiredTagsContainer = C; }))
            )
        ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("ActivationBlockedTags", "Activation Blocked Tags"),
                SNew(SGameplayTagContainerCombo)
                    .TagContainer_Lambda([this]() { return ActivationBlockedTagsContainer; })
                    .OnTagContainerChanged(FOnGameplayTagContainerChanged::CreateLambda(
                        [this](const FGameplayTagContainer& C) { ActivationBlockedTagsContainer = C; }))
            )
        ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("CancelAbilitiesWithTag", "Cancel Abilities With Tag"),
                SNew(SGameplayTagContainerCombo)
                    .TagContainer_Lambda([this]() { return CancelAbilitiesWithTagContainer; })
                    .OnTagContainerChanged(FOnGameplayTagContainerChanged::CreateLambda(
                        [this](const FGameplayTagContainer& C) { CancelAbilitiesWithTagContainer = C; }))
            )
        ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("BlockAbilitiesWithTag", "Block Abilities With Tag"),
                SNew(SGameplayTagContainerCombo)
                    .TagContainer_Lambda([this]() { return BlockAbilitiesWithTagContainer; })
                    .OnTagContainerChanged(FOnGameplayTagContainerChanged::CreateLambda(
                        [this](const FGameplayTagContainer& C) { BlockAbilitiesWithTagContainer = C; }))
            )
        ]

        // ── Policies ─────────────────────────────────────
        + SScrollBox::Slot().Padding(FMargin(0.f, 8.f, 0.f, 0.f))
        [ MakeSectionHeader(LOCTEXT("SectionPolicies", "Policies")) ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("NetExec", "Net Execution Policy"),
                SNew(SComboBox<TSharedPtr<FString>>)
                    .OptionsSource(&NetExecOptions)
                    .InitiallySelectedItem(SelectedNetExecPolicy)
                    .OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type)
                    {
                        if (Item.IsValid()) SelectedNetExecPolicy = Item;
                    })
                    .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) -> TSharedRef<SWidget>
                    {
                        return SNew(STextBlock).Text(FText::FromString(*Item));
                    })
                    [
                        SNew(STextBlock).Text_Lambda([this]() -> FText
                        {
                            return FText::FromString(
                                SelectedNetExecPolicy.IsValid() ? *SelectedNetExecPolicy : TEXT(""));
                        })
                    ]
            )
        ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("Instancing", "Instancing Policy"),
                SNew(SComboBox<TSharedPtr<FString>>)
                    .OptionsSource(&InstancingOptions)
                    .InitiallySelectedItem(SelectedInstancingPolicy)
                    .OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type)
                    {
                        if (Item.IsValid()) SelectedInstancingPolicy = Item;
                    })
                    .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) -> TSharedRef<SWidget>
                    {
                        return SNew(STextBlock).Text(FText::FromString(*Item));
                    })
                    [
                        SNew(STextBlock).Text_Lambda([this]() -> FText
                        {
                            return FText::FromString(
                                SelectedInstancingPolicy.IsValid() ? *SelectedInstancingPolicy : TEXT(""));
                        })
                    ]
            )
        ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("NetSec", "Net Security Policy"),
                SNew(SComboBox<TSharedPtr<FString>>)
                    .OptionsSource(&NetSecOptions)
                    .InitiallySelectedItem(SelectedNetSecPolicy)
                    .OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type)
                    {
                        if (Item.IsValid()) SelectedNetSecPolicy = Item;
                    })
                    .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) -> TSharedRef<SWidget>
                    {
                        return SNew(STextBlock).Text(FText::FromString(*Item));
                    })
                    [
                        SNew(STextBlock).Text_Lambda([this]() -> FText
                        {
                            return FText::FromString(
                                SelectedNetSecPolicy.IsValid() ? *SelectedNetSecPolicy : TEXT(""));
                        })
                    ]
            )
        ]

        // ── Costs ────────────────────────────────────────
        + SScrollBox::Slot().Padding(FMargin(0.f, 8.f, 0.f, 0.f))
        [ MakeSectionHeader(LOCTEXT("SectionCosts", "Costs")) ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("CostGE", "Cost GE Class"),
                SNew(SClassPropertyEntryBox)
                    .MetaClass(UGameplayEffect::StaticClass())
                    .AllowNone(true)
                    .AllowAbstract(false)
                    .SelectedClass_Lambda([this]() -> const UClass* { return CostGEClass; })
                    .OnSetClass(FOnSetClass::CreateLambda(
                        [this](const UClass* C) { CostGEClass = const_cast<UClass*>(C); }))
            )
        ]

        // ── Cooldowns ────────────────────────────────────
        + SScrollBox::Slot().Padding(FMargin(0.f, 8.f, 0.f, 0.f))
        [ MakeSectionHeader(LOCTEXT("SectionCooldowns", "Cooldowns")) ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("CooldownGE", "Cooldown GE Class"),
                SNew(SClassPropertyEntryBox)
                    .MetaClass(UGameplayEffect::StaticClass())
                    .AllowNone(true)
                    .AllowAbstract(false)
                    .SelectedClass_Lambda([this]() -> const UClass* { return CooldownGEClass; })
                    .OnSetClass(FOnSetClass::CreateLambda(
                        [this](const UClass* C) { CooldownGEClass = const_cast<UClass*>(C); }))
            )
        ]

        // ── Create 버튼 ───────────────────────────────────
        + SScrollBox::Slot().Padding(FMargin(4.f, 12.f))
        [
            SNew(SButton)
            .HAlign(HAlign_Center)
            .Text(LOCTEXT("CreateBtn", "Create GA"))
            .OnClicked(this, &SSpyCreateGADialog::OnCreateClicked)
        ]
    ];
}

// ──────────────────────────────────────────────────────────
// 유효성 검사
// ──────────────────────────────────────────────────────────
bool SSpyCreateGADialog::ValidateInputs(FText& OutError) const
{
    if (!SelectedClass)
    {
        OutError = LOCTEXT("ErrNoClass", "Parent Class를 선택하세요.");
        return false;
    }
    if (GAName.IsEmpty())
    {
        OutError = LOCTEXT("ErrNoName", "GA 이름을 입력하세요.");
        return false;
    }
    for (TCHAR C : GAName)
    {
        if (!FChar::IsAlnum(C) && C != TEXT('_'))
        {
            OutError = LOCTEXT("ErrBadName", "이름은 영숫자와 언더스코어(_)만 허용됩니다.");
            return false;
        }
    }
    FString PackagePath = FString::Printf(
        TEXT("/Game/Spy/Blueprints/GameplayAbilities/GA_%s"), *GAName);
    if (FPackageName::DoesPackageExist(PackagePath))
    {
        OutError = FText::Format(
            LOCTEXT("ErrExists", "'{0}' 에셋이 이미 존재합니다."),
            FText::FromString(PackagePath));
        return false;
    }
    return true;
}

// ──────────────────────────────────────────────────────────
// Create 버튼 핸들러 — 스텁 (Task 4에서 로직 추가)
// ──────────────────────────────────────────────────────────
FReply SSpyCreateGADialog::OnCreateClicked()
{
    FText Error;
    if (!ValidateInputs(Error))
    {
        FMessageDialog::Open(EAppMsgType::Ok, Error);
        return FReply::Handled();
    }
    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
```

- [ ] **Step 3: 빌드 후 UI 검증**

에디터 실행 → Window → Spy GA Creator:
- Parent Class 드롭다운 열기 → UGameplayAbility 및 서브클래스 목록 표시 확인
- GA Name 입력 → Output Path 가 실시간으로 `GA_<입력값>` 으로 바뀌는지 확인
- 6개 Tag 콤보, 3개 Policy 드롭다운, 2개 GE Class 피커 표시 확인
- 이름 없이 Create GA 클릭 → "GA 이름을 입력하세요" 오류 다이얼로그 표시 확인
- 영숫자 외 문자 입력 후 Create GA 클릭 → "영숫자와 언더스코어" 오류 다이얼로그 표시 확인

- [ ] **Step 4: 커밋 준비**

```
git add SkillProject/Source/SpyGACreatorTool/Public/SSpyCreateGADialog.h
git add SkillProject/Source/SpyGACreatorTool/Private/SSpyCreateGADialog.cpp
```

커밋 메시지: `[Feature] SSpyCreateGADialog — 전체 UI 구현 (Tags, Policies, Costs, Cooldowns)`

---

## Task 4: Blueprint 생성 플로우

**Files:**
- Modify: `SkillProject/Source/SpyGACreatorTool/Private/SSpyCreateGADialog.cpp`

- [ ] **Step 1: include 추가**

`SSpyCreateGADialog.cpp` 의 기존 include 블록 끝에 추가:

```cpp
#include "KismetEditorUtilities.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "UObject/SavePackage.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Editor.h"
```

- [ ] **Step 2: Policy 변환 헬퍼 추가**

`#define LOCTEXT_NAMESPACE` 바로 아래에 추가:

```cpp
static EGameplayAbilityNetExecutionPolicy::Type ParseNetExecPolicy(const FString& S)
{
    if (S == TEXT("LocalOnly"))       return EGameplayAbilityNetExecutionPolicy::LocalOnly;
    if (S == TEXT("ServerInitiated")) return EGameplayAbilityNetExecutionPolicy::ServerInitiated;
    if (S == TEXT("ServerOnly"))      return EGameplayAbilityNetExecutionPolicy::ServerOnly;
    return EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

static EGameplayAbilityInstancingPolicy::Type ParseInstancingPolicy(const FString& S)
{
    if (S == TEXT("NonInstanced"))          return EGameplayAbilityInstancingPolicy::NonInstanced;
    if (S == TEXT("InstancedPerExecution")) return EGameplayAbilityInstancingPolicy::InstancedPerExecution;
    return EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

static EGameplayAbilityNetSecurityPolicy::Type ParseNetSecPolicy(const FString& S)
{
    if (S == TEXT("ServerOnlyExecution")) return EGameplayAbilityNetSecurityPolicy::ServerOnlyExecution;
    if (S == TEXT("ClientOnlyExecution")) return EGameplayAbilityNetSecurityPolicy::ClientOnlyExecution;
    if (S == TEXT("ServerOnly"))          return EGameplayAbilityNetSecurityPolicy::ServerOnly;
    return EGameplayAbilityNetSecurityPolicy::ClientOrServer;
}
```

- [ ] **Step 3: OnCreateClicked 전체 교체**

기존 `OnCreateClicked` 스텁을 아래로 교체:

```cpp
FReply SSpyCreateGADialog::OnCreateClicked()
{
    FText Error;
    if (!ValidateInputs(Error))
    {
        FMessageDialog::Open(EAppMsgType::Ok, Error);
        return FReply::Handled();
    }

    // 1. 패키지 경로 결정 및 생성
    const FString BlueprintName = FString::Printf(TEXT("GA_%s"), *GAName);
    const FString PackagePath   = FString::Printf(
        TEXT("/Game/Spy/Blueprints/GameplayAbilities/%s"), *BlueprintName);

    UPackage* Package = CreatePackage(*PackagePath);
    if (!Package)
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("ErrPackage", "패키지 생성에 실패했습니다."));
        return FReply::Handled();
    }

    // 2. Blueprint 생성
    UBlueprint* NewBP = FKismetEditorUtilities::CreateBlueprint(
        SelectedClass,
        Package,
        FName(*BlueprintName),
        BPTYPE_Normal,
        UBlueprint::StaticClass(),
        UBlueprintGeneratedClass::StaticClass()
    );
    if (!NewBP)
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("ErrBPCreate", "Blueprint 생성에 실패했습니다."));
        return FReply::Handled();
    }

    // 3. 1차 컴파일 — GeneratedClass 준비
    FKismetEditorUtilities::CompileBlueprint(NewBP);

    // 4. CDO에 설정값 반영
    if (NewBP->GeneratedClass)
    {
        if (UGameplayAbility* CDO = NewBP->GeneratedClass->GetDefaultObject<UGameplayAbility>())
        {
            CDO->Modify();

            CDO->AbilityTags            = AbilityTagsContainer;
            CDO->ActivationOwnedTags    = ActivationOwnedTagsContainer;
            CDO->ActivationRequiredTags = ActivationRequiredTagsContainer;
            CDO->ActivationBlockedTags  = ActivationBlockedTagsContainer;
            CDO->CancelAbilitiesWithTag = CancelAbilitiesWithTagContainer;
            CDO->BlockAbilitiesWithTag  = BlockAbilitiesWithTagContainer;

            CDO->NetExecutionPolicy = ParseNetExecPolicy(
                SelectedNetExecPolicy.IsValid() ? *SelectedNetExecPolicy : TEXT("LocalPredicted"));
            CDO->InstancingPolicy = ParseInstancingPolicy(
                SelectedInstancingPolicy.IsValid() ? *SelectedInstancingPolicy : TEXT("InstancedPerActor"));
            CDO->NetSecurityPolicy = ParseNetSecPolicy(
                SelectedNetSecPolicy.IsValid() ? *SelectedNetSecPolicy : TEXT("ClientOrServer"));

            CDO->CostGameplayEffectClass     = CostGEClass;
            CDO->CooldownGameplayEffectClass = CooldownGEClass;

            CDO->MarkPackageDirty();
        }
    }

    // 5. 2차 컴파일 — CDO 변경 반영
    FKismetEditorUtilities::CompileBlueprint(NewBP);

    // 6. 저장
    FString PackageFilename;
    if (FPackageName::TryConvertLongPackageNameToFilename(
            Package->GetName(), PackageFilename, FPackageName::GetAssetPackageExtension()))
    {
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        UPackage::SavePackage(Package, NewBP, *PackageFilename, SaveArgs);
    }

    // 7. Blueprint 에디터 열기
    if (UAssetEditorSubsystem* EdSub = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
    {
        EdSub->OpenEditorForAsset(NewBP);
    }

    return FReply::Handled();
}
```

- [ ] **Step 4: 빌드 후 전체 플로우 검증**

에디터 실행 → Window → Spy GA Creator:

1. Parent Class: `USKGameplayAbility_SkillAction` 선택
2. GA Name: `TestSkillGA` 입력 (Output Path 가 `.../GA_TestSkillGA` 로 표시되는지 확인)
3. Ability Tags: 태그 하나 추가
4. Net Execution Policy: `ServerOnly` 선택
5. "Create GA" 클릭
6. Content Browser에서 `/Game/Spy/Blueprints/GameplayAbilities/GA_TestSkillGA` 에셋 생성 확인
7. Blueprint 에디터가 자동으로 열리는지 확인
8. Blueprint 에디터 → Class Defaults → Ability Tags 에 입력한 태그가 있는지, Net Execution Policy 가 `ServerOnly` 인지 확인
9. 같은 이름으로 한 번 더 Create GA 클릭 → "에셋이 이미 존재합니다" 오류 다이얼로그 표시 확인

- [ ] **Step 5: 커밋 준비**

```
git add SkillProject/Source/SpyGACreatorTool/Private/SSpyCreateGADialog.cpp
```

커밋 메시지: `[Feature] SSpyCreateGADialog — Blueprint 생성 및 CDO 설정 완료`
