# SpyDataEditorTool Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** `Content/Spy/Data/`의 6개 DataAsset과 `Content/Spy/Data/Config/`의 4개 DataAsset을 한 곳에서 Scan → 편집 → Apply 할 수 있는 Unreal Editor 전용 플러그인 탭을 구축한다. 두 경로는 탭으로 구분해 세팅한다.

**Architecture:** Editor 전용 모듈(`SpyDataEditorTool`)을 신규 생성하고, `IDetailsView`를 Assets / Ability / Config 3개 탭에 임베드한다. Assets·Ability 탭은 `Spy/Data/` 에셋을, Config 탭은 `Spy/Data/Config/` 에셋을 담당한다. `SpyDataScanner`가 AssetRegistry로 `Content/Spy/`를 스캔해 프로퍼티 핸들에 값을 주입하고, Apply 시 Diff 다이얼로그를 거쳐 uasset을 저장한다. 커스터마이징 3종(경로 필터 + 태그 필터 + Copy 버튼)을 `IDetailCustomization` / `IPropertyTypeCustomization`으로 구현한다.

**Tech Stack:** Unreal Engine 5.7 C++, Slate UI, PropertyEditor, AssetRegistry, UnrealEd, GameplayTags

---

## 파일 맵

**신규 생성:**
```
SkillProject/Source/SpyDataEditorTool/
├── SpyDataEditorTool.Build.cs
├── Public/
│   ├── SpyDataEditorTool.h
│   ├── Tabs/
│   │   ├── SSpyAssetsTab.h       (Spy/Data/ — SpyAssetData, SpyAnimAssetData)
│   │   ├── SSpyAbilityTab.h      (Spy/Data/ — CharacterAssetData, Ability, Combo)
│   │   └── SSpyConfigTab.h       (Spy/Data/Config/ — AI/Character/Input/Movement)
│   ├── Customizations/
│   │   ├── SpyArrayCopyCustomization.h
│   │   ├── SpyAssetPathCustomization.h
│   │   └── SpyGameplayTagCustomization.h
│   └── Utils/
│       └── SpyDataScanner.h
└── Private/
    ├── SpyDataEditorTool.cpp
    ├── Tabs/
    │   ├── SSpyAssetsTab.cpp
    │   ├── SSpyAbilityTab.cpp
    │   └── SSpyConfigTab.cpp
    ├── Customizations/
    │   ├── SpyArrayCopyCustomization.cpp
    │   ├── SpyAssetPathCustomization.cpp
    │   └── SpyGameplayTagCustomization.cpp
    └── Utils/
        ├── SpyDataScanner.cpp
        └── SpyEditorUtils.h      (Private 헤더 — SaveAsset + ConfirmApply)
```

**수정:**
- `SkillProject/SkillProject.uproject` — SpyDataEditorTool 모듈 추가
- `SkillProject/Source/SkillProjectEditor.Target.cs` — ExtraModuleNames 추가
- `SkillProject/Source/SkillProject/Data/SpyCharacterAssetData.h` — ClassType에 meta=(Categories) 추가

---

## Task 1: 모듈 스캐폴딩

**Files:**
- Create: `SkillProject/Source/SpyDataEditorTool/SpyDataEditorTool.Build.cs`
- Create: `SkillProject/Source/SpyDataEditorTool/Public/SpyDataEditorTool.h`
- Create: `SkillProject/Source/SpyDataEditorTool/Private/SpyDataEditorTool.cpp`
- Modify: `SkillProject/SkillProject.uproject`
- Modify: `SkillProject/Source/SkillProjectEditor.Target.cs`

- [ ] **Step 1: Build.cs 생성**

```csharp
// SpyDataEditorTool.Build.cs
using UnrealBuildTool;

public class SpyDataEditorTool : ModuleRules
{
    public SpyDataEditorTool(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "SkillProject", "GameplayTags"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "UnrealEd", "PropertyEditor", "AssetRegistry",
            "Slate", "SlateCore", "InputCore",
            "ToolMenus", "EditorFramework", "EditorWidgets",
            "EditorSubsystem", "AssetTools"
        });
    }
}
```

- [ ] **Step 2: 모듈 헤더 생성**

```cpp
// Public/SpyDataEditorTool.h
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FSpyDataEditorToolModule : public IModuleInterface
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

- [ ] **Step 3: 모듈 cpp 생성 (빈 구현)**

```cpp
// Private/SpyDataEditorTool.cpp
#include "SpyDataEditorTool.h"
#include "Framework/Docking/TabManager.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FSpyDataEditorToolModule"

const FName FSpyDataEditorToolModule::TabName = TEXT("SpyDataEditorTool");

void FSpyDataEditorToolModule::StartupModule()
{
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        TabName,
        FOnSpawnTab::CreateRaw(this, &FSpyDataEditorToolModule::OnSpawnTab))
        .SetDisplayName(LOCTEXT("TabTitle", "Spy Data Editor"))
        .SetTooltipText(LOCTEXT("TabTooltip", "Spy Data 에셋 일괄 편집 도구"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);

    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FSpyDataEditorToolModule::RegisterMenus));
}

void FSpyDataEditorToolModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);
}

TSharedRef<SDockTab> FSpyDataEditorToolModule::OnSpawnTab(const FSpawnTabArgs& SpawnTabArgs)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(STextBlock).Text(LOCTEXT("Placeholder", "Spy Data Editor - WIP"))
        ];
}

void FSpyDataEditorToolModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);
    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
    FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
    Section.AddMenuEntryWithCommandList(
        FUIAction(FExecuteAction::CreateLambda([]()
        {
            FGlobalTabmanager::Get()->TryInvokeTab(FSpyDataEditorToolModule::TabName);
        })),
        LOCTEXT("MenuEntry", "Spy Data Editor"),
        LOCTEXT("MenuEntryTooltip", "Spy Data 에셋 일괄 편집 도구 열기"),
        FSlateIcon()
    );
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSpyDataEditorToolModule, SpyDataEditorTool)
```

- [ ] **Step 4: uproject에 모듈 추가**

`SkillProject/SkillProject.uproject`의 `"Modules"` 배열에 추가:
```json
{
    "Name": "SpyDataEditorTool",
    "Type": "Editor",
    "LoadingPhase": "PostEngineInit"
}
```

- [ ] **Step 5: Editor.Target.cs에 모듈 추가**

`SkillProject/Source/SkillProjectEditor.Target.cs`:
```csharp
ExtraModuleNames.Add("SpyDataEditorTool");
```

- [ ] **Step 6: 빌드 확인**

언리얼 에디터에서 프로젝트 다시 열거나 Visual Studio에서 빌드.
기대: 컴파일 오류 없이 성공. `Window` 메뉴에 "Spy Data Editor" 항목 표시.

- [ ] **Step 7: 커밋**

```bash
git add SkillProject/Source/SpyDataEditorTool/ SkillProject/SkillProject.uproject SkillProject/Source/SkillProjectEditor.Target.cs
git commit -m "[SpyDataEditorTool] 에디터 모듈 스캐폴딩 추가"
```

---

## Task 2: SpyDataScanner 유틸리티

**Files:**
- Create: `SkillProject/Source/SpyDataEditorTool/Public/Utils/SpyDataScanner.h`
- Create: `SkillProject/Source/SpyDataEditorTool/Private/Utils/SpyDataScanner.cpp`

- [ ] **Step 1: SpyDataScanner 헤더 생성**

```cpp
// Public/Utils/SpyDataScanner.h
#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"

/**
 * Content/Spy/ 하위 에셋을 스캔해 DataAsset 프로퍼티에 적용할 값을 수집한다.
 * 반환값은 AssetName → SoftObjectPath 맵.
 */
struct FSpyDataScanner
{
    /** Content/Spy/ 하위의 UAnimInstance 서브클래스를 스캔해 AnimLayerMap용 맵 반환 */
    static TMap<FName, FSoftObjectPath> ScanAnimLayers();

    /** Content/Spy/ 하위의 에셋 전체를 그룹별로 스캔해 AssetGroupNameToSet용 맵 반환 */
    static TMap<FName, TArray<FSoftObjectPath>> ScanAssetGroups();

    /** 지정 경로 하위의 특정 클래스 에셋 경로 목록 반환 */
    static TArray<FSoftObjectPath> ScanDataAssetsByClass(const FString& Path, UClass* AssetClass);

private:
    static TArray<FAssetData> GetAssetsUnderPath(const FString& Path, UClass* FilterClass = nullptr);
};
```

- [ ] **Step 2: SpyDataScanner 구현**

```cpp
// Private/Utils/SpyDataScanner.cpp
#include "Utils/SpyDataScanner.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Animation/AnimInstance.h"

static IAssetRegistry& GetRegistry()
{
    return FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
}

TArray<FAssetData> FSpyDataScanner::GetAssetsUnderPath(const FString& Path, UClass* FilterClass)
{
    TArray<FAssetData> Result;
    FARFilter Filter;
    Filter.PackagePaths.Add(*Path);
    Filter.bRecursivePaths = true;
    if (FilterClass)
    {
        Filter.ClassPaths.Add(FilterClass->GetClassPathName());
        Filter.bRecursiveClasses = true;
    }
    GetRegistry().GetAssets(Filter, Result);
    return Result;
}

TMap<FName, FSoftObjectPath> FSpyDataScanner::ScanAnimLayers()
{
    TMap<FName, FSoftObjectPath> Result;
    TArray<FAssetData> Assets = GetAssetsUnderPath(TEXT("/Game/Spy/Animation"), UAnimInstance::StaticClass());
    for (const FAssetData& Asset : Assets)
    {
        FString Name = Asset.AssetName.ToString();
        FString Key;
        if (Name.StartsWith(TEXT("ABP_AnimLayers_")))
        {
            Key = Name.RightChop(FString(TEXT("ABP_AnimLayers_")).Len());
            Result.Add(FName(*Key), FSoftObjectPath(Asset.GetSoftObjectPath()));
        }
    }
    return Result;
}

TMap<FName, TArray<FSoftObjectPath>> FSpyDataScanner::ScanAssetGroups()
{
    TMap<FName, TArray<FSoftObjectPath>> Result;
    TArray<FAssetData> Assets = GetAssetsUnderPath(TEXT("/Game/Spy"));

    for (const FAssetData& Asset : Assets)
    {
        FString PackagePath = Asset.PackagePath.ToString();
        FString Remainder = PackagePath.RightChop(FString(TEXT("/Game/Spy/")).Len());
        FString GroupName;
        Remainder.Split(TEXT("/"), &GroupName, nullptr);
        if (GroupName.IsEmpty()) GroupName = Remainder;
        if (!GroupName.IsEmpty())
        {
            Result.FindOrAdd(FName(*GroupName)).Add(FSoftObjectPath(Asset.GetSoftObjectPath()));
        }
    }
    return Result;
}

TArray<FSoftObjectPath> FSpyDataScanner::ScanDataAssetsByClass(const FString& Path, UClass* AssetClass)
{
    TArray<FSoftObjectPath> Result;
    TArray<FAssetData> Assets = GetAssetsUnderPath(Path, AssetClass);
    for (const FAssetData& Asset : Assets)
    {
        Result.Add(FSoftObjectPath(Asset.GetSoftObjectPath()));
    }
    return Result;
}
```

- [ ] **Step 3: 빌드 확인**

빌드 기대: 컴파일 오류 없이 성공.

- [ ] **Step 4: 커밋**

```bash
git add SkillProject/Source/SpyDataEditorTool/Public/Utils/ SkillProject/Source/SpyDataEditorTool/Private/Utils/
git commit -m "[SpyDataEditorTool] SpyDataScanner 유틸리티 추가"
```

---

## Task 3: 4개 탭 IDetailsView 기본 구조

**Files:**
- Create: `Public/Tabs/SSpyAssetsTab.h`, `Private/Tabs/SSpyAssetsTab.cpp`
- Create: `Public/Tabs/SSpyAbilityTab.h`, `Private/Tabs/SSpyAbilityTab.cpp`
- Create: `Public/Tabs/SSpyConfigTab.h`, `Private/Tabs/SSpyConfigTab.cpp`
- Modify: `Private/SpyDataEditorTool.cpp`

- [ ] **Step 1: SSpyAssetsTab 헤더**

```cpp
// Public/Tabs/SSpyAssetsTab.h
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class IDetailsView;
class USpyAssetData;
class USpyAnimAssetData;

