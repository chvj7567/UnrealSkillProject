# 스킬바 슬롯 config (DataAsset) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 스킬바 슬롯의 스킬(입력태그)·아이콘을 코드 수정 없이 `USpySkillBarConfig` DataAsset(Details 패널)으로 설정한다.

**Architecture:** 신규 `USpySkillBarConfig`(`TArray<FSpySkillBarSlot{InputTag, Icon}>`)를 `USpySkillBarWidget` 이 참조(EditDefaultsOnly + CDO 폴백). 순수함수 `ExtractSlotInputTags(config)` 가 슬롯 태그 목록을 해석하고(config 없으면 기존 하드코딩 폴백), `BuildSlots` 가 슬롯별 아이콘을 `SpySkillSlotWidget::Setup` 에 넘겨 `Img_Icon` 에 적용한다.

**Tech Stack:** Unreal Engine 5.7, C++, UMG, GameplayTags, Unreal Automation(`WITH_DEV_AUTOMATION_TESTS`).

## Global Constraints

- 주석 `//#`, `!` 금지(`== nullptr`/`== false`), `TObjectPtr`, include 순서 self→UE→project. (cpp-style.md)
- 하드코딩 `/Game/...` 경로 리터럴 금지 — 아이콘은 `TSoftObjectPtr`, config 는 WBP 기본값 참조. (plugin-skassetcore.md)
- 위젯은 `USpyUserWidget` 계열, 태그는 리터럴 금지. (기존)
- **빌드/테스트/컴파일은 사용자가 에디터·VS 에서** — CLI 없음. "실행" 스텝은 사용자 수동 검증 지점.
- **`git commit` 자동 금지** — `git add` 까지만, 커밋 메시지(안)은 마무리에서. (git-conventions.md)
- 테스트 등록명 `"SkillProject.HUD.SkillBar.<케이스>"`, 파일 전체 `#if WITH_DEV_AUTOMATION_TESTS`. 기존 `SpySkillBarTests.cpp` 스타일 준수.
- **하위호환 필수**: `SkillBarConfig` 미지정 시 현행 하드코딩 Skill_1~6 동작 유지.
- EditDefaultsOnly 임베드 인스턴스 런타임 null → CDO 폴백(2026-07-24 SlotWidgetClass 선례).

---

### Task 1: `USpySkillBarConfig` DataAsset

**Files:**
- Create: `SkillProject/Source/SkillProject/Data/SpySkillBarConfig.h`
- Create: `SkillProject/Source/SkillProject/Data/SpySkillBarConfig.cpp`

**Interfaces:**
- Produces:
  - `struct FSpySkillBarSlot { FGameplayTag InputTag; TSoftObjectPtr<UTexture2D> Icon; }`
  - `class USpySkillBarConfig : public UDataAsset { TArray<FSpySkillBarSlot> Slots; }`

- [ ] **Step 1: 헤더 작성** (`SpySkillBarConfig.h`)

```cpp
// Fill out your copyright notice in the Description page of Project Settings.
#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SpySkillBarConfig.generated.h"

class UTexture2D;

//# 스킬바 슬롯 1개 정의 — 배열 순서가 곧 화면 좌→우 슬롯 순서
USTRUCT(BlueprintType)
struct FSpySkillBarSlot
{
    GENERATED_BODY()

    //# 이 슬롯이 표시·활성·쿨다운 조회에 쓰는 입력태그
    UPROPERTY(EditAnywhere, meta = (Categories = "Input.Ability"))
    FGameplayTag InputTag;

    //# 슬롯 아이콘. 소프트 참조 — 슬롯 빌드 시 로드
    UPROPERTY(EditAnywhere)
    TSoftObjectPtr<UTexture2D> Icon;
};

//# 스킬바 슬롯 구성 DataAsset. 에디터 Details 패널에서 편집한다
UCLASS()
class SKILLPROJECT_API USpySkillBarConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    //# 배열 순서 = 슬롯 순서
    UPROPERTY(EditDefaultsOnly, Category = "SkillBar")
    TArray<FSpySkillBarSlot> Slots;
};
```

- [ ] **Step 2: cpp 작성** (`SpySkillBarConfig.cpp`)

```cpp
#include "Data/SpySkillBarConfig.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(SpySkillBarConfig)
```

