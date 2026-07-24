# HUD ①③⑤ (마나바 · 방향 나침반 · 스킬바/쿨다운) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** MainHUD 에 마나바 + 실제 스킬 마나 코스트, 방향 나침반, 데이터 구동 스킬바(쿨다운 시각화)를 추가한다.

**Architecture:** 접근법 A — 분해 위젯. `USpyCompassWidget`·`USpySkillBarWidget`·`USpySkillSlotWidget` 를 독립 `USpyUserWidget` 으로 만들고 `USpyMainHUD` 가 호스트한다. 표현 로직은 순수함수(`HeadingToCardinal`·`CooldownNormalized`)로 분리해 위젯 없이 테스트한다. 마나 코스트는 GAS 표준 코스트 경로(SetByCaller GE)로 구현한다.

**Tech Stack:** Unreal Engine 5.7, C++, SKGAS(커스텀 GAS), SKUICore(UI 매니저/위젯 베이스), Enhanced Input, Unreal Automation(`WITH_DEV_AUTOMATION_TESTS`).

## Global Constraints

- 주석은 `//#` 로 시작. `!` 단항 부정 금지 — `== false` / `== nullptr` 명시 비교. UObject 포인터는 `TObjectPtr`. (cpp-style.md)
- 게임플레이 태그는 문자열 리터럴 금지 — `SpyGameplayTags.h/.cpp` 에 `UE_DECLARE_GAMEPLAY_TAG_EXTERN` + `UE_DEFINE_GAMEPLAY_TAG`. (plugin-skgas.md §2)
- 위젯은 `USpyUserWidget` 상속, UI 진입은 `USpyUIManager` 경유. (plugin-skuicore.md)
- 서버 권한: 마나 코스트 감산은 GA `ApplyCost` 표준 경로. HUD·나침반·쿨다운 표시는 로컬 클라 연출(상태 변경 없음).
- **빌드/테스트는 사용자가 에디터·VS 에서 수행** — 이 환경엔 CLI 빌드/테스트 커맨드가 없다. 각 태스크의 "실행" 스텝은 사용자 수동 검증 지점이다.
- **`git commit` 자동 금지** — 관련 파일 `git add` 까지만, 커밋 메시지(안)은 파이프라인 마무리에서 일괄 제시. (git-conventions.md)
- 테스트 등록 문자열 `"SkillProject.<도메인>.<기능>.<케이스>"`, 파일 전체 `#if WITH_DEV_AUTOMATION_TESTS` 로 감싼다. 기존 `AI/Tests/SpyAICircleStrafeTests.cpp` 스타일 준수.
- 코스트 수치·(선택)쿨다운 지속시간은 **game-designer 설계 산출물**(`docs/design/`)을 SoT 로 삼는다 — 이 플랜은 메커니즘만.

---

### Task 1: 순수 표현 함수 + 태그 (`SpyHUDMath` + `Data.Cost.Mana`)

위젯/월드 없이 테스트 가능한 표현 로직과 신규 태그를 먼저 확정한다. 이후 위젯 태스크가 이 함수를 소비한다.

**Files:**
- Create: `SkillProject/Source/SkillProject/UI/SpyHUDMath.h`
- Create: `SkillProject/Source/SkillProject/UI/SpyHUDMath.cpp`
- Modify: `SkillProject/Source/SkillProject/Util/SpyGameplayTags.h` (SetByCaller 데이터 태그 절)
- Modify: `SkillProject/Source/SkillProject/Util/SpyGameplayTags.cpp`
- Test: `SkillProject/Source/SkillProject/UI/Tests/SpyHUDMathTests.cpp`

**Interfaces:**
- Produces:
  - `enum class ESpyCardinal : uint8 { N, NE, E, SE, S, SW, W, NW }`
  - `ESpyCardinal SpyHUDMath::HeadingToCardinal(float YawDegrees)` — yaw 를 [0,360) 로 정규화 후 8방위. 경계는 각 방위 중심 ±22.5°, N 중심=0°.
  - `float SpyHUDMath::CooldownNormalized(float Remaining, float Duration)` — `Duration <= 0` 이면 0. `Remaining` 을 `[0, Duration]` 로 클램프 후 `Remaining/Duration` 반환(0=준비완료, 1=방금발동).
  - 태그 `SpyGameplayTags::Data_Cost_Mana` (`"Data.Cost.Mana"`).

