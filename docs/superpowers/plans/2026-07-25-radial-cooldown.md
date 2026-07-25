# 레이디얼(360°) 쿨다운 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 스킬 슬롯 쿨다운 표시를 세로 바에서 원형(360°) 레이디얼 언와인드로 바꾼다 — 아이콘 위에 어두운 오버레이가 시계방향으로 풀린다.

**Architecture:** 레이디얼 마스크 머티리얼(`M_RadialCooldown`, `Icon`/`Percent` 파라미터)을 슬롯의 `Img_Cooldown`(UImage)에 얹고, `USpySkillSlotWidget` 이 MID 를 통해 tick 마다 `Percent = CooldownNormalized` 를 세팅한다. 기존 세로 `PB_Cooldown` 을 대체한다.

**Tech Stack:** Unreal Engine 5.7, C++, UMG, Material(UI 도메인 Translucent), MaterialInstanceDynamic.

## Global Constraints

- 주석 `//#`, `!` 금지(`== nullptr`/`== false`), `TObjectPtr`, include self→UE→project. (cpp-style.md)
- 하드코딩 `/Game/...` 경로 금지 — 머티리얼은 `WBP_SkillSlot` 의 `Img_Cooldown` 브러시로 참조(코드는 `GetDynamicMaterial()`). (plugin-skassetcore.md)
- **빌드/컴파일/테스트는 사용자** — CLI 없음. **WidgetBlueprint `compile_blueprint()` 금지(데드락)** — 사용자 디자이너 컴파일. (ui-workflow.md)
- **`git commit` 자동 금지** — `git add` 까지만.
- 방향 = **B 언와인드**(발동=Percent 1=전체 어둠 → 준비=0). `CooldownNormalized`(1=방금발동, 0=준비) 를 `Percent` 로 직결.
- 하위호환: 쿨다운 없는 어빌리티는 `bOnCooldown=false` → 오버레이 숨김(현행 유지).

---

### Task 1: 슬롯 위젯 — 레이디얼 MID 구동 (`USpySkillSlotWidget`)

세로 `PB_Cooldown` 대신 `Img_Cooldown`(UImage) 의 머티리얼 MID 로 쿨다운을 표시한다.

**Files:**
- Modify: `SkillProject/Source/SkillProject/UI/SpySkillSlotWidget.h`
- Modify: `SkillProject/Source/SkillProject/UI/SpySkillSlotWidget.cpp`

**Interfaces:**
- Consumes: `SpyHUDMath::CooldownNormalized`(기존), `M_RadialCooldown` 머티리얼(Task 2, WBP 브러시로 주입).
- Produces: `Img_Cooldown`(BindWidgetOptional UImage) + `CooldownMID` 로 `Percent`/`Icon` 파라미터 구동.

**Notes:**
- `Img_Cooldown->GetDynamicMaterial()` 은 브러시 머티리얼로 MID 를 자동 생성/반환한다 — 머티리얼 경로 하드코딩 불필요.
- 파라미터 이름은 머티리얼과 정확히 일치: `Percent`(Scalar), `Icon`(Texture).

- [ ] **Step 1: 헤더 교체** (`.h`) — `PB_Cooldown`(UProgressBar) 멤버 제거, 아래 추가. `class UMaterialInstanceDynamic;` 전방선언.

```cpp
//# 쿨다운 레이디얼 오버레이(M_RadialCooldown 머티리얼). 세로바 대체
UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
TObjectPtr<UImage> Img_Cooldown;

//# Img_Cooldown 브러시 머티리얼의 동적 인스턴스 — Percent/Icon 세팅용
UPROPERTY()
TObjectPtr<UMaterialInstanceDynamic> CooldownMID;
```
- `#include "Components/ProgressBar.h"` 가 더 안 쓰이면 제거, `UMaterialInstanceDynamic` 는 `Materials/MaterialInstanceDynamic.h`.

- [ ] **Step 2: MID 생성 + 아이콘 파라미터** (`.cpp` `Setup`) — 아이콘 세팅 인근에 추가.

```cpp
//# 쿨다운 레이디얼 MID 준비(브러시 머티리얼 기반). 아이콘 있으면 오버레이도 그 텍스처를 어둡게 쓴다
if (Img_Cooldown != nullptr)
{
    CooldownMID = Img_Cooldown->GetDynamicMaterial();
    if (CooldownMID != nullptr && InIcon != nullptr)
    {
        CooldownMID->SetTextureParameterValue(TEXT("Icon"), InIcon);
    }
}
```

- [ ] **Step 3: NativeTick 구동** (`.cpp`) — 기존 `PB_Cooldown->SetPercent/SetVisibility` 블록을 아래로 교체.

```cpp
//# 레이디얼 언와인드: Percent=CooldownNormalized(1=방금발동→0=준비). 머티리얼이 각도 마스크 처리
if (Img_Cooldown != nullptr)
{
    Img_Cooldown->SetVisibility(bOnCooldown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    if (CooldownMID != nullptr)
    {
        CooldownMID->SetScalarParameterValue(TEXT("Percent"), Normalized);
    }
}
```
- `Txt_Cooldown` 잔여 초·`Img_Icon` 마나부족 틴트 로직은 **변경 없이 유지**.