- [ ] **Step 3: 빌드(사용자)** — 컴파일 후 에디터 Content 우클릭 > Miscellaneous > Data Asset 목록에 `SpySkillBarConfig` 노출 확인.
- [ ] **Step 4: 스테이징** — `git add`.

---

### Task 2: `ExtractSlotInputTags` 순수함수 + 테스트

config 로 슬롯 태그 목록을 해석하고, 없으면 기존 하드코딩으로 폴백한다.

**Files:**
- Modify: `SkillProject/Source/SkillProject/UI/SpySkillBarWidget.h`
- Modify: `SkillProject/Source/SkillProject/UI/SpySkillBarWidget.cpp`
- Test: `SkillProject/Source/SkillProject/UI/Tests/SpySkillBarTests.cpp`

**Interfaces:**
- Consumes: `USpySkillBarConfig::Slots` (Task 1), 기존 `USpySkillBarWidget::BuildSlotInputTags()`(하드코딩 6태그).
- Produces:
  - `static TArray<FGameplayTag> USpySkillBarWidget::ExtractSlotInputTags(const USpySkillBarConfig* Config);`
  - 계약: `Config` 유효 && `Slots` 비어있지 않음 → `Slots[i].InputTag` 순서대로. 아니면 `BuildSlotInputTags()`.

- [ ] **Step 1: 실패 테스트 작성** (`SpySkillBarTests.cpp` 에 추가)

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpySkillBarConfigExtractTest, "SkillProject.HUD.SkillBar.ConfigExtract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSpySkillBarConfigExtractTest::RunTest(const FString& Parameters)
{
    //# (a) null → 기본 6태그
    TestEqual(TEXT("null -> default 6"), USpySkillBarWidget::ExtractSlotInputTags(nullptr).Num(), 6);

    //# (b) 빈 config → 기본 6태그
    USpySkillBarConfig* Empty = NewObject<USpySkillBarConfig>();
    TestEqual(TEXT("empty -> default 6"), USpySkillBarWidget::ExtractSlotInputTags(Empty).Num(), 6);

    //# (c) 3슬롯 config → 그 3태그 순서대로
    USpySkillBarConfig* Cfg = NewObject<USpySkillBarConfig>();
    FSpySkillBarSlot S1; S1.InputTag = SpyGameplayTags::Input_Ability_Skill_3;
    FSpySkillBarSlot S2; S2.InputTag = SpyGameplayTags::Input_Ability_Skill_1;
    FSpySkillBarSlot S3; S3.InputTag = SpyGameplayTags::Input_Ability_Skill_2;
    Cfg->Slots = { S1, S2, S3 };
    const TArray<FGameplayTag> Tags = USpySkillBarWidget::ExtractSlotInputTags(Cfg);
    TestEqual(TEXT("3 slots"), Tags.Num(), 3);
    TestEqual(TEXT("order[0]"), Tags[0], SpyGameplayTags::Input_Ability_Skill_3);
    TestEqual(TEXT("order[1]"), Tags[1], SpyGameplayTags::Input_Ability_Skill_1);
    TestEqual(TEXT("order[2]"), Tags[2], SpyGameplayTags::Input_Ability_Skill_2);
    return true;
}
```

- [ ] **Step 2: 컴파일 실패 확인** — `ExtractSlotInputTags` 미정의. (`#include "Data/SpySkillBarConfig.h"` 를 테스트에 추가)

- [ ] **Step 3: 선언 + 구현**

```cpp
//# SpySkillBarWidget.h — public, BuildSlotInputTags 인근
static TArray<FGameplayTag> ExtractSlotInputTags(const class USpySkillBarConfig* Config);
```
```cpp
//# SpySkillBarWidget.cpp (#include "Data/SpySkillBarConfig.h")
TArray<FGameplayTag> USpySkillBarWidget::ExtractSlotInputTags(const USpySkillBarConfig* Config)
{
    if (Config != nullptr && Config->Slots.Num() > 0)
    {
        TArray<FGameplayTag> Tags;
        Tags.Reserve(Config->Slots.Num());
        for (const FSpySkillBarSlot& Slot : Config->Slots)
        {
            Tags.Add(Slot.InputTag);
        }
        return Tags;
    }
    return BuildSlotInputTags();
}
```

- [ ] **Step 4: 빌드 + Automation(사용자)** — `SkillProject.HUD.SkillBar.ConfigExtract` PASS. 기존 SlotOrder/ExactOrder/Uniqueness 유지.
- [ ] **Step 5: 스테이징** — `git add`.