- [ ] **Step 1: 실패 테스트 작성** — `SpyHUDMathTests.cpp` 에 `HeadingToCardinal` 경계값과 `CooldownNormalized` 방어/클램프 케이스.

```cpp
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "UI/SpyHUDMath.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpyHUDMathHeadingTest, "SkillProject.HUD.Math.HeadingToCardinal", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSpyHUDMathHeadingTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("0 -> N"), SpyHUDMath::HeadingToCardinal(0.f), ESpyCardinal::N);
    TestEqual(TEXT("360 wraps to N"), SpyHUDMath::HeadingToCardinal(360.f), ESpyCardinal::N);
    TestEqual(TEXT("-90 wraps to W"), SpyHUDMath::HeadingToCardinal(-90.f), ESpyCardinal::W);
    TestEqual(TEXT("90 -> E"), SpyHUDMath::HeadingToCardinal(90.f), ESpyCardinal::E);
    TestEqual(TEXT("180 -> S"), SpyHUDMath::HeadingToCardinal(180.f), ESpyCardinal::S);
    TestEqual(TEXT("45 -> NE"), SpyHUDMath::HeadingToCardinal(45.f), ESpyCardinal::NE);
    TestEqual(TEXT("22.4 still N"), SpyHUDMath::HeadingToCardinal(22.4f), ESpyCardinal::N);
    TestEqual(TEXT("22.6 tips to NE"), SpyHUDMath::HeadingToCardinal(22.6f), ESpyCardinal::NE);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpyHUDMathCooldownTest, "SkillProject.HUD.Math.CooldownNormalized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSpyHUDMathCooldownTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("zero duration -> 0"), SpyHUDMath::CooldownNormalized(5.f, 0.f), 0.f);
    TestEqual(TEXT("negative duration -> 0"), SpyHUDMath::CooldownNormalized(5.f, -3.f), 0.f);
    TestEqual(TEXT("full remaining -> 1"), SpyHUDMath::CooldownNormalized(4.f, 4.f), 1.f);
    TestEqual(TEXT("half -> 0.5"), SpyHUDMath::CooldownNormalized(2.f, 4.f), 0.5f);
    TestEqual(TEXT("remaining>duration clamps 1"), SpyHUDMath::CooldownNormalized(9.f, 4.f), 1.f);
    TestEqual(TEXT("negative remaining clamps 0"), SpyHUDMath::CooldownNormalized(-1.f, 4.f), 0.f);
    return true;
}
#endif
```

- [ ] **Step 2: 컴파일 실패 확인** — `SpyHUDMath.h/.cpp` 아직 없음. 빌드 시 미해결 심볼(사용자 빌드에서 확인, 또는 헤더 부재로 컴파일 에러).

- [ ] **Step 3: `SpyHUDMath` 구현**

```cpp
//# SpyHUDMath.h
#pragma once
#include "CoreMinimal.h"

enum class ESpyCardinal : uint8 { N, NE, E, SE, S, SW, W, NW };

namespace SpyHUDMath
{
    //# yaw(도)를 8방위로. N 중심=0, 각 방위 폭 45도(±22.5)
    SKILLPROJECT_API ESpyCardinal HeadingToCardinal(float YawDegrees);

    //# 쿨다운 진행 0(준비)~1(방금발동). Duration<=0 이면 0
    SKILLPROJECT_API float CooldownNormalized(float Remaining, float Duration);
}
```