class SSpyAssetsTab : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SSpyAssetsTab) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    FReply OnScanClicked();
    FReply OnApplyClicked();

    TSharedPtr<IDetailsView> AssetDataView;
    TSharedPtr<IDetailsView> AnimAssetDataView;

    USpyAssetData* AssetData = nullptr;
    USpyAnimAssetData* AnimAssetData = nullptr;
};
```

- [ ] **Step 2: SSpyAssetsTab 구현 (Scan/Apply는 빈 stub)**

```cpp
// Private/Tabs/SSpyAssetsTab.cpp
#include "Tabs/SSpyAssetsTab.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Data/SpyAssetData.h"
#include "Data/SpyAnimAssetData.h"

#define LOCTEXT_NAMESPACE "SSpyAssetsTab"

static UObject* LoadDataAssetByClass(UClass* Class, const FString& SearchPath = TEXT("/Game/Spy/Data"))
{
    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
    TArray<FAssetData> Found;
    FARFilter Filter;
    Filter.PackagePaths.Add(*SearchPath);
    Filter.ClassPaths.Add(Class->GetClassPathName());
    Filter.bRecursiveClasses = true;
    Registry.GetAssets(Filter, Found);
    return Found.Num() > 0 ? Found[0].GetAsset() : nullptr;
}

void SSpyAssetsTab::Construct(const FArguments& InArgs)
{
    FPropertyEditorModule& PropModule =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    FDetailsViewArgs Args;
    Args.bHideSelectionTip = true;
    Args.bAllowSearch = false;
    Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;

    AssetDataView = PropModule.CreateDetailView(Args);
    AnimAssetDataView = PropModule.CreateDetailView(Args);

    AssetData = Cast<USpyAssetData>(LoadDataAssetByClass(USpyAssetData::StaticClass()));
    AnimAssetData = Cast<USpyAnimAssetData>(LoadDataAssetByClass(USpyAnimAssetData::StaticClass()));

    if (AssetData) AssetDataView->SetObject(AssetData);
    if (AnimAssetData) AnimAssetDataView->SetObject(AnimAssetData);

    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(4)
        [
            SNew(SButton)
            .Text(LOCTEXT("Scan", "Scan"))
            .OnClicked(this, &SSpyAssetsTab::OnScanClicked)
        ]
        + SVerticalBox::Slot().FillHeight(1.f)
        [
            SNew(SScrollBox)
            + SScrollBox::Slot()
            [ SNew(STextBlock).Text(LOCTEXT("AssetDataLabel", "SpyAssetData")) ]
            + SScrollBox::Slot()
            [ AssetDataView.ToSharedRef() ]
            + SScrollBox::Slot().Padding(FMargin(0, 8, 0, 0))
            [ SNew(STextBlock).Text(LOCTEXT("AnimDataLabel", "SpyAnimAssetData")) ]
            + SScrollBox::Slot()
            [ AnimAssetDataView.ToSharedRef() ]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(4)
        [
            SNew(SButton)
            .Text(LOCTEXT("Apply", "Apply Assets"))
            .OnClicked(this, &SSpyAssetsTab::OnApplyClicked)
        ]
    ];
}

FReply SSpyAssetsTab::OnScanClicked() { return FReply::Handled(); }
FReply SSpyAssetsTab::OnApplyClicked() { return FReply::Handled(); }

#undef LOCTEXT_NAMESPACE
```

- [ ] **Step 3: SSpyAbilityTab 헤더**

```cpp
// Public/Tabs/SSpyAbilityTab.h
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class IDetailsView;
class USpyCharacterAssetData;
class USpyAbilityData;
class USpyComboAssetData;

class SSpyAbilityTab : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SSpyAbilityTab) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    FReply OnScanClicked();
    FReply OnApplyClicked();

    TSharedPtr<IDetailsView> CharacterAssetView;
    TSharedPtr<IDetailsView> CommonAbilityView;
    TSharedPtr<IDetailsView> NormalAbilityView;
    TSharedPtr<IDetailsView> ComboAssetView;

    USpyCharacterAssetData* CharacterAssetData = nullptr;
    USpyAbilityData* CommonAbility = nullptr;
    USpyAbilityData* NormalAbility = nullptr;
    USpyComboAssetData* ComboAssetData = nullptr;
};
```

- [ ] **Step 4: SSpyAbilityTab 구현 (stub)**

```cpp
// Private/Tabs/SSpyAbilityTab.cpp
#include "Tabs/SSpyAbilityTab.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Data/SpyCharacterAssetData.h"
#include "Data/SpyAbilityData.h"
#include "Data/SpyComboAssetData.h"

#define LOCTEXT_NAMESPACE "SSpyAbilityTab"

static UObject* LoadAssetByClassAndName(UClass* Class, const FString& AssetName)
{
    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
    TArray<FAssetData> Found;
    FARFilter Filter;
    Filter.PackagePaths.Add(TEXT("/Game/Spy/Data"));
    Filter.ClassPaths.Add(Class->GetClassPathName());
    Filter.bRecursiveClasses = true;
    Registry.GetAssets(Filter, Found);
    for (const FAssetData& A : Found)
    {
        if (A.AssetName.ToString() == AssetName) return A.GetAsset();
    }
    return Found.Num() > 0 ? Found[0].GetAsset() : nullptr;
}

