# HUD ①③⑤ — 마나바 · 방향 나침반 · 스킬바/쿨다운 (설계)

- 날짜: 2026-07-24
- 파이프라인: `/start-develop` (승인 게이트 버전)
- 참고 목업: MainHUD 아티팩트 ①③⑤ (뷰포트 콜아웃 번호)
- 상태: 브레인스토밍 승인 완료 → writing-plans 진입 대기

---

## 1. 목표

데모의 MainHUD 는 현재 미션명·진행·경험치만 노출한다. 코드에는 이미 존재하지만 화면에 드러나지 않은 시스템(마나·스킬 쿨다운)을 HUD 로 끌어올리고, 방향 감각(나침반)을 더해 "무엇을 하는 중인지"가 한눈에 읽히게 한다.

세 항목의 확정 범위:

| # | 항목 | 확정 범위 |
|---|---|---|
| ① | 마나바 + 스킬 코스트 | HUD 마나 ProgressBar 바인딩 **+ 액션 스킬에 실제 Mana 코스트 부여**(부족 시 발동 차단). 코스트 수치는 game-designer 설계 |
| ③ | 방향 나침반 + 목표 텍스트 | 컨트롤 회전 기반 방위(N/E/S/W) 나침반 + 미션명·진행 텍스트. **거리/웨이포인트/미니맵 없음, 미션 시스템 미변경** |
| ⑤ | 스킬바 + 쿨다운 라디얼 | InputConfig 의 `Input_Ability_Skill_1~6` 입력 태그로 슬롯 **데이터 구동** 생성(6슬롯 — Parry 는 개정으로 제외, 어빌리티는 유지). 슬롯마다 키힌트 + 쿨다운 시각화 + 마나 코스트 숫자 |

## 2. 접근법 결정

**채택: A — 분해된 위젯.** Compass·SkillBar·SkillSlot 을 각각 독립 `USpyUserWidget` 으로 두고 `USpyMainHUD` 가 호스트한다.

- 근거: 각 유닛이 단일 책임 + 독립 테스트 가능. 프로젝트가 이미 C++ 위젯(`USpyMainHUD`·`USpyHPBar`) + 매니저 경유 관례를 따름.
- 기각: B(MainHUD 단일 바인딩 — 비대·테스트難), C(로직 BP 분산 — C++-first 관례·룰 리뷰 이탈).

## 3. 아키텍처

데이터 획득 원칙 — **이벤트 구동 우선, 연속 보간이 필요한 것만 tick**:

### 3-1. ① 마나
- **표시**: `USpyMainHUD` 에 `PB_Mana`(BindWidgetOptional) 추가. 로컬 폰의 ASC/AttributeSet 를 찾아 `USKAttributeSet::OnManaChanged` / `OnMaxManaChanged` 구독. 기존 경험치 바인딩(bind-retry 타이머)과 동일 패턴 재사용.
- **소비(코스트)**: GAS 표준 코스트 경로.
  - 신규 태그 `Data.Cost.Mana`(SetByCaller) — `SpyGameplayTags` 에 등록.
  - 신규 GE `GE_Cost_Mana` — SetByCaller `Data.Cost.Mana` 매그니튜드로 Mana 를 감산.
  - 액션 스킬 베이스(`USKGameplayAbility_SkillAction` 또는 게임 서브클래스)에 `float ManaCost` 프로퍼티 추가.
  - `ApplyCost`/`CheckCost`(이미 오버라이드 존재)에서 `ManaCost` 를 SetByCaller 로 주입 → 마나 부족 시 `CheckCost` 실패로 발동 차단.
  - 코스트 GE 지정·수치는 어빌리티 데이터/에셋 작업 + game-designer 밸런스.

### 3-2. ③ 방향 나침반
- 신규 `USpyCompassWidget`. 로컬 플레이어 컨트롤 회전(또는 카메라) yaw 를 매 tick 읽어 방위 눈금을 이동.
- **순수함수** `HeadingToCardinal(float Yaw) → 방위 라벨/각도` 로 표현 로직을 분리(테스트 대상).
- 목표 텍스트(미션명·진행)는 **기존 MissionComponent 바인딩 유지** — 나침반 위젯은 방향만 책임.