```cpp
//# SpyHUDMath.cpp
#include "UI/SpyHUDMath.h"

ESpyCardinal SpyHUDMath::HeadingToCardinal(float YawDegrees)
{
    //# [0,360) 정규화
    float Yaw = FMath::Fmod(YawDegrees, 360.f);
    if (Yaw < 0.f)
    {
        Yaw += 360.f;
    }
    //# 22.5도 오프셋 후 45도 섹터 인덱스 (0=N)
    const int32 Sector = FMath::FloorToInt((Yaw + 22.5f) / 45.f) % 8;
    return static_cast<ESpyCardinal>(Sector);
}

float SpyHUDMath::CooldownNormalized(float Remaining, float Duration)
{
    if (Duration <= 0.f)
    {
        return 0.f;
    }
    return FMath::Clamp(Remaining / Duration, 0.f, 1.f);
}
```

- [ ] **Step 4: `Data.Cost.Mana` 태그 추가** — 기존 SetByCaller 데이터 태그 절(`Data_Experience_Gain` 인접)에 선언/정의 추가.

```cpp
//# SpyGameplayTags.h — SetByCaller 데이터 태그 절
SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Cost_Mana);
```
```cpp
//# SpyGameplayTags.cpp — 동일 절
UE_DEFINE_GAMEPLAY_TAG(Data_Cost_Mana, "Data.Cost.Mana");
```

- [ ] **Step 5: 빌드 + Automation 실행(사용자)** — 에디터 재컴파일 후 Session Frontend 에서 `SkillProject.HUD.Math.*` 2건 실행. Expected: PASS. 태그는 Project Settings > GameplayTags 에 `Data.Cost.Mana` 노출 확인.

- [ ] **Step 6: 스테이징** — `git add` 신규/수정 파일. 커밋은 파이프라인 마무리에서.

---

### Task 2: 스킬 마나 코스트 (`GE`, `ManaCost`, `ApplyCost/CheckCost`)

액션 스킬이 마나를 실제로 소비하고, 부족하면 발동을 차단하게 한다.

**Files:**
- Create: `SkillProject/Source/SkillProject/AbilitySystem/Effect/SpyGE_ManaCost.h`
- Create: `SkillProject/Source/SkillProject/AbilitySystem/Effect/SpyGE_ManaCost.cpp`
- Modify: `SkillProject/Source/SkillProject/AbilitySystem/Skill/SpyGameplayAbility_SkillAction.h`
- Modify: `SkillProject/Source/SkillProject/AbilitySystem/Skill/SpyGameplayAbility_SkillAction.cpp`

**Interfaces:**
- Consumes: `SpyGameplayTags::Data_Cost_Mana` (Task 1).
- Produces:
  - `USpyGE_ManaCost` — `Data.Cost.Mana` SetByCaller 매그니튜드로 `USKAttributeSet::Mana` 를 Additive 감산하는 즉시(Instant) GE. 생성자에서 modifier 를 구성한다(`SpyGE_ExperienceGain` 선례 참고).
  - `USpyGameplayAbility_SkillAction::ManaCost` (`float`, `EditDefaultsOnly`, 기본 0) — 어빌리티별 코스트.
  - `ApplyCost` 오버라이드 — `ManaCost > 0` 이면 `USpyGE_ManaCost` 스펙에 `SetByCallerMagnitude(Data.Cost.Mana, ManaCost)` 를 실어 적용.
  - `CheckCost` 오버라이드 — 현재 `Mana < ManaCost` 이면 false 반환(발동 차단).

**Notes:**
- `SpyGE_ExperienceGain.h/.cpp` 가 C++ GE + SetByCaller 구성의 선례다. 동일 패턴으로 modifier 를 `EGameplayModOp::Additive`, `FSetByCallerFloat{ Data_Cost_Mana }` 로 만든다(부호는 감산이 되도록).
- `CheckCost` 는 `const` — ASC 는 `ActorInfo->AbilitySystemComponent` 에서 얻어 `GetNumericAttribute(USKAttributeSet::GetManaAttribute())` 로 현재 마나를 읽는다.
- 코스트 GE 클래스는 어빌리티의 `CostGameplayEffectClass` 로 지정하지 않고(레벨/매그니튜드 주입을 위해) `ApplyCost` 에서 직접 `MakeOutgoingSpec` → `SetByCaller` → `ApplyGameplayEffectSpecToSelf` 한다.