void SSpyAbilityTab::Construct(const FArguments& InArgs)
{
    FPropertyEditorModule& PropModule =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    FDetailsViewArgs Args;
    Args.bHideSelectionTip = true;
    Args.bAllowSearch = false;
    Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;

    CharacterAssetView = PropModule.CreateDetailView(Args);
    CommonAbilityView  = PropModule.CreateDetailView(Args);
    NormalAbilityView  = PropModule.CreateDetailView(Args);
    ComboAssetView     = PropModule.CreateDetailView(Args);

    CharacterAssetData = Cast<USpyCharacterAssetData>(
        LoadAssetByClassAndName(USpyCharacterAssetData::StaticClass(), TEXT("SpyCharacterAssetData")));
    CommonAbility = Cast<USpyAbilityData>(
        LoadAssetByClassAndName(USpyAbilityData::StaticClass(), TEXT("SpyCommonCharacterAbility")));
    NormalAbility = Cast<USpyAbilityData>(
        LoadAssetByClassAndName(USpyAbilityData::StaticClass(), TEXT("SpyNormalAbility")));
    ComboAssetData = Cast<USpyComboAssetData>(
        LoadAssetByClassAndName(USpyComboAssetData::StaticClass(), TEXT("SpyNormalComboAssetData")));

    if (CharacterAssetData) CharacterAssetView->SetObject(CharacterAssetData);
    if (CommonAbility)      CommonAbilityView->SetObject(CommonAbility);
    if (NormalAbility)      NormalAbilityView->SetObject(NormalAbility);
    if (ComboAssetData)     ComboAssetView->SetObject(ComboAssetData);

    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(4)
        [
            SNew(SButton)
            .Text(LOCTEXT("Scan", "Scan"))
            .OnClicked(this, &SSpyAbilityTab::OnScanClicked)
        ]
        + SVerticalBox::Slot().FillHeight(1.f)
        [
            SNew(SScrollBox)
            + SScrollBox::Slot()
            [ SNew(STextBlock).Text(LOCTEXT("CharLabel", "SpyCharacterAssetData")) ]
            + SScrollBox::Slot() [ CharacterAssetView.ToSharedRef() ]
            + SScrollBox::Slot().Padding(FMargin(0,8,0,0))
            [ SNew(STextBlock).Text(LOCTEXT("CommonLabel", "SpyCommonCharacterAbility")) ]
            + SScrollBox::Slot() [ CommonAbilityView.ToSharedRef() ]
            + SScrollBox::Slot().Padding(FMargin(0,8,0,0))
            [ SNew(STextBlock).Text(LOCTEXT("NormalLabel", "SpyNormalAbility")) ]
            + SScrollBox::Slot() [ NormalAbilityView.ToSharedRef() ]
            + SScrollBox::Slot().Padding(FMargin(0,8,0,0))
            [ SNew(STextBlock).Text(LOCTEXT("ComboLabel", "SpyNormalComboAssetData")) ]
            + SScrollBox::Slot() [ ComboAssetView.ToSharedRef() ]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(4)
        [
            SNew(SButton)
            .Text(LOCTEXT("Apply", "Apply Abilities"))
            .OnClicked(this, &SSpyAbilityTab::OnApplyClicked)
        ]
    ];
}

FReply SSpyAbilityTab::OnScanClicked() { return FReply::Handled(); }
FReply SSpyAbilityTab::OnApplyClicked() { return FReply::Handled(); }

#undef LOCTEXT_NAMESPACE
```

- [ ] **Step 5: SSpyConfigTab 헤더**

```cpp
// Public/Tabs/SSpyConfigTab.h
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class IDetailsView;
class USpyAIConfig;
class USpyCharacterConfig;
class USpyInputConfig;
class USpyMovementConfig;

/** Spy/Data/Config/ 하위 4개 DataAsset을 편집하는 탭 */
class SSpyConfigTab : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SSpyConfigTab) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    void LoadAssets();
    FReply OnScanClicked();
    FReply OnApplyClicked();

    TSharedPtr<IDetailsView> AIConfigView;
    TSharedPtr<IDetailsView> CharacterConfigView;
    TSharedPtr<IDetailsView> InputConfigView;
    TSharedPtr<IDetailsView> MovementConfigView;

    USpyAIConfig*         AIConfig         = nullptr;
    USpyCharacterConfig*  CharacterConfig  = nullptr;
    USpyInputConfig*      InputConfig      = nullptr;
    USpyMovementConfig*   MovementConfig   = nullptr;
};
```

- [ ] **Step 8: SSpyConfigTab 구현**

```cpp
// Private/Tabs/SSpyConfigTab.cpp
#include "Tabs/SSpyConfigTab.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Data/SpyAIConfig.h"
#include "Data/SpyCharacterConfig.h"
#include "Data/SpyMovementConfig.h"
#include "Input/SpyInputConfig.h"

#define LOCTEXT_NAMESPACE "SSpyConfigTab"

// Spy/Data/Config/ 경로 전용 로더
static UObject* LoadConfigAsset(UClass* Class)
{
    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
    TArray<FAssetData> Found;
    FARFilter Filter;
    Filter.PackagePaths.Add(TEXT("/Game/Spy/Data/Config"));
    Filter.bRecursivePaths = false;
    Filter.ClassPaths.Add(Class->GetClassPathName());
    Filter.bRecursiveClasses = true;
    Registry.GetAssets(Filter, Found);
    return Found.Num() > 0 ? Found[0].GetAsset() : nullptr;
}

void SSpyConfigTab::LoadAssets()
{
    AIConfig        = Cast<USpyAIConfig>(LoadConfigAsset(USpyAIConfig::StaticClass()));
    CharacterConfig = Cast<USpyCharacterConfig>(LoadConfigAsset(USpyCharacterConfig::StaticClass()));
    InputConfig     = Cast<USpyInputConfig>(LoadConfigAsset(USpyInputConfig::StaticClass()));
    MovementConfig  = Cast<USpyMovementConfig>(LoadConfigAsset(USpyMovementConfig::StaticClass()));

    AIConfigView->SetObject(AIConfig);
    CharacterConfigView->SetObject(CharacterConfig);
    InputConfigView->SetObject(InputConfig);
    MovementConfigView->SetObject(MovementConfig);
}

