# Spy GA Creator Tool — Design Spec
Date: 2026-04-30

## 개요

Unreal Editor Window 메뉴에 "Spy GA Creator" 항목을 추가한다. 클릭 시 NomadTab이 열리고, 부모 클래스 선택·이름 입력·GAS 기본 설정을 커스텀 Slate 폼으로 입력한 뒤 "Create GA" 버튼 한 번으로 `/Game/Spy/Blueprints/GameplayAbilities/GA_<Name>` 경로에 Blueprint 에셋을 생성하고 Blueprint 에디터를 자동으로 연다.

기존 `SpyDataEditorTool` 모듈과 코드·의존성이 완전히 분리된 별도 C++ 에디터 모듈로 구현한다.

---

## 1. 모듈 구조

### 신규 모듈: `SpyGACreatorTool` (Type: Editor)

```
SkillProject/Source/SpyGACreatorTool/
├── SpyGACreatorTool.Build.cs
├── Public/
│   ├── SpyGACreatorTool.h          (IModuleInterface 구현)
│   └── SSpyCreateGADialog.h        (탭 콘텐츠 SCompoundWidget)
└── Private/
    ├── SpyGACreatorTool.cpp
    └── SSpyCreateGADialog.cpp
```

### Build.cs 의존 모듈

```
"Core", "CoreUObject", "Engine",
"Slate", "SlateCore", "EditorStyle",
"UnrealEd", "Kismet",
"GameplayAbilities", "GameplayTags", "GameplayTagsEditor",
"SkillProject", "SKGAS"
```

### 등록 파일 수정

| 파일 | 변경 |
|------|------|
| `SkillProject.uproject` | `SpyGACreatorTool` 모듈 추가 (Type: Editor, LoadingPhase: Default) |

---

## 2. 모듈 등록 (SpyGACreatorTool.cpp)

- `FGlobalTabmanager`에 NomadTab Spawner 등록 (`TabName = "SpyGACreatorTool"`)
- `UToolMenus`로 `LevelEditor.MainMenu.Window` 에 "Spy GA Creator" 메뉴 항목 추가
- `ShutdownModule`에서 Spawner·메뉴 해제

---

## 3. UI 레이아웃 (SSpyCreateGADialog)

`SCompoundWidget`을 상속하며 `SScrollBox` 안에 아래 섹션을 세로로 배치한다.

### 3-1. 헤더 영역

| 필드 | 위젯 | 비고 |
|------|------|------|
| Parent Class | `SComboBox<UClass*>` | `GetDerivedClasses(UGameplayAbility::StaticClass())` 로 스캔, Abstract 제외 |
| GA Name | `SEditableTextBox` | 영숫자+언더스코어만 허용 |
| Output Path | `STextBlock` (읽기 전용) | `/Game/Spy/Blueprints/GameplayAbilities/GA_<Name>` 실시간 표시 |

### 3-2. Tags 섹션

| 필드 | 위젯 |
|------|------|
| Ability Tags | `SGameplayTagContainerCombo` |
| Activation Owned Tags | `SGameplayTagContainerCombo` |
| Activation Required Tags | `SGameplayTagContainerCombo` |
| Activation Blocked Tags | `SGameplayTagContainerCombo` |
| Cancel Abilities With Tag | `SGameplayTagContainerCombo` |
| Block Abilities With Tag | `SGameplayTagContainerCombo` |

### 3-3. Policies 섹션

| 필드 | 위젯 | 기본값 |
|------|------|--------|
| Net Execution Policy | `SComboBox<TSharedPtr<FString>>` | `LocalPredicted` |
| Instancing Policy | `SComboBox<TSharedPtr<FString>>` | `InstancedPerActor` |
| Net Security Policy | `SComboBox<TSharedPtr<FString>>` | `ClientOrServer` |

### 3-4. Costs 섹션

| 필드 | 위젯 |
|------|------|
| Cost GE Class | `SClassPropertyEntryBox` (UGameplayEffect 서브클래스 필터) |

### 3-5. Cooldowns 섹션

| 필드 | 위젯 |
|------|------|
| Cooldown GE Class | `SClassPropertyEntryBox` (UGameplayEffect 서브클래스 필터) |

### 3-6. 하단

`SButton` "Create GA" — 유효성 검사 통과 시 생성 플로우 실행.

> Triggers(`TArray<FAbilityTriggerData>`) 섹션은 이번 범위에서 제외.

---

## 4. Blueprint 생성 플로우

```
[Create GA 클릭]
  │
  ├─ 유효성 검사
  │   ├─ GA Name 비어있음 → 오류 다이얼로그
  │   ├─ 이미 동일 경로에 에셋 존재 → 오류 다이얼로그
  │   └─ Parent Class 미선택 → 오류 다이얼로그
  │
  ├─ UPackage 생성
  │   └─ /Game/Spy/Blueprints/GameplayAbilities/GA_<Name>
  │
  ├─ FKismetEditorUtilities::CreateBlueprint(
  │       SelectedClass, Package, FName("GA_<Name>"), BPTYPE_Normal)
  │
  ├─ FKismetEditorUtilities::CompileBlueprint(NewBP)  ← GeneratedClass 준비
  │
  ├─ CDO에 설정값 반영
  │   ├─ CDO = NewBP->GeneratedClass->GetDefaultObject<UGameplayAbility>()
  │   ├─ Tag 컨테이너 6개
  │   ├─ NetExecutionPolicy / InstancingPolicy / NetSecurityPolicy
  │   ├─ CostGameplayEffectClass
  │   └─ CooldownGameplayEffectClass
  │
  ├─ FKismetEditorUtilities::CompileBlueprint(NewBP)  ← CDO 변경 반영
  │
  ├─ UPackage::SavePackage(...)
  │
  └─ UAssetEditorSubsystem::OpenEditorForAsset(NewBP)
```

---

## 5. 유효성 규칙

- GA Name: 비어있으면 생성 불가. 영숫자 + 언더스코어만 허용.
- 동일 경로에 같은 이름의 에셋이 이미 존재하면 생성 불가.
- Parent Class가 선택되지 않으면 생성 불가.

---

## 6. 범위 밖 (이번 구현에서 제외)

- Triggers 섹션 (`TArray<FAbilityTriggerData>`)
- SpyGameplayTags.h/.cpp 자동 등록
- USpyAbilityData DataAsset 자동 등록
- Input 바인딩 자동 설정