- [ ] **Step 1: `USpyGE_ManaCost` 작성** — `SpyGE_ExperienceGain` 을 템플릿으로, modifier 를 Mana Additive + `Data.Cost.Mana` SetByCaller 로 구성. 코드는 선례 파일을 열어 동일 구조로 작성(속성명만 Mana/Data.Cost.Mana 로 교체).

- [ ] **Step 2: `ManaCost` 프로퍼티 추가** (`SpyGameplayAbility_SkillAction.h`)

```cpp
//# 어빌리티별 마나 코스트. 0 이면 코스트 없음. 수치는 game-designer 설계
UPROPERTY(EditDefaultsOnly, Category = "Cost", meta = (ClampMin = "0.0"))
float ManaCost = 0.f;
```

- [ ] **Step 3: `ApplyCost`/`CheckCost` 오버라이드 선언 + 구현** — `CheckCost` 는 현재 마나와 `ManaCost` 비교, `ApplyCost` 는 SetByCaller 스펙 적용. `ManaCost <= 0` 이면 base 동작으로 폴백.

```cpp
//# CheckCost 골격 (const)
bool USpyGameplayAbility_SkillAction::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
    if (ManaCost <= 0.f)
    {
        return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
    }
    const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    if (ASC == nullptr)
    {
        return false;
    }
    const float CurMana = ASC->GetNumericAttribute(USKAttributeSet::GetManaAttribute());
    return CurMana >= ManaCost;
}
```

- [ ] **Step 4: 빌드 + 수동 검증(사용자)** — 코스트를 부여한 테스트 어빌리티로 PIE 에서: 마나 충분 시 발동+마나 감소, 부족 시 발동 차단 확인. (ASC 통합 자동화는 하니스 부재로 수동. 순수 비교 로직이 필요하면 Task 1 의 헬퍼로 추출 가능하나 현 범위는 수동 검증.)

- [ ] **Step 5: 스테이징** — `git add` 신규/수정 파일.

---

### Task 3: 마나 재생 GE + Mana 클램프 (`USpyGE_ManaRegen`, `PostGameplayEffectExecute`)

> **design-reviewer BLOCKER 반영 태스크.** 재생·클램프는 원 플랜에 홈이 없어 task 구동 실행에서 드롭될 위험이 있었다. 마나 경제가 실제로 성립하려면 필수.

마나가 시간당 회복되고, `MaxMana` 를 초과하지 않게 클램프한다.

**Files:**
- Create: `SkillProject/Source/SkillProject/AbilitySystem/Effect/SpyGE_ManaRegen.h`
- Create: `SkillProject/Source/SkillProject/AbilitySystem/Effect/SpyGE_ManaRegen.cpp`
- Modify: `SkillProject/Plugins/SKGAS/Source/SKGAS/Attribute/SKAttributeSet.cpp` (`PostGameplayEffectExecute` — Mana 클램프 분기 추가)

**Interfaces:**
- Produces:
  - `USpyGE_ManaRegen` — Duration=Infinite, Period=2.0s, Modifier `Mana` Additive `+3`(상수). 값은 game-designer SoT(재생 +3/2s = 1.5/s). `SpyGE_LevelGrowth`/`SpyGE_ExperienceGain` 을 C++ GE 선례로.
  - `USKAttributeSet::PostGameplayEffectExecute` 가 `Mana` 를 `[0, MaxMana]` 로 클램프.
- 부여: **에셋(Task 8)** — 캐릭터 GrantedGameplayEffects 에 `USpyGE_ManaRegen` 등록 → 기존 `GiveToAbilitySystem` 파이프라인이 grant + `FSpyAbilitySet_GrantedHandles` 트래킹. 별도 부여 코드 불필요.