void SSpyConfigTab::Construct(const FArguments& InArgs)
{
    FPropertyEditorModule& PropModule =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    FDetailsViewArgs Args;
    Args.bHideSelectionTip = true;
    Args.bAllowSearch = false;
    Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;

    AIConfigView        = PropModule.CreateDetailView(Args);
    CharacterConfigView = PropModule.CreateDetailView(Args);
    InputConfigView     = PropModule.CreateDetailView(Args);
    MovementConfigView  = PropModule.CreateDetailView(Args);

    LoadAssets();

    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(4)
        [
            SNew(SButton)
            .Text(LOCTEXT("Scan", "Scan"))
            .OnClicked(this, &SSpyConfigTab::OnScanClicked)
        ]
        + SVerticalBox::Slot().FillHeight(1.f)
        [
            SNew(SScrollBox)
            + SScrollBox::Slot()
            [ SNew(STextBlock).Text(LOCTEXT("AILabel", "SpyAIConfig")) ]
            + SScrollBox::Slot() [ AIConfigView.ToSharedRef() ]
            + SScrollBox::Slot().Padding(FMargin(0,8,0,0))
            [ SNew(STextBlock).Text(LOCTEXT("CharLabel", "SpyCharacterConfig")) ]
            + SScrollBox::Slot() [ CharacterConfigView.ToSharedRef() ]
            + SScrollBox::Slot().Padding(FMargin(0,8,0,0))
            [ SNew(STextBlock).Text(LOCTEXT("InputLabel", "SpyInputConfig")) ]
            + SScrollBox::Slot() [ InputConfigView.ToSharedRef() ]
            + SScrollBox::Slot().Padding(FMargin(0,8,0,0))
            [ SNew(STextBlock).Text(LOCTEXT("MoveLabel", "SpyMovementConfig")) ]
            + SScrollBox::Slot() [ MovementConfigView.ToSharedRef() ]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(4)
        [
            SNew(SButton)
            .Text(LOCTEXT("Apply", "Apply Config"))
            .OnClicked(this, &SSpyConfigTab::OnApplyClicked)
        ]
    ];
}

FReply SSpyConfigTab::OnScanClicked()
{
    // Spy/Data/Config/ 에셋을 재수집해 DetailsView 갱신
    LoadAssets();
    return FReply::Handled();
}

FReply SSpyConfigTab::OnApplyClicked() { return FReply::Handled(); }

#undef LOCTEXT_NAMESPACE
```

- [ ] **Step 7: OnSpawnTab에 3개 탭 연결**

`Private/SpyDataEditorTool.cpp`의 `OnSpawnTab` 함수 교체:

```cpp
#include "Tabs/SSpyAssetsTab.h"
#include "Tabs/SSpyAbilityTab.h"
#include "Tabs/SSpyConfigTab.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

TSharedRef<SDockTab> FSpyDataEditorToolModule::OnSpawnTab(const FSpawnTabArgs& SpawnTabArgs)
{
    TSharedRef<SSpyAssetsTab>  AssetsTab  = SNew(SSpyAssetsTab);
    TSharedRef<SSpyAbilityTab> AbilityTab = SNew(SSpyAbilityTab);
    TSharedRef<SSpyConfigTab>  ConfigTab  = SNew(SSpyConfigTab);

    TSharedRef<SWidgetSwitcher> Switcher = SNew(SWidgetSwitcher)
        + SWidgetSwitcher::Slot() [ AssetsTab ]   // index 0
        + SWidgetSwitcher::Slot() [ AbilityTab ]  // index 1
        + SWidgetSwitcher::Slot() [ ConfigTab ];  // index 2

    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SVerticalBox)
            // 탭 전환 버튼 행
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SButton).Text(LOCTEXT("TabAssets", "Assets"))
                    .ToolTipText(LOCTEXT("TabAssetsTip", "Spy/Data/ — SpyAssetData, SpyAnimAssetData"))
                    .OnClicked_Lambda([Switcher]() { Switcher->SetActiveWidgetIndex(0); return FReply::Handled(); })
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SButton).Text(LOCTEXT("TabAbility", "Ability"))
                    .ToolTipText(LOCTEXT("TabAbilityTip", "Spy/Data/ — CharacterAssetData, Ability, Combo"))
                    .OnClicked_Lambda([Switcher]() { Switcher->SetActiveWidgetIndex(1); return FReply::Handled(); })
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SButton).Text(LOCTEXT("TabConfig", "Config"))
                    .ToolTipText(LOCTEXT("TabConfigTip", "Spy/Data/Config/ — AI/Character/Input/Movement"))
                    .OnClicked_Lambda([Switcher]() { Switcher->SetActiveWidgetIndex(2); return FReply::Handled(); })
                ]
            ]
            // 탭 컨텐츠
            + SVerticalBox::Slot().FillHeight(1.f)
            [
                Switcher
            ]
        ];
}
```

> **탭 구분 원칙:** Assets(0) · Ability(1) 두 탭은 `/Game/Spy/Data` 직속 에셋만 로드한다. Config(2) 탭은 `/Game/Spy/Data/Config` 경로만 사용하며 `bRecursivePaths = false`로 경로를 격리한다.

- [ ] **Step 8: 빌드 및 에디터에서 3탭 확인**

빌드 후 에디터의 `Window > Spy Data Editor` 클릭.  
기대: Assets / Ability / Config 버튼 클릭 시 각 에셋의 Details View가 표시됨.

- [ ] **Step 9: 커밋**

```bash
git add SkillProject/Source/SpyDataEditorTool/
git commit -m "[SpyDataEditorTool] 3탭 IDetailsView 기본 구조 구현"
```

---

## Task 4: Assets 탭 Scan 로직 연결

**Files:**
- Modify: `Private/Tabs/SSpyAssetsTab.cpp`

- [ ] **Step 1: OnScanClicked 구현**

```cpp
#include "Utils/SpyDataScanner.h"

FReply SSpyAssetsTab::OnScanClicked()
{
    if (!AnimAssetData) return FReply::Handled();

    TMap<FName, FSoftObjectPath> AnimLayers = FSpyDataScanner::ScanAnimLayers();

    AnimAssetData->Modify();
    AnimAssetData->AnimLayerMap.Empty();
    for (auto& Pair : AnimLayers)
    {
        TSoftClassPtr<UAnimInstance> SoftClass(Pair.Value);
        AnimAssetData->AnimLayerMap.Add(Pair.Key, SoftClass);
    }
    AnimAssetDataView->ForceRefreshDetails();

    if (AssetData)
    {
        AssetData->Modify();
        AssetDataView->ForceRefreshDetails();
    }

    return FReply::Handled();
}
```

- [ ] **Step 2: 빌드 및 Scan 버튼 동작 확인**

빌드 후 Assets 탭에서 Scan 클릭.  
기대: AnimLayerMap에 `OHS`, `Unarmed` 키가 자동 채워짐.

- [ ] **Step 3: 커밋**

```bash
git add SkillProject/Source/SpyDataEditorTool/Private/Tabs/SSpyAssetsTab.cpp
git commit -m "[SpyDataEditorTool] Assets 탭 Scan 로직 구현"
```

---

## Task 5: Apply 로직 + Diff 다이얼로그

**Files:**
- Create: `Private/Utils/SpyEditorUtils.h`
- Modify: `Private/Tabs/SSpyAssetsTab.cpp`
- Modify: `Private/Tabs/SSpyAbilityTab.cpp`
- Modify: `Private/Tabs/SSpyConfigTab.cpp`

- [ ] **Step 1: SpyEditorUtils 헤더 생성**

```cpp
// Private/Utils/SpyEditorUtils.h
#pragma once
#include "CoreMinimal.h"
#include "Misc/MessageDialog.h"
#include "UObject/SavePackage.h"