---

### Task 3: 스킬바 config 참조 + 슬롯 빌드 연동

`USpySkillBarWidget` 이 config 를 참조(CDO 폴백)하고, 슬롯 목록·아이콘을 config 로 구동한다.

**Files:**
- Modify: `SkillProject/Source/SkillProject/UI/SpySkillBarWidget.h`
- Modify: `SkillProject/Source/SkillProject/UI/SpySkillBarWidget.cpp`

**Interfaces:**
- Consumes: `ExtractSlotInputTags` (Task 2), `USpySkillSlotWidget::Setup(..., UTexture2D* InIcon)` (Task 4).
- Produces:
  - 멤버 `TObjectPtr<USpySkillBarConfig> SkillBarConfig` (EditDefaultsOnly).
  - `BuildSlots` 가 `ResolvedConfig`(인스턴스→CDO 폴백)로 슬롯 목록·아이콘 해석.

**Notes:**
- `ResolvedConfig` 폴백은 `ResolvedSlotClass`(2026-07-24) 와 동일 패턴: `SkillBarConfig` → null 이면 `GetClass()->GetDefaultObject<USpySkillBarWidget>()->SkillBarConfig`.
- 아이콘 해석: 각 InputTag 에 대해 `ResolvedConfig->Slots` 에서 같은 InputTag 항목을 찾아 `Icon.LoadSynchronous()`(소프트, 작음). config 없거나 미매치면 `nullptr`.

- [ ] **Step 1: 멤버 추가** (`SpySkillBarWidget.h`)

```cpp
//# 슬롯 구성 DataAsset. WBP 기본값에서 지정. 미지정 시 하드코딩 Skill_1~6 폴백
UPROPERTY(EditDefaultsOnly, Category = "SkillBar")
TObjectPtr<class USpySkillBarConfig> SkillBarConfig;
```

- [ ] **Step 2: BuildSlots 연동** (`SpySkillBarWidget.cpp`) — 슬롯 목록·아이콘을 config 로.

```cpp
//# BuildSlots 진입부 — config 인스턴스→CDO 폴백 (SlotWidgetClass 폴백과 동일)
const USpySkillBarConfig* ResolvedConfig = SkillBarConfig;
if (ResolvedConfig == nullptr)
{
    if (const USpySkillBarWidget* DefaultBar = GetClass()->GetDefaultObject<USpySkillBarWidget>())
    {
        ResolvedConfig = DefaultBar->SkillBarConfig;
    }
}

//# 슬롯 태그 목록: config 있으면 그것, 없으면 하드코딩
const TArray<FGameplayTag> InputTags = ExtractSlotInputTags(ResolvedConfig);
```
- 기존 `const TArray<FGameplayTag> InputTags = BuildSlotInputTags();` 를 위로 교체. `TryBuildSlots` 의 `bAnyGranted` 판정도 동일하게 `ExtractSlotInputTags(ResolvedConfig)` 를 쓰도록 통일(빈 스킬바 방지 게이트가 config 슬롯 기준으로 동작).
- 각 슬롯 루프에서 아이콘 해석 후 `Setup(..., SlotIcon)` 호출:

```cpp
//# 이 InputTag 에 대응하는 config 아이콘 (없으면 nullptr → 더미색 유지)
UTexture2D* SlotIcon = nullptr;
if (ResolvedConfig != nullptr)
{
    for (const FSpySkillBarSlot& CfgSlot : ResolvedConfig->Slots)
    {
        if (CfgSlot.InputTag == InputTag)
        {
            SlotIcon = CfgSlot.Icon.LoadSynchronous();
            break;
        }
    }
}
SlotWidget->Setup(ASC, InputTag, CooldownTags, SlotKeyHint(InputTag), SlotManaCost, SlotIcon);
```

- [ ] **Step 3: 빌드 + 수동 검증(사용자)** — config 미지정 시 기존 6슬롯 유지(하위호환) / config 지정 시 그 순서·개수 확인(Task 5 에셋 후).
- [ ] **Step 4: 스테이징** — `git add`.

---

### Task 4: 슬롯 아이콘 적용 (`USpySkillSlotWidget::Setup`)

**Files:**
- Modify: `SkillProject/Source/SkillProject/UI/SpySkillSlotWidget.h`
- Modify: `SkillProject/Source/SkillProject/UI/SpySkillSlotWidget.cpp`