**Notes:**
- design-reviewer 실측: `SKAttributeSet.cpp` 의 `PostGameplayEffectExecute` 는 **Health 분기만** 처리 → Mana 클램프 부재. Health 분기와 **대칭**으로 Mana 분기를 추가한다(Mana 는 base `USKAttributeSet` 속성이므로 클램프 위치도 base 가 맞다).
- 재생 GE 는 constant modifier 라 로직이 없다 — C++ GE 로 두는 이유는 결정성·리뷰 가능성(프로젝트 C++ GE 선례 일치)이다.

- [ ] **Step 1: `USpyGE_ManaRegen` 작성** — `SpyGE_LevelGrowth` 를 템플릿으로 Duration=Infinite, Period=2.0, Modifier `Mana` Additive `+3` 구성.

- [ ] **Step 2: Mana 클램프 추가** (`SKAttributeSet.cpp` `PostGameplayEffectExecute`) — Health 클램프 분기 바로 아래에 대칭 분기.

```cpp
//# 기존 Health 분기와 대칭 — Mana 를 [0, MaxMana] 로 클램프
else if (Data.EvaluatedData.Attribute == GetManaAttribute())
{
    SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
}
```

- [ ] **Step 3: 빌드 + 수동 검증(사용자)** — 스킬로 마나 소모 후 2초당 +3 회복, `MaxMana`(60) 초과 안 함 확인. (부여는 Task 8 에셋 완료 후.)

- [ ] **Step 4: 스테이징** — `git add` 신규/수정 파일(SKGAS 플러그인 변경 포함).

---

### Task 4: MainHUD 마나바 바인딩

경험치 바인딩과 동일 패턴으로 마나를 구독·표시한다.

**Files:**
- Modify: `SkillProject/Source/SkillProject/UI/SpyMainHUD.h`
- Modify: `SkillProject/Source/SkillProject/UI/SpyMainHUD.cpp`

**Interfaces:**
- Consumes: `USKAttributeSet::OnManaChanged` / `OnMaxManaChanged` (SKGAS).
- Produces: `PB_Mana`(BindWidgetOptional) 가 `Mana/MaxMana` 비율로 갱신.

**Notes:**
- 기존 `TryBindLevelComponent`/bind-retry 타이머 인프라를 재사용한다. 로컬 폰의 ASC → `USKAttributeSet` 를 찾아 `OnManaChanged`/`OnMaxManaChanged` 에 핸들러를 붙이고, 언바인드 대칭 유지.
- `RefreshAll` 에 마나 초기 반영 추가.

- [ ] **Step 1: 멤버 추가** (`SpyMainHUD.h`) — `TObjectPtr<UProgressBar> PB_Mana;`(BindWidgetOptional), 바인딩 대상 AttributeSet 포인터, 핸들러 선언(`HandleManaChanged`/`HandleMaxManaChanged`), `TryBindAttributeSet`/`UnbindAttributeSet`.

- [ ] **Step 2: 구현** (`SpyMainHUD.cpp`) — 로컬 폰 ASC 해석 → AttributeSet 구독. 핸들러에서 `PB_Mana->SetPercent(Mana/MaxMana)`(MaxMana<=0 방어). `NativeConstruct`/`NativeDestruct` 에서 bind/unbind, 재시도 타이머 흐름에 포함.

- [ ] **Step 3: 빌드 + 수동 검증(사용자)** — WBP_MainHUD 에 `PB_Mana` 배치(Task 8) 후 PIE 에서 스킬 사용 시 마나바 감소 확인. (Optional 바인딩이라 위젯 미배치 상태에서도 크래시 없음.)

- [ ] **Step 4: 스테이징** — `git add`.

---

### Task 5: 방향 나침반 위젯 (`USpyCompassWidget`)

**Files:**
- Create: `SkillProject/Source/SkillProject/UI/SpyCompassWidget.h`
- Create: `SkillProject/Source/SkillProject/UI/SpyCompassWidget.cpp`

**Interfaces:**
- Consumes: `SpyHUDMath::HeadingToCardinal` (Task 1).
- Produces: `USpyCompassWidget` — 매 tick 로컬 컨트롤 회전 yaw 를 읽어 방위 표시/눈금 오프셋 갱신. `USpyUserWidget` 상속.