- [ ] **Step 4: 빌드 + 수동 검증(사용자)** — Task 2·3 후 PIE 에서 스킬 발동 시 원형 어둠이 시계방향으로 풀리는지 확인.
- [ ] **Step 5: 스테이징** — `git add`.

---

### Task 2: `M_RadialCooldown` 머티리얼 (메인 unreal-mcp / 사용자 폴백)

**에셋 태스크** — 코드 변경 0건.

**대상:** `/Game/Spy/UI/M_RadialCooldown` (UI 도메인, Translucent).

**노드 스펙:**
- 파라미터: `Icon`(TextureSampleParameter2D, 기본 흰 텍스처), `Percent`(ScalarParameter, 0~1).
- 각도 마스크: `TexCoord`−0.5 → `atan2(y, x)` → `/(2π)`+0.5 → 12시 기준·시계방향 되도록 회전(예: `frac(0.75 - angleNorm)` 형태로 조정) → 결과 `A`(0~1).
- 마스크 = `A < Percent ? 1 : 0` — `if`(A, Percent) 또는 `step`/`smoothstep`(부드러운 경계).
- Emissive = `Icon.rgb × 0.35`(어둡게), Opacity = `마스크 × 0.7`.
- Blend Mode = Translucent, Shading Model = Unlit, Used with UI.

- [ ] **Step 1: 머티리얼 생성 + 노드 구성** — MCP `unreal.MaterialEditingLibrary.create_material_expression`/`connect_material_property` 로 위 그래프 구성 시도. **복잡/실패 시 사용자에게 위 스펙 전달 → 머티리얼 에디터에서 구성**(폴백, 표준 레이디얼 머티리얼).
- [ ] **Step 2: 저장** — `save_asset(only_if_is_dirty=False)`. 12시 기준·시계방향·언와인드 방향 확인(Percent 1→전체, 0→없음).
- [ ] **Step 3: 스테이징** — `.uasset` `git add`.

---

### Task 3: `WBP_SkillSlot` 배선 (메인 unreal-mcp + 사용자)

**에셋 태스크** — 코드 변경 0건.

**대상:** `WBP_SkillSlot` — `PB_Cooldown`(세로 ProgressBar) 제거, `Img_Cooldown`(UImage) 추가.

- `Img_Cooldown`: 슬롯 전체 채움(anchors 0–1, offsets 0), 브러시 머티리얼 = `M_RadialCooldown`.
- z-order: `Img_Icon`(원본, 아래) → `Img_Cooldown`(오버레이) → `Txt_KeyHint`/`Txt_Cooldown`/`Txt_Cost`(위).

**Notes:**
- MCP: `PB_Cooldown` 제거(`remove_child`), `Img_Cooldown` 을 `new_object(unreal.Image, wt, "Img_Cooldown")` + `add_child_to_canvas` + 브러시 머티리얼 세팅(`SetBrushFromMaterial` 또는 브러시 오브젝트). WidgetBlueprint **컴파일 금지**(사용자).
- 브러시 이미지 크기: 슬롯 채움. `Img_Icon` 위에 겹치도록 z-order.

- [ ] **Step 1: PB_Cooldown 제거 + Img_Cooldown(머티리얼) 추가**(메인 MCP).
- [ ] **Step 2: 사용자** — 디자이너에서 WBP_SkillSlot 컴파일 + z-order 확인 + PIE 검증(원형 언와인드).
- [ ] **Step 3: 스테이징** — 변경 `.uasset` `git add`.

---

## Self-Review

**Spec coverage:**
- §2 시각(원본 배경 + 어두운 레이디얼 오버레이 + 남은 초) → Task 1(오버레이 구동)+Task 3(레이어). ✓
- §3-1 머티리얼(Icon/Percent, atan2 마스크, 0.35 어둠) → Task 2. ✓
- §3-2 슬롯 위젯(Img_Cooldown MID, tick Percent) → Task 1. ✓
- §3-3 WBP(PB_Cooldown→Img_Cooldown, z-order) → Task 3. ✓
- §4 순수함수(CooldownNormalized 재사용, 신규 없음) → 태스크 없음(의도). ✓
- §5 에셋(머티리얼·WBP) → Task 2·3. ✓
- §6 범위밖(방향A·플래시·아이콘없음 폴백) → Task 2 노드가 Icon 미세팅 시 어두운 원형. ✓
- §7 하위호환(쿨다운 없으면 숨김) → Task 1 Step 3 `bOnCooldown` 게이트. ✓

**Placeholder scan:** 머티리얼 노드는 스펙+MCP 시도+사용자 폴백으로 명시(의도적). 엔진 API 골격 제공.

**Type consistency:** `Percent`(Scalar)·`Icon`(Texture) 파라미터명 Task1(SetScalar/SetTexture)=Task2(파라미터) 일치. `Img_Cooldown`(UImage) Task1 멤버=Task3 위젯명 일치. `GetDynamicMaterial()→UMaterialInstanceDynamic*`. ✓