namespace SpyEditorUtils
{
    inline bool SaveAsset(UObject* Asset)
    {
        if (!Asset) return false;
        UPackage* Package = Asset->GetOutermost();
        Package->MarkPackageDirty();
        FString PackageFilename;
        if (!FPackageName::TryConvertLongPackageNameToFilename(
                Package->GetName(), PackageFilename, FPackageName::GetAssetPackageExtension()))
            return false;
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        return UPackage::SavePackage(Package, Asset, *PackageFilename, SaveArgs)
               == ESavePackageResult::Success;
    }

    inline bool ConfirmApply(const TArray<FString>& AssetNames)
    {
        FString Message = TEXT("다음 에셋을 저장합니다:\n\n");
        for (const FString& Name : AssetNames) Message += TEXT("  • ") + Name + TEXT("\n");
        Message += TEXT("\n계속하시겠습니까?");
        return FMessageDialog::Open(EAppMsgType::OkCancel,
            FText::FromString(Message)) == EAppReturnType::Ok;
    }
}
```

- [ ] **Step 2: SSpyAssetsTab OnApplyClicked 구현**

```cpp
#include "Utils/SpyEditorUtils.h"

FReply SSpyAssetsTab::OnApplyClicked()
{
    TArray<FString> Names;
    if (AssetData)     Names.Add(TEXT("SpyAssetData"));
    if (AnimAssetData) Names.Add(TEXT("SpyAnimAssetData"));

    if (!SpyEditorUtils::ConfirmApply(Names)) return FReply::Handled();

    int32 Saved = 0, Failed = 0;
    if (SpyEditorUtils::SaveAsset(AssetData))     ++Saved; else ++Failed;
    if (SpyEditorUtils::SaveAsset(AnimAssetData)) ++Saved; else ++Failed;

    FMessageDialog::Open(EAppMsgType::Ok,
        FText::FromString(FString::Printf(TEXT("저장 완료: %d개 / 실패: %d개"), Saved, Failed)));
    return FReply::Handled();
}
```

- [ ] **Step 3: SSpyAbilityTab OnApplyClicked 구현**

```cpp
FReply SSpyAbilityTab::OnApplyClicked()
{
    TArray<FString> Names;
    if (CharacterAssetData) Names.Add(TEXT("SpyCharacterAssetData"));
    if (CommonAbility)      Names.Add(TEXT("SpyCommonCharacterAbility"));
    if (NormalAbility)      Names.Add(TEXT("SpyNormalAbility"));
    if (ComboAssetData)     Names.Add(TEXT("SpyNormalComboAssetData"));

    if (!SpyEditorUtils::ConfirmApply(Names)) return FReply::Handled();

    int32 Saved = 0, Failed = 0;
    if (SpyEditorUtils::SaveAsset(CharacterAssetData)) ++Saved; else ++Failed;
    if (SpyEditorUtils::SaveAsset(CommonAbility))      ++Saved; else ++Failed;
    if (SpyEditorUtils::SaveAsset(NormalAbility))      ++Saved; else ++Failed;
    if (SpyEditorUtils::SaveAsset(ComboAssetData))     ++Saved; else ++Failed;

    FMessageDialog::Open(EAppMsgType::Ok,
        FText::FromString(FString::Printf(TEXT("저장 완료: %d개 / 실패: %d개"), Saved, Failed)));
    return FReply::Handled();
}
```

- [ ] **Step 4: SSpyConfigTab OnApplyClicked 구현**

```cpp
FReply SSpyConfigTab::OnApplyClicked()
{
    TArray<FString> Names;
    if (AIConfig)        Names.Add(TEXT("SpyAIConfig"));
    if (CharacterConfig) Names.Add(TEXT("SpyCharacterConfig"));
    if (InputConfig)     Names.Add(TEXT("SpyInputConfig (Spy/Data/Config/)"));
    if (MovementConfig)  Names.Add(TEXT("SpyMovementConfig"));

    if (!SpyEditorUtils::ConfirmApply(Names)) return FReply::Handled();

    int32 Saved = 0, Failed = 0;
    if (SpyEditorUtils::SaveAsset(AIConfig))        ++Saved; else ++Failed;
    if (SpyEditorUtils::SaveAsset(CharacterConfig)) ++Saved; else ++Failed;
    if (SpyEditorUtils::SaveAsset(InputConfig))     ++Saved; else ++Failed;
    if (SpyEditorUtils::SaveAsset(MovementConfig))  ++Saved; else ++Failed;

    FMessageDialog::Open(EAppMsgType::Ok,
        FText::FromString(FString::Printf(TEXT("저장 완료: %d개 / 실패: %d개"), Saved, Failed)));
    return FReply::Handled();
}
```

- [ ] **Step 5: 빌드 및 Apply 동작 확인**

빌드 후 각 탭의 Apply 클릭.  
기대: Diff 다이얼로그 표시 → 확인 → 에셋 저장 → 결과 메시지. Config 탭은 `Spy/Data/Config/` 에셋 4개만 저장됨.

- [ ] **Step 6: 커밋**

```bash
git add SkillProject/Source/SpyDataEditorTool/
git commit -m "[SpyDataEditorTool] Apply 로직 및 Diff 다이얼로그 3탭 구현"
```

---

## Task 6: SpyAssetPathCustomization (경로 필터 + Scan 하이라이트)

**Files:**
- Create: `Public/Customizations/SpyAssetPathCustomization.h`
- Create: `Private/Customizations/SpyAssetPathCustomization.cpp`
- Modify: `Private/SpyDataEditorTool.cpp` (등록)

- [ ] **Step 1: SpyAssetPathCustomization 헤더**

```cpp
// Public/Customizations/SpyAssetPathCustomization.h
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
```

- [ ] **Step 2: SpyAssetPathCustomization 구현**

```cpp
// Private/Customizations/SpyAssetPathCustomization.cpp
#include "Customizations/SpyAssetPathCustomization.h"
#include "DetailWidgetRow.h"
#include "PropertyHandle.h"
#include "Widgets/SBoxPanel.h"
#include "PropertyCustomizationHelpers.h"

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

    TSharedRef<SWidget> AssetPickerWidget =
        PropertyCustomizationHelpers::MakeAssetPickerWithMenu(
            PropertyHandle,
            false,
            TArray<const UClass*>{},
            TArray<UFactory*>{},
            FOnShouldFilterAsset::CreateLambda([](const FAssetData& AssetData) -> bool
            {
                return !AssetData.PackagePath.ToString().StartsWith(TEXT("/Game/Spy"));
            }),
            FOnAssetSelected::CreateLambda([PropertyHandle](const FAssetData& AssetData)
            {
                PropertyHandle->SetValue(AssetData.GetSoftObjectPath().ToString());
            }),
            FSimpleDelegate()
        );

    HeaderRow.NameContent()
    [
        PropertyHandle->CreatePropertyNameWidget()
    ]
    .ValueContent()
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().FillWidth(1.f)
        [
            SNew(SBorder)
            .BorderBackgroundColor(bChanged
                ? FLinearColor(1.f, 0.9f, 0.f, 0.3f)
                : FLinearColor::Transparent)
            [
                AssetPickerWidget
            ]
        ]
    ];
}
```

- [ ] **Step 3: 모듈에 커스터마이징 등록**

`StartupModule()`에 추가:
```cpp
FPropertyEditorModule& PropModule =
    FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