**Notes:**
- `NativeTick` 에서 `GetOwningPlayer()->GetControlRotation().Yaw` 를 읽는다. 컨트롤러/폰 null 방어.
- 방위 라벨(`Txt_Heading`, BindWidgetOptional)에 `HeadingToCardinal` 결과를 매핑. 눈금 스트립 이동은 yaw 를 픽셀 오프셋으로 변환(위젯 폭 기준, 선형). 표현 세부는 위젯 BP 에서 조정.
- 목표 텍스트는 이 위젯 책임 아님 — MainHUD 의 미션 바인딩 유지.

- [ ] **Step 1: 클래스 작성** — `USpyUserWidget` 상속, `NativeTick` 오버라이드, `Txt_Heading` BindWidgetOptional, yaw→방위 라벨 갱신. `HeadingToCardinal` 사용.
- [ ] **Step 2: 빌드 + 수동 검증(사용자)** — WBP_Compass(Task 8) 배치 후 PIE 에서 카메라 회전 시 방위 갱신 확인.
- [ ] **Step 3: 스테이징** — `git add`.

---

### Task 6: 스킬 슬롯 위젯 (`USpySkillSlotWidget`)

슬롯 1개 = 키힌트 + 아이콘(플레이스홀더) + 쿨다운 시각화 + 마나코스트.

**Files:**
- Create: `SkillProject/Source/SkillProject/UI/SpySkillSlotWidget.h`
- Create: `SkillProject/Source/SkillProject/UI/SpySkillSlotWidget.cpp`

**Interfaces:**
- Consumes: `SpyHUDMath::CooldownNormalized` (Task 1), `USpyGameplayAbility_SkillAction::ManaCost` (Task 2).
- Produces:
  - `void USpySkillSlotWidget::Setup(UAbilitySystemComponent* ASC, FGameplayTag InputTag, FGameplayTagContainer CooldownTags, FText KeyHint, float ManaCost)` — 슬롯 초기화.
  - `NativeTick` 에서 `ASC->GetCooldownTimeRemainingAndDuration(...)` 로 잔여/지속 조회 → `CooldownNormalized` → 어둠 오버레이 스윕 + 잔여 초 텍스트. 준비 시 오버레이 숨김.

**Notes:**
- 쿨다운 조회: 쿨다운 태그 컨테이너로 `ASC->GetCooldownTimeRemainingAndDuration` 사용(어빌리티 스펙 경유). 하단 앵커 높이 스윕은 세로 `PB_Cooldown`(BindWidgetOptional, `UProgressBar`) 의 `SetPercent(CooldownNormalized)` 로 구동(§6-1: 발동=가득→준비=0, 아래로 비는 방향). 잔여 초는 `Txt_Cooldown`. (opacity 페이드 아님.)
- 마나 코스트 숫자는 `Txt_Cost`(BindWidgetOptional).
- ASC/스펙 null 방어. 로컬 클라 전용(상태 변경 없음).

- [ ] **Step 1: 클래스 작성** — `Setup` + `NativeTick` 쿨다운 갱신 + 코스트/키힌트 표시.
- [ ] **Step 2: 빌드 + 수동 검증(사용자)** — Task 7 스킬바에 실려 PIE 에서 스킬 발동 시 쿨다운 스윕·숫자 확인.
- [ ] **Step 3: 스테이징** — `git add`.

---

### Task 7: 스킬바 위젯 (`USpySkillBarWidget`, 데이터 구동)

**Files:**
- Create: `SkillProject/Source/SkillProject/UI/SpySkillBarWidget.h`
- Create: `SkillProject/Source/SkillProject/UI/SpySkillBarWidget.cpp`
- Test: `SkillProject/Source/SkillProject/UI/Tests/SpySkillBarTests.cpp`

