# 레이디얼(360°) 쿨다운 — 설계

- 날짜: 2026-07-25
- 관련: `USpySkillSlotWidget`(쿨다운 표시), `WBP_SkillSlot`
- 목업: cooldown-mockup 아티팩트 — **방향 B(언와인드) 승인**
- 상태: 목업 승인(ui-workflow §1) → writing-plans 진입 대기

---

## 1. 목표

스킬 슬롯 쿨다운 표시를 현재 **세로 바(PB_Cooldown)** 에서 **원형(360°) 레이디얼 스윕**으로 바꾼다. 아이콘 원본은 배경에 그대로, 그 위에 **어둡게(흐릿한) 오버레이**가 시계방향 360° 마스크로 표시된다.

**방향(사용자 승인) = B 언와인드**: 발동 순간 오버레이가 **전체 원(360°)** 을 덮고, 준비될수록 시계방향으로 **풀리며(360°→0°)** 아이콘이 드러난다. (MOBA/RPG 표준.)

## 2. 시각 정의

- **배경 레이어**: 기존 `Img_Icon`(원본 아이콘, config 텍스처 또는 더미색) — 변경 없음.
- **쿨다운 오버레이**: 원본 위에 겹치는 이미지. 아이콘을 **어둡게(≈0.35 곱)** 한 색을, **레이디얼 각도 마스크**로 표시. 마스크 영역 = "아직 쿨다운 중"인 부분.
- **남은 초 텍스트**(`Txt_Cooldown`, 중앙, 소수 1자리): 유지.
- 쿨다운 아님 → 오버레이 숨김(Percent=0), 남은 초 숨김.

## 3. 기술 구조

### 3-1. 레이디얼 마스크 머티리얼 (`M_RadialCooldown`)
- 도메인: **User Interface**, 블렌드: **Translucent**.
- 파라미터:
  - `Icon` (Texture 파라미터) — 오버레이가 어둡게 표시할 아이콘.
  - `Percent` (Scalar 파라미터, 0~1) — 남은 쿨다운 비율(1=방금발동, 0=준비).
- 노드 흐름:
  1. `TexCoord` → 중심 이동 `uv - 0.5`.
  2. 각도 = `atan2(uv.y, uv.x)` → 0~1 정규화(`+PI`, `/2PI`), **12시 기준·시계방향** 되도록 회전/부호 조정.
  3. 마스크 = `angleNorm < Percent ? 1 : 0` (또는 부드러운 경계 `smoothstep`).
  4. Emissive = `Icon.rgb * 0.35`(어둡게), Opacity = `마스크 × 0.7`(반투명 어둠).
- 파생: `MID`(Material Instance Dynamic)로 런타임에 `Icon`/`Percent` 세팅.

### 3-2. 슬롯 위젯 (`USpySkillSlotWidget`)
- 쿨다운 오버레이용 **`Img_Cooldown`(UImage)** — 기존 세로 `PB_Cooldown`(ProgressBar) 을 **이 Image 로 대체**.
- `Setup`/`NativeConstruct` 에서 `Img_Cooldown` 브러시 머티리얼 → `CreateDynamicMaterialInstance` 로 MID 생성, `Icon` 파라미터에 슬롯 아이콘 텍스처 세팅(있으면).
- `NativeTick`:
  - `Normalized = CooldownNormalized(Remaining, Duration)`.
  - `bOnCooldown` 이면 `Img_Cooldown` 표시 + `MID->SetScalarParameterValue("Percent", Normalized)`. 아니면 숨김(또는 Percent=0).
  - `Txt_Cooldown` 잔여 초는 현행 유지.
- 마나부족 적색 틴트(`Img_Icon` 대상)는 **원본 아이콘 레이어**에 그대로 — 쿨다운 오버레이와 독립.

### 3-3. 위젯 배선 (`WBP_SkillSlot`)
- 기존 `PB_Cooldown`(세로 ProgressBar) 제거 → `Img_Cooldown`(Image, 슬롯 전체 채움, `M_RadialCooldown` 머티리얼) 배치.
- z-order: `Img_Icon`(원본) 아래, `Img_Cooldown`(오버레이) 위, `Txt_KeyHint`/`Txt_Cooldown`/`Txt_Cost` 최상단.

## 4. 순수함수 + 테스트

- `CooldownNormalized`(기존, 재사용) — 이미 테스트됨. 레이디얼은 이 값을 `Percent` 로 넘길 뿐이라 신규 순수함수 없음.
- 머티리얼·MID·tick 은 렌더/월드 의존이라 단위테스트 대상 아님(수동 PIE 검증).

## 5. 에셋

- `M_RadialCooldown` 머티리얼 생성 — **MCP `MaterialEditingLibrary` 시도**(노드 구성). 복잡/실패 시 **사용자가 머티리얼 에디터에서** 위 노드 스펙대로 구성(폴백).
- `WBP_SkillSlot`: `PB_Cooldown` → `Img_Cooldown`(머티리얼) 교체.

## 6. 범위 밖 (YAGNI)

- 방향 A(채워짐), 파티클/글로우 연출.
- 쿨다운 완료 플래시·사운드.
- 아이콘 없을 때(더미색)의 오버레이 — 아이콘 텍스처 없으면 어두운 단색 오버레이로 폴백(Icon 파라미터 미세팅 시 검정 곱 → 어두운 원형).

## 7. 하위호환 / 룰

- 쿨다운 없는 어빌리티(볼트 등)는 Percent 조회 시 쿨다운 GE 없음 → `bOnCooldown=false` → 오버레이 숨김(현행과 동일).
- UI 변경이므로 ui-workflow §1 목업 우선 준수(완료). 위젯 편집은 MCP(new_object/브러시) — WidgetBlueprint 컴파일 금지(사용자).
- `//#`·`!` 금지·`TObjectPtr` 등 cpp-style 유지.