PropModule.RegisterCustomPropertyTypeLayout(
    TEXT("SoftObjectPath"),
    FOnGetPropertyTypeCustomizationInstance::CreateStatic(
        &FSpyAssetPathCustomization::MakeInstance));
PropModule.NotifyCustomizationModuleChanged();
```

`ShutdownModule()`에 추가:
```cpp
PropModule.UnregisterCustomPropertyTypeLayout(TEXT("SoftObjectPath"));
```

- [ ] **Step 4: 빌드 확인**

빌드 후 에셋 피커 클릭 시 Content/Spy/ 하위만 표시 확인.

- [ ] **Step 5: 커밋**

```bash
git add SkillProject/Source/SpyDataEditorTool/
git commit -m "[SpyDataEditorTool] SpyAssetPathCustomization 경로 필터 + Scan 하이라이트 구현"
```

---

## Task 7: SpyGameplayTagCustomization (태그 필터)

**Files:**
- Create: `Public/Customizations/SpyGameplayTagCustomization.h`
- Create: `Private/Customizations/SpyGameplayTagCustomization.cpp`
- Modify: `SkillProject/Source/SkillProject/Data/SpyCharacterAssetData.h`
- Modify: `Private/SpyDataEditorTool.cpp` (등록)

- [ ] **Step 1: SpyCharacterAssetData.h ClassType에 Categories meta 추가**

```cpp
UPROPERTY(EditDefaultsOnly, meta=(Categories="Spy.Class"))
FGameplayTag ClassType;
```

- [ ] **Step 2: SpyGameplayTagCustomization 헤더**

```cpp
// Public/Customizations/SpyGameplayTagCustomization.h
#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

class FSpyGameplayTagCustomization : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle,
        FDetailWidgetRow& HeaderRow,
        IPropertyTypeCustomizationUtils& CustomizationUtils) override;

    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle,
        IDetailChildrenBuilder& ChildBuilder,
        IPropertyTypeCustomizationUtils& CustomizationUtils) override {}
};
```

- [ ] **Step 3: SpyGameplayTagCustomization 구현**

```cpp
// Private/Customizations/SpyGameplayTagCustomization.cpp
#include "Customizations/SpyGameplayTagCustomization.h"
#include "DetailWidgetRow.h"
#include "PropertyHandle.h"
#include "Widgets/Text/STextBlock.h"

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
        FilterCategory = Prop->GetMetaData(TEXT("Categories"));

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
```

- [ ] **Step 4: 모듈에 등록 / 해제**

`StartupModule()`:
```cpp
PropModule.RegisterCustomPropertyTypeLayout(
    TEXT("GameplayTag"),
    FOnGetPropertyTypeCustomizationInstance::CreateStatic(
        &FSpyGameplayTagCustomization::MakeInstance));
```

`ShutdownModule()`:
```cpp
PropModule.UnregisterCustomPropertyTypeLayout(TEXT("GameplayTag"));
```

- [ ] **Step 5: 빌드 확인**

Ability 탭에서 `ClassType` 필드에 "범위: Spy.Class" 라벨 확인.

- [ ] **Step 6: 커밋**

```bash
git add SkillProject/Source/SpyDataEditorTool/ SkillProject/Source/SkillProject/Data/SpyCharacterAssetData.h
git commit -m "[SpyDataEditorTool] SpyGameplayTagCustomization 태그 필터 구현"
```

---

## Task 8: SpyArrayCopyCustomization (Copy 버튼 — Ability 탭 전 에셋)

Ability 탭의 세 에셋(USpyCharacterAssetData, USpyAbilityData, USpyComboAssetData) 내 **모든 TArray 프로퍼티**에 Copy 버튼을 추가한다. 동일 커스터마이징 클래스를 각 에셋 클래스에 등록해 재사용한다.

**Files:**
- Create: `Public/Customizations/SpyArrayCopyCustomization.h`
- Create: `Private/Customizations/SpyArrayCopyCustomization.cpp`
- Modify: `Private/SpyDataEditorTool.cpp` (등록)

- [ ] **Step 1: SpyArrayCopyCustomization 헤더**

```cpp
// Public/Customizations/SpyArrayCopyCustomization.h
#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

/**
 * Ability 탭 에셋의 모든 TArray 프로퍼티 엘리먼트 옆에 Copy 버튼을 추가한다.
 * USpyCharacterAssetData / USpyAbilityData / USpyComboAssetData 세 클래스에 공통 등록.
 */
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
```

- [ ] **Step 2: SpyArrayCopyCustomization 구현**

```cpp
// Private/Customizations/SpyArrayCopyCustomization.cpp
#include "Customizations/SpyArrayCopyCustomization.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FSpyArrayCopyCustomization"