**Interfaces:**
- Consumes: `USpySkillSlotWidget` (Task 6), `USpyInputConfig::AbilityInputActions` (기존).
- Produces:
  - `USpySkillBarWidget` — 슬롯 대상 입력 태그 목록(`Input_Ability_Skill_1~6`, `Input_Ability_Parry`)을 순서대로 순회하며 `USpySkillSlotWidget` 을 동적 생성해 컨테이너(`Panel_Slots`)에 추가.
  - `static TArray<FGameplayTag> USpySkillBarWidget::BuildSlotInputTags()` — 슬롯 순서를 반환하는 순수 정적 함수(테스트 대상: 개수·순서).

**Notes:**
- 슬롯 순서 정의를 `BuildSlotInputTags()` 로 분리해 위젯 없이 테스트한다. 위젯 생성 로직은 이 목록을 소비.
- 각 슬롯의 쿨다운 태그·코스트·키힌트는 대응 어빌리티 스펙/데이터에서 해석. 어빌리티 미부여 슬롯은 비활성 표시(빈 슬롯 유지).

- [ ] **Step 1: 실패 테스트 작성** — `SpySkillBarTests.cpp` 에서 `BuildSlotInputTags()` 가 7개(Skill_1~6 + Parry)를 정해진 순서로 반환하는지.

```cpp
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "UI/SpySkillBarWidget.h"
#include "Util/SpyGameplayTags.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpySkillBarSlotOrderTest, "SkillProject.HUD.SkillBar.SlotOrder", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSpySkillBarSlotOrderTest::RunTest(const FString& Parameters)
{
    const TArray<FGameplayTag> Tags = USpySkillBarWidget::BuildSlotInputTags();
    TestEqual(TEXT("7 slots"), Tags.Num(), 7);
    TestEqual(TEXT("first is Skill_1"), Tags[0], SpyGameplayTags::Input_Ability_Skill_1);
    TestEqual(TEXT("sixth is Skill_6"), Tags[5], SpyGameplayTags::Input_Ability_Skill_6);
    TestEqual(TEXT("last is Parry"), Tags[6], SpyGameplayTags::Input_Ability_Parry);
    return true;
}
#endif
```

- [ ] **Step 2: 컴파일 실패 확인** — `BuildSlotInputTags` 미정의.
- [ ] **Step 3: 구현** — `BuildSlotInputTags()`(정적, 태그 배열 반환) + `NativeConstruct`/`Setup` 에서 목록 순회해 슬롯 생성·`Setup` 호출.
- [ ] **Step 4: 빌드 + Automation 실행(사용자)** — `SkillProject.HUD.SkillBar.SlotOrder` PASS.
- [ ] **Step 5: 스테이징** — `git add`.

---

### Task 8: 에셋 배선 (WBP · 초기 스탯 · 재생 부여 · 코스트) — 메인 unreal-mcp + 사용자 수동

C++ 클래스를 실제 위젯/에셋에 연결하고, 마나 경제 성립에 필요한 초기 스탯·재생 부여를 에셋으로 완성한다. **에셋 한정 태스크** — 코드 변경 0건.

**대상:**
- `WBP_Compass`(신규, parent=`USpyCompassWidget`) — `Txt_Heading` + 눈금 스트립.
- `WBP_SkillSlot`(신규, parent=`USpySkillSlotWidget`) — 키힌트/아이콘/`PB_Cooldown`(세로 ProgressBar, dark fill, 하단 앵커)/`Txt_Cooldown`/`Txt_Cost`.
- `WBP_SkillBar`(신규, parent=`USpySkillBarWidget`) — `Panel_Slots`(HorizontalBox).
- `WBP_MainHUD` — `PB_Mana` 추가 + `WBP_Compass`·`WBP_SkillBar` 임베드.
- **`GE_InitStat` — `MaxMana`/`Mana` 각 `60` ADD_BASE 모디파이어 추가** (design-reviewer BLOCKER: 미설정 시 MaxMana=0 → 전 스킬 발동 불가. 단일 실패점).
- **캐릭터 GrantedGameplayEffects 에 `USpyGE_ManaRegen`(Task 3) 등록** (재생 부여 = 기존 grant 파이프라인 경유).
- 액션 스킬 데이터에 스킬별 `ManaCost` 입력(A5/B5/C15/D15/E20/F5/Parry0 — game-designer 수치), (선택) 쿨다운 초안 2/6/10s.