### 3-3. ⑤ 스킬바 / 쿨다운
- 신규 `USpySkillBarWidget` — `USpyInputConfig` 의 `AbilityInputActions` 중 `Input_Ability_Skill_1~6` 태그로 슬롯을 **동적 생성**(6슬롯, Parry 제외).
- 신규 `USpySkillSlotWidget` — 슬롯 1개: 키힌트, (플레이스홀더) 아이콘, 쿨다운 시각화, 마나 코스트 숫자.
  - 쿨다운: 슬롯이 대응 어빌리티의 쿨다운 태그(`Cooldown_Skill_Action_*`)로 ASC `GetCooldownTimeRemainingAndDuration` 조회.
  - **순수함수** `CooldownNormalized(Remaining, Duration) → 0..1`(테스트 대상).
  - 시각화: 데모 단순화를 위해 **어둠 오버레이 스윕 + 잔여 초 숫자**(커스텀 머티리얼 불필요). 원형 라디얼 머티리얼은 옵션 확장.
  - 마나 코스트 숫자: 대응 어빌리티의 `ManaCost` 표시.

### 3-4. 호스팅
- `USpyMainHUD` 가 `WBP_Compass`·`WBP_SkillBar` 를 자식으로 임베드하고 `PB_Mana` 를 소유. UI 진입은 기존 `USpyUIManager` 경유 관례 유지.

## 4. 신규 산출물

**C++ (gameplay-programmer)**
- `USpyCompassWidget` (신규)
- `USpySkillBarWidget` (신규)
- `USpySkillSlotWidget` (신규)
- `USpyMainHUD` 확장 — `PB_Mana` 바인딩 + 마나 구독 + 나침반/스킬바 호스팅
- `SpyGameplayTags` — `Data_Cost_Mana` 태그 등록(선언+정의)
- 스킬 코스트 메커니즘 — 액션 스킬 베이스 `ManaCost` + `ApplyCost/CheckCost` 마나 주입
- 순수함수 2종: `HeadingToCardinal`, `CooldownNormalized`

**에셋 (메인 unreal-mcp / 사용자 수동)**
- `WBP_Compass`·`WBP_SkillBar`·`WBP_SkillSlot`(신규, C++ 클래스로 reparent)
- `WBP_MainHUD` — `PB_Mana` 추가 + 나침반·스킬바 임베드
- `GE_Cost_Mana` GameplayEffect
- 액션 스킬 데이터에 코스트 GE·`ManaCost` 지정

**수치 (game-designer)**
- 스킬별 마나 코스트, (선택) 쿨다운 지속시간

## 5. 테스트 (test-engineer)

Unreal Automation, `SkillProject.HUD.*` 네임스페이스:
- `HeadingToCardinal` — 경계각(0/45/90/315…)에서 올바른 방위 반환.
- `CooldownNormalized` — Duration 0/음수 방어, Remaining≥Duration, 중간값 클램프.
- 스킬바 슬롯 생성 — 입력 태그 N개 → 슬롯 N개, 순서 보존.

순수함수는 위젯/월드 없이 테스트 가능하도록 자유함수 또는 static 으로 분리한다.

## 6. 범위 밖 (YAGNI)

- 미션 웨이포인트·목표까지 거리·미니맵
- 가젯/변장 슬롯 (목업 ⑥ 계열)
- 스킬 아이콘 아트 (플레이스홀더 사용)
- 실제 쿨다운 밸런스 수치 튜닝 (표시 파이프라인만; 지속시간 데이터는 후속)
- 적 체력바·킬피드(②⑦) — 이번 범위 아님

## 7. 서버 권한 / 룰 준수

- 마나 코스트 감산은 GA `ApplyCost`(서버 권한 흐름) 표준 경로 — 별도 RPC 불필요.
- HUD·나침반·쿨다운 표시는 순수 클라이언트 연출(로컬 폰 기준). 서버 상태 변경 없음.
- 신규 태그는 문자열 리터럴 금지, `SpyGameplayTags` 에 `UE_DECLARE/DEFINE`.
- 위젯은 `USpyUserWidget` 상속, UI 진입은 `USpyUIManager` 경유 (plugin-skuicore 룰).