TSharedRef<IDetailCustomization> FSpyArrayCopyCustomization::MakeInstance()
{
    return MakeShareable(new FSpyArrayCopyCustomization());
}

void FSpyArrayCopyCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    // 오브젝트의 모든 프로퍼티를 순회해 TArray인 것에 Copy 버튼 삽입
    TArray<TWeakObjectPtr<UObject>> Objects;
    DetailBuilder.GetObjectsBeingCustomized(Objects);
    if (Objects.Num() == 0 || !Objects[0].IsValid()) return;

    UClass* Class = Objects[0]->GetClass();

    for (TFieldIterator<FArrayProperty> It(Class); It; ++It)
    {
        FArrayProperty* ArrayProp = *It;
        TSharedRef<IPropertyHandle> ArrayHandle =
            DetailBuilder.GetProperty(ArrayProp->GetFName(), Class);

        if (!ArrayHandle->IsValidHandle()) continue;

        FString CategoryName;
        ArrayProp->GetMetaData(TEXT("Category")).Split(TEXT("|"), &CategoryName, nullptr);
        if (CategoryName.IsEmpty()) CategoryName = ArrayProp->GetMetaData(TEXT("Category"));
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
            ElementHandle->ExportText(ExportedValue, PPF_None);

            TSharedPtr<IPropertyHandleArray> ArrayHandleArray = ArrayHandle->AsArray();
            if (!ArrayHandleArray.IsValid()) return FReply::Handled();

            ArrayHandleArray->AddItem();

            uint32 NumElements = 0;
            ArrayHandleArray->GetNumElements(NumElements);
            if (NumElements > 0)
            {
                TSharedRef<IPropertyHandle> NewElement =
                    ArrayHandleArray->GetElement(NumElements - 1);
                NewElement->ImportText(*ExportedValue, PPF_None);
            }

            return FReply::Handled();
        })
    ];
}

#undef LOCTEXT_NAMESPACE
```

- [ ] **Step 3: 모듈에 등록 / 해제 (세 에셋 클래스 공통)**

`StartupModule()`:
```cpp
#include "Customizations/SpyArrayCopyCustomization.h"
#include "Data/SpyCharacterAssetData.h"
#include "Data/SpyAbilityData.h"
#include "Data/SpyComboAssetData.h"

const auto CopyFactory = FOnGetDetailCustomizationInstance::CreateStatic(
    &FSpyArrayCopyCustomization::MakeInstance);

PropModule.RegisterCustomClassLayout(USpyCharacterAssetData::StaticClass()->GetFName(), CopyFactory);
PropModule.RegisterCustomClassLayout(USpyAbilityData::StaticClass()->GetFName(),        CopyFactory);
PropModule.RegisterCustomClassLayout(USpyComboAssetData::StaticClass()->GetFName(),     CopyFactory);
```

`ShutdownModule()`:
```cpp
PropModule.UnregisterCustomClassLayout(USpyCharacterAssetData::StaticClass()->GetFName());
PropModule.UnregisterCustomClassLayout(USpyAbilityData::StaticClass()->GetFName());
PropModule.UnregisterCustomClassLayout(USpyComboAssetData::StaticClass()->GetFName());
```

- [ ] **Step 4: 빌드 확인**

Ability 탭의 SpyCharacterAssetData, SpyCommonCharacterAbility, SpyNormalAbility, SpyNormalComboAssetData 내 모든 배열 프로퍼티 엘리먼트마다 "Copy Entry" 버튼 확인.

- [ ] **Step 5: 커밋**

```bash
git add SkillProject/Source/SpyDataEditorTool/
git commit -m "[SpyDataEditorTool] SpyArrayCopyCustomization — Ability 탭 전 에셋 배열 Copy 버튼 구현"
```

---

## Task 9: 프로퍼티 필터 + 카테고리 자동 펼치기 (Polish)

**Files:**
- Modify: `Private/Tabs/SSpyAssetsTab.cpp`
- Modify: `Private/Tabs/SSpyAbilityTab.cpp`
- Modify: `Private/Tabs/SSpyConfigTab.cpp`

- [ ] **Step 1: 프로퍼티 필터 설정 (공통)**

각 탭 `Construct`에서 DetailsView 생성 직후:

```cpp
AssetDataView->SetIsPropertyVisibleDelegate(
    FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& PropertyAndParent) -> bool
    {
        const FProperty& Prop = PropertyAndParent.Property;
        return !Prop.GetName().EndsWith(TEXT("Cache")) &&
               !Prop.GetName().EndsWith(TEXT("ToPath"));
    }));
```

- [ ] **Step 2: 카테고리 자동 펼치기**

각 탭 `Construct` 마지막에 추가:

```cpp
GEditor->GetTimerManager()->SetTimerForNextTick([this]()
{
    AssetDataView->ForceRefreshDetails();
    AnimAssetDataView->ForceRefreshDetails();
});
```

Config 탭:
```cpp
GEditor->GetTimerManager()->SetTimerForNextTick([this]()
{
    AIConfigView->ForceRefreshDetails();
    CharacterConfigView->ForceRefreshDetails();
    InputConfigView->ForceRefreshDetails();
    MovementConfigView->ForceRefreshDetails();
});
```

- [ ] **Step 3: 빌드 최종 확인**

전체 흐름 확인:
1. `Window > Spy Data Editor` 탭 오픈
2. **Assets 탭** (Spy/Data/): Scan → AnimLayerMap 채워짐 → Apply → Diff 다이얼로그 → 저장
3. **Ability 탭** (Spy/Data/): 모든 에셋 배열 프로퍼티에 Copy 버튼 동작 → Apply → 저장
4. **Config 탭** (Spy/Data/Config/): Scan → DetailsView 갱신 → 4개 에셋 편집 → Apply → 저장
5. 에셋 피커에서 Content/Spy/ 외부 경로 숨김 확인
6. GameplayTag 필드에 "범위: Spy.Class" 라벨 확인

- [ ] **Step 4: 최종 커밋**

```bash
git add SkillProject/Source/SpyDataEditorTool/
git commit -m "[SpyDataEditorTool] 프로퍼티 필터 + 카테고리 자동 펼치기 폴리시 완료"
```