**Notes:**
- MCP 함정(메모리 `project-mcp-umg-editing`): 빈 WBP 루트 못 만듦 → 복제+reparent 우회, `save_asset` 은 `only_if_is_dirty=False`, 열린 에셋 컴파일 금지, `add_asset_entry` 조용한 실패 주의.
- BindWidget 이름은 C++ 프로퍼티명과 정확히 일치해야 한다.
- 위젯 GUID/BindWidget 검증 위해 **사용자 1회 컴파일** 필요.
- 수치의 SoT 는 `docs/design/hud-mana-compass-skillbar.md`.

- [ ] **Step 1: 위젯 3종 생성 + reparent + 자식 위젯 배치**(메인 MCP).
- [ ] **Step 2: WBP_MainHUD 에 PB_Mana + 두 위젯 임베드**(메인 MCP).
- [ ] **Step 3: `GE_InitStat` 에 MaxMana/Mana=60 모디파이어 추가**(메인 MCP) — 미설정 시 기능 전체 무증상 사망.
- [ ] **Step 4: 캐릭터 GrantedGameplayEffects 에 `USpyGE_ManaRegen` 등록**(메인 MCP).
- [ ] **Step 5: 스킬별 `ManaCost` 입력**(메인 MCP, 수치는 game-designer).
- [ ] **Step 6: 사용자 수동** — 디자이너에서 위젯 컴파일, PIE 통합 확인(마나바 감소·시간 재생·나침반·쿨다운 스윕).
- [ ] **Step 7: 스테이징** — 변경 `.uasset` `git add`.

---

## Self-Review

**Spec coverage:**
- §3-1 마나 표시 → Task 4. 마나 코스트 → Task 2. 마나 재생·클램프·초기 스탯(기획서 §3/§7) → Task 3(재생 GE·클램프)+Task 8(초기 스탯·재생 부여). ✓
- §3-2 나침반 + `HeadingToCardinal` → Task 1(함수)+Task 5(위젯). 목표텍스트=기존 유지(명시). ✓
- §3-3 스킬바 데이터구동 → Task 7. 슬롯/쿨다운/`CooldownNormalized` → Task 1(함수)+Task 6(슬롯). ✓
- §3-4 호스팅 → Task 8(임베드). ✓
- §4 신규 산출물(위젯 3종·MainHUD 확장·태그·코스트 메커니즘·재생 GE·순수함수 2종) → Task 1~8 전부 대응. ✓
- §5 테스트(HeadingToCardinal·CooldownNormalized·슬롯생성) → Task 1·Task 7 테스트. ✓
- §6 범위밖(웨이포인트/미니맵/가젯/아이콘아트/쿨다운수치) → 태스크 없음(의도). ✓
- **design-reviewer BLOCKER(초기 MaxMana·GE_ManaRegen·Mana 클램프 미매핑)** → Task 3(재생 GE·클램프 코드)+Task 8 Step 3·4(초기 스탯·재생 부여 에셋) 신설로 해소. ✓

**Placeholder scan:** 코스트 수치·쿨다운 지속은 game-designer SoT 로 명시(플랜은 메커니즘). 엔진 API 바디는 인터페이스+골격+선례 파일 지정으로 대체(전체 리터럴 바디는 엔진 시그니처 리스크 → 골격 제공). 의도적.

**Type consistency:** `HeadingToCardinal(float)→ESpyCardinal`, `CooldownNormalized(float,float)→float`, `BuildSlotInputTags()→TArray<FGameplayTag>`, `Setup(ASC,InputTag,CooldownTags,KeyHint,ManaCost)` — 태스크 간 일치 확인. `Data_Cost_Mana` 태그명 Task1 정의=Task2 소비 일치. ✓