**Interfaces:**
- Produces: `void Setup(UAbilitySystemComponent* InASC, FGameplayTag InInputTag, FGameplayTagContainer InCooldownTags, FText InKeyHint, float InManaCost, UTexture2D* InIcon);`

- [ ] **Step 1: 시그니처 변경** (`.h`) — 기존 `Setup(...)` 뒤에 `UTexture2D* InIcon` 인자 추가. `class UTexture2D;` 전방선언 추가.

- [ ] **Step 2: 아이콘 적용** (`.cpp` `Setup`)

```cpp
//# 아이콘이 있으면 브러시로 설정(플레이스홀더 더미색 대체). null 이면 기존 브러시 유지
if (InIcon != nullptr && Img_Icon != nullptr)
{
    Img_Icon->SetBrushFromTexture(InIcon);
}
```
- `#include "Engine/Texture2D.h"` 추가. 마나부족 적색 틴트(`SetColorAndOpacity`)는 브러시 위 곱연산이라 그대로 동작.

- [ ] **Step 3: 빌드 + 수동 검증(사용자)** — config 아이콘 지정 시 슬롯에 아이콘 표시 확인.
- [ ] **Step 4: 스테이징** — `git add`.

---

### Task 5: 에셋 — `DA_SpySkillBarConfig` + WBP 참조 (메인 unreal-mcp + 사용자 수동)

**에셋 한정 태스크** — 코드 변경 0건.

**대상:**
- `DA_SpySkillBarConfig`(신규, class=`USpySkillBarConfig`) — `Slots` 배열에 초기 슬롯(현행 Skill_1~6 순서) + 아이콘 텍스처(있으면 지정, 없으면 비움=더미색 유지).
- `WBP_SkillBar` 기본값(Class Defaults)에 `SkillBarConfig = DA_SpySkillBarConfig` 지정.

**Notes:**
- `Slots` 는 `EditDefaultsOnly` 중첩 struct 배열 → MCP 자동편집은 `export_text`/`import_text` 왕복 필요(ui-workflow.md §2-2). 단순하면 **사용자가 Details 에서 직접 입력**이 가장 안전.
- `SkillBarConfig` 참조는 WBP **Class Defaults** 에 지정(임베드 인스턴스는 코드 CDO 폴백이 처리하나, Class Defaults 지정이 정석).
- 아이콘 아트는 이번 범위 밖 — 텍스처 있으면 지정, 없으면 비워도 동작(더미색).

- [ ] **Step 1: `DA_SpySkillBarConfig` 생성 + Slots 입력**(사용자 Details, 또는 메인 MCP import_text).
- [ ] **Step 2: `WBP_SkillBar` Class Defaults 에 SkillBarConfig 지정**(사용자 Details).
- [ ] **Step 3: 사용자 검증** — PIE 에서 config 순서·아이콘 반영 확인. config 비우면 기존 6슬롯 유지 확인.
- [ ] **Step 4: 스테이징** — 변경 `.uasset` `git add`.

---

## Self-Review

**Spec coverage:**
- §2 데이터 모델(USpySkillBarConfig, FSpySkillBarSlot) → Task 1. ✓
- §3 스킬바 연동(SkillBarConfig 멤버·CDO폴백·ExtractSlotInputTags·아이콘) → Task 2(함수)+Task 3(연동). ✓
- §4 슬롯 아이콘(Setup 인자·SetBrushFromTexture) → Task 4. ✓
- §5 테스트(null/빈/N/순서) → Task 2 테스트. ✓
- §6 에셋(DA + WBP 참조) → Task 5. ✓
- §7 범위밖 → 태스크 없음(의도). ✓
- 하위호환(config 미지정 폴백) → Task 2 계약 + Task 3 ResolvedConfig. ✓

**Placeholder scan:** 아이콘 아트=범위 밖 명시(플레이스홀더 아님). 엔진 API 바디는 골격+선례 지정(의도적).

**Type consistency:** `ExtractSlotInputTags(const USpySkillBarConfig*)→TArray<FGameplayTag>`, `FSpySkillBarSlot{InputTag,Icon}`, `Setup(ASC,InputTag,CooldownTags,KeyHint,ManaCost,Icon)` — Task 1·2·3·4 간 일치. `SkillBarConfig` 멤버명 Task3 정의=Task5 참조 일치. ✓
