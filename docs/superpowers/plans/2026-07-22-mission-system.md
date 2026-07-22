# 미션 시스템 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 정해진 순서대로 제시되는 미션 6종(적 처치 / 파쿠르 / 벽타기 / 그래플링 / 콤보 / 레벨 달성)을 수행하면 경험치를 받고, 진행 중인 미션을 MainHUD에 표시한다.

**Architecture:** `ASpyPlayerState`에 붙는 `USpyMissionComponent`가 네 종류의 진행 신호를 전부 `(태그, 수량)` 으로 정규화해 받는다. 진행 판정은 `USpyMissionConfig::ResolveMissionProgress` 순수 함수로 분리해 Automation 으로 검증한다. 미션 정의는 DataAsset 의 순서 배열이라 미션 추가가 데이터 추가로 끝난다.

**Tech Stack:** Unreal Engine 5.7 / C++ / GameplayAbilities(GAS) / SKGAS·SKUICore 플러그인 / UMG / Unreal Automation

**Spec:** `docs/superpowers/specs/2026-07-22-mission-system-design.md`

## Global Constraints

이 섹션의 요구사항은 **모든 Task 에 암묵적으로 포함**된다.

- **주석은 `//#` 로 시작한다.** 일반 `//`, `///`, `/* */` 금지. UE 자동 생성 저작권 헤더는 예외. (`.claude/rules/cpp-style.md`)
- **`!` 단항 부정 금지.** `bFlag == false`, `Ptr == nullptr`, `IsValid(X) == false` 로 명시 비교한다.
- **UObject 포인터는 `TObjectPtr<>`**.
- **include 순서**: 자기 자신 → UE 헤더 → 프로젝트 헤더 → `*.generated.h`(마지막). **clang-format 은 이 순서를 재정렬하지 않는다**(`SortIncludes: Never`) — 작성자가 직접 지킬 것.
- **`.cpp` 마지막 include 는 `#include UE_INLINE_GENERATED_CPP_BY_NAME(<ClassName>)`**.
- **레플리케이션**: `ReplicatedUsing` 프로퍼티는 `GetLifetimeReplicatedProps` 등록 필수, 오버라이드에서 `Super::` 호출 필수.
- **게임플레이 태그는 문자열 리터럴 금지.** `UE_DECLARE_GAMEPLAY_TAG_EXTERN`(.h) + `UE_DEFINE_GAMEPLAY_TAG`(.cpp).
- **서버 권한**: 진행도 변경은 서버에서만. 어빌리티 활성화 콜백은 클라이언트에서도 발화하므로 권한 게이트 필수.
- **SKGAS 플러그인 본체를 수정하지 않는다.** 엔진이 제공하는 `UAbilitySystemComponent::AbilityActivatedCallbacks`(public)를 쓴다. (`.claude/rules/unreal-infra.md` §3)
- **밸런스 수치를 코드에 박지 않는다.** 미션 순서·목표 수·보상 XP·표시 이름은 전부 DataAsset 입력값이다.
- **`UGameplayAbility::AbilityTags` 를 쓰지 않는다 — `GetAssetTags()` 를 쓴다.** UE 5.5 부터 `AbilityTags` 는 `UE_DEPRECATED_FORGAME(5.5, "Use GetAssetTags()...")` 로 표시되어 있다(엔진 `GameplayAbility.h:497-499` 확인). 기존 프로젝트 코드(`SpyGameplayAbility_SkillAction.cpp:69`)가 아직 옛 형태를 쓰고 있으나 **이번 작업에서 그 방식을 답습하지 않는다.** (기존 코드 수정은 이번 범위 밖)
- **`git commit` 금지.** 각 Task 끝에서 `git add` 까지만 하고 커밋 메시지(안)를 남긴다.
- **빌드·테스트 실행은 사용자 몫.** 이 환경에는 컴파일러·에디터 실행 경로가 없다. 검증 결과를 지어내지 않는다.

---

## File Structure

**신규 파일**

| 파일 | 책임 |
|---|---|
| `Data/SpyMissionConfig.h\|.cpp` | 미션 정의 배열 + 진행 판정 순수 함수 |
| `System/SpyMissionComponent.h\|.cpp` | 진행도 보유·판정 적용·보상 지급 (서버 권한), HUD 델리게이트 |
| `System/Tests/SpyMissionTests.cpp` | `ResolveMissionProgress` Automation 테스트 |

**수정 파일**

| 파일 | 변경 |
|---|---|
| `System/SpyPlayerState.h\|.cpp` | 컴포넌트 생성 + ASC 초기화 연결 |
| `Util/SpyGameplayTags.h\|.cpp` | `Event_Mission_Kill` / `Event_Mission_Combo` / `Event_Mission_Level` |
| `Character/SpyLevelComponent.cpp` | 처치 이벤트 발신(`HandleDeath`) + 레벨 승급 이벤트 발신(`TryLevelUp` 승급 블록) |
| `AbilitySystem/Skill/SpyGameplayAbility_SkillAction.cpp` | 콤보 성공 이벤트 발신 |
| `UI/SpyMainHUD.h\|.cpp` | 미션 이름·진행도 표시 |

---

## Task 1: SpyMissionConfig — 미션 정의 + 진행 판정 순수 함수

이 기능에서 자동 테스트가 가능한 유일한 부분이라 제일 먼저, TDD 로 간다.

**Files:**
- Create: `SkillProject/Source/SkillProject/Data/SpyMissionConfig.h`
- Create: `SkillProject/Source/SkillProject/Data/SpyMissionConfig.cpp`
- Test: `SkillProject/Source/SkillProject/System/Tests/SpyMissionTests.cpp`

**Interfaces:**
- Consumes: 없음
- Produces:
  - `enum class ESpyMissionMode : uint8 { Accumulate, Threshold }`
  - `struct FSpyMissionEntry { FGameplayTag MatchTag; ESpyMissionMode Mode; int32 TargetCount; float ExperienceReward; FText DisplayName; }`
  - `struct FSpyMissionProgressResult { int32 MissionIndex; int32 Count; bool bCompletedNow; bool bAllCompleted; }`
  - `int32 USpyMissionConfig::GetMissionCount() const`
  - `bool USpyMissionConfig::IsValidMissionIndex(int32 InIndex) const`
  - `const FSpyMissionEntry* USpyMissionConfig::GetMission(int32 InIndex) const`
  - `FSpyMissionProgressResult USpyMissionConfig::ResolveMissionProgress(int32 InIndex, int32 InCount, FGameplayTag InEventTag, int32 InAmount) const`
  - 프로퍼티: `TArray<FSpyMissionEntry> Missions`

- [ ] **Step 1: 실패하는 테스트 작성**

`SkillProject/Source/SkillProject/System/Tests/SpyMissionTests.cpp` 생성. 구조·플래그·`#if WITH_DEV_AUTOMATION_TESTS` 래핑은 기존 `SkillProject/Source/SkillProject/AI/Tests/SpyAICircleStrafeTests.cpp` 를 따른다 (`EditorContext | ProductFilter`).

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"
#include "Data/SpyMissionConfig.h"
#include "Util/SpyGameplayTags.h"

#if WITH_DEV_AUTOMATION_TESTS

//# 픽스처 — [0] Vault 3회(Accumulate), [1] 레벨 3 도달(Threshold)
static USpyMissionConfig* SpyMissionTests_MakeConfig()
{
    USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

    FSpyMissionEntry Vault;
    Vault.MatchTag = SpyGameplayTags::Skill_Move_Vault;
    Vault.Mode = ESpyMissionMode::Accumulate;
    Vault.TargetCount = 3;
    Vault.ExperienceReward = 10.f;
    Config->Missions.Add(Vault);

    FSpyMissionEntry Level;
    Level.MatchTag = SpyGameplayTags::Skill_Move_Climb;
    Level.Mode = ESpyMissionMode::Threshold;
    Level.TargetCount = 3;
    Level.ExperienceReward = 20.f;
    Config->Missions.Add(Level);

    return Config;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpyMissionTagMismatchTest,
    "SkillProject.System.Mission.TagMismatch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTagMismatchTest::RunTest(const FString& Parameters)
{
    const USpyMissionConfig* Config = SpyMissionTests_MakeConfig();

    //# 현재 미션은 Vault 인데 Climb 이벤트가 들어옴
    const FSpyMissionProgressResult Result = Config->ResolveMissionProgress(0, 1, SpyGameplayTags::Skill_Move_Climb, 1);

    TestEqual(TEXT("Index unchanged"), Result.MissionIndex, 0);
    TestEqual(TEXT("Count unchanged"), Result.Count, 1);
    TestFalse(TEXT("Not completed"), Result.bCompletedNow);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpyMissionAccumulatePartialTest,
    "SkillProject.System.Mission.AccumulatePartial",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionAccumulatePartialTest::RunTest(const FString& Parameters)
{
    const USpyMissionConfig* Config = SpyMissionTests_MakeConfig();

    const FSpyMissionProgressResult Result = Config->ResolveMissionProgress(0, 1, SpyGameplayTags::Skill_Move_Vault, 1);

    TestEqual(TEXT("Count increased"), Result.Count, 2);
    TestEqual(TEXT("Index unchanged"), Result.MissionIndex, 0);
    TestFalse(TEXT("Not completed"), Result.bCompletedNow);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpyMissionAccumulateExactTest,
    "SkillProject.System.Mission.AccumulateExact",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionAccumulateExactTest::RunTest(const FString& Parameters)
{
    const USpyMissionConfig* Config = SpyMissionTests_MakeConfig();

    const FSpyMissionProgressResult Result = Config->ResolveMissionProgress(0, 2, SpyGameplayTags::Skill_Move_Vault, 1);

    TestTrue(TEXT("Completed"), Result.bCompletedNow);
    TestEqual(TEXT("Advanced to next mission"), Result.MissionIndex, 1);
    TestEqual(TEXT("Count reset"), Result.Count, 0);
    TestFalse(TEXT("Not all completed"), Result.bAllCompleted);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpyMissionAccumulateOvershootTest,
    "SkillProject.System.Mission.AccumulateOvershoot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionAccumulateOvershootTest::RunTest(const FString& Parameters)
{
    const USpyMissionConfig* Config = SpyMissionTests_MakeConfig();

    //# 목표 3인데 한 번에 10 이 들어옴 — 초과분은 다음 미션으로 이월하지 않는다
    const FSpyMissionProgressResult Result = Config->ResolveMissionProgress(0, 0, SpyGameplayTags::Skill_Move_Vault, 10);

    TestTrue(TEXT("Completed"), Result.bCompletedNow);
    TestEqual(TEXT("Advanced to next mission"), Result.MissionIndex, 1);
    TestEqual(TEXT("No carry-over"), Result.Count, 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpyMissionThresholdTest,
    "SkillProject.System.Mission.Threshold",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionThresholdTest::RunTest(const FString& Parameters)
{
    const USpyMissionConfig* Config = SpyMissionTests_MakeConfig();

    //# Threshold 는 누적하지 않고 값을 대치한다 — 미달
    const FSpyMissionProgressResult Below = Config->ResolveMissionProgress(1, 0, SpyGameplayTags::Skill_Move_Climb, 2);
    TestEqual(TEXT("Value replaced"), Below.Count, 2);
    TestFalse(TEXT("Not completed"), Below.bCompletedNow);

    //# 도달 — 마지막 미션이므로 전체 완료
    const FSpyMissionProgressResult Reached = Config->ResolveMissionProgress(1, 2, SpyGameplayTags::Skill_Move_Climb, 3);
    TestTrue(TEXT("Completed"), Reached.bCompletedNow);
    TestTrue(TEXT("All completed"), Reached.bAllCompleted);
    TestEqual(TEXT("Index past the end"), Reached.MissionIndex, 2);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpyMissionThresholdNoAccumulateTest,
    "SkillProject.System.Mission.ThresholdNoAccumulate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionThresholdNoAccumulateTest::RunTest(const FString& Parameters)
{
    const USpyMissionConfig* Config = SpyMissionTests_MakeConfig();

    //# 2가 세 번 들어와도 누적되지 않으므로 목표 3에 도달하지 않는다
    FSpyMissionProgressResult Result = Config->ResolveMissionProgress(1, 0, SpyGameplayTags::Skill_Move_Climb, 2);
    Result = Config->ResolveMissionProgress(Result.MissionIndex, Result.Count, SpyGameplayTags::Skill_Move_Climb, 2);
    Result = Config->ResolveMissionProgress(Result.MissionIndex, Result.Count, SpyGameplayTags::Skill_Move_Climb, 2);

    TestEqual(TEXT("Still replaced, not accumulated"), Result.Count, 2);
    TestFalse(TEXT("Never completed"), Result.bCompletedNow);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpyMissionAfterAllCompletedTest,
    "SkillProject.System.Mission.AfterAllCompleted",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionAfterAllCompletedTest::RunTest(const FString& Parameters)
{
    const USpyMissionConfig* Config = SpyMissionTests_MakeConfig();

    //# 인덱스가 배열 범위를 벗어난 상태 — 추가 이벤트에 반응하지 않는다
    const FSpyMissionProgressResult Result = Config->ResolveMissionProgress(2, 0, SpyGameplayTags::Skill_Move_Vault, 5);

    TestEqual(TEXT("Index unchanged"), Result.MissionIndex, 2);
    TestEqual(TEXT("Count unchanged"), Result.Count, 0);
    TestFalse(TEXT("Nothing completed"), Result.bCompletedNow);
    TestTrue(TEXT("Reported as all completed"), Result.bAllCompleted);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpyMissionEmptyConfigTest,
    "SkillProject.System.Mission.EmptyConfig",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionEmptyConfigTest::RunTest(const FString& Parameters)
{
    USpyMissionConfig* Config = NewObject<USpyMissionConfig>();
    Config->Missions.Empty();

    TestEqual(TEXT("No missions"), Config->GetMissionCount(), 0);

    const FSpyMissionProgressResult Result = Config->ResolveMissionProgress(0, 0, SpyGameplayTags::Skill_Move_Vault, 1);

    TestTrue(TEXT("All completed with empty config"), Result.bAllCompleted);
    TestFalse(TEXT("Nothing completed"), Result.bCompletedNow);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpyMissionHierarchicalTagTest,
    "SkillProject.System.Mission.HierarchicalTag",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionHierarchicalTagTest::RunTest(const FString& Parameters)
{
    USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

    //# 부모 태그 Skill.Move 로 하위 전체를 묶는다
    FSpyMissionEntry AnyMove;
    AnyMove.MatchTag = SKGameplayTags::Skill_Move;
    AnyMove.Mode = ESpyMissionMode::Accumulate;
    AnyMove.TargetCount = 2;
    Config->Missions.Add(AnyMove);

    //# 자식 태그 이벤트가 부모 미션에 반영돼야 한다
    FSpyMissionProgressResult Result = Config->ResolveMissionProgress(0, 0, SpyGameplayTags::Skill_Move_Vault, 1);
    TestEqual(TEXT("Child tag matched parent mission"), Result.Count, 1);

    Result = Config->ResolveMissionProgress(Result.MissionIndex, Result.Count, SpyGameplayTags::Skill_Move_GrappleHook, 1);
    TestTrue(TEXT("Completed by a different child tag"), Result.bCompletedNow);

    return true;
}

#endif //# WITH_DEV_AUTOMATION_TESTS
```

- [ ] **Step 2: 테스트가 실패(= 컴파일 실패)하는지 확인**

이 시점에 `SpyMissionConfig.h` 가 없으므로 **컴파일 에러가 정상**이다. 사용자에게 빌드를 요청해 `Cannot open include file: 'Data/SpyMissionConfig.h'` 를 확인한다. 결과를 지어내지 않는다.

- [ ] **Step 3: `SpyMissionConfig.h` 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "SpyMissionConfig.generated.h"

//# 진행도 집계 방식
UENUM(BlueprintType)
enum class ESpyMissionMode : uint8
{
    //# 이벤트 수량을 누적한다 (파쿠르 N회, 처치 N회 등)
    Accumulate,

    //# 이벤트가 전달한 값으로 대치한다 (레벨 N 도달 등)
    Threshold,
};

//# 미션 1개의 정의
USTRUCT(BlueprintType)
struct FSpyMissionEntry
{
    GENERATED_BODY()

public:
    //# 이 미션이 반응할 이벤트 태그. 계층 매칭이므로 부모 태그로 하위를 묶을 수 있다
    UPROPERTY(EditDefaultsOnly, Category = "Mission")
    FGameplayTag MatchTag;

    UPROPERTY(EditDefaultsOnly, Category = "Mission")
    ESpyMissionMode Mode = ESpyMissionMode::Accumulate;

    UPROPERTY(EditDefaultsOnly, Category = "Mission", meta = (ClampMin = "1"))
    int32 TargetCount = 1;

    //# 완료 시 지급할 경험치
    UPROPERTY(EditDefaultsOnly, Category = "Mission", meta = (ClampMin = "0.0"))
    float ExperienceReward = 0.f;

    //# HUD 표시 이름
    UPROPERTY(EditDefaultsOnly, Category = "Mission")
    FText DisplayName;
};

//# 진행 판정 결과 — 부수효과 없는 계산의 출력
USTRUCT(BlueprintType)
struct FSpyMissionProgressResult
{
    GENERATED_BODY()

public:
    //# 판정 후 진행 중인 미션 인덱스. 배열 범위를 벗어나면 전체 완료
    UPROPERTY(BlueprintReadOnly)
    int32 MissionIndex = 0;

    //# 판정 후 현재 미션의 누적치
    UPROPERTY(BlueprintReadOnly)
    int32 Count = 0;

    //# 이번 판정으로 미션 하나가 완료됐는가
    UPROPERTY(BlueprintReadOnly)
    bool bCompletedNow = false;

    //# 마지막 미션까지 전부 끝났는가
    UPROPERTY(BlueprintReadOnly)
    bool bAllCompleted = false;
};

UCLASS()
class SKILLPROJECT_API USpyMissionConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    //# 배열 인덱스가 곧 진행 순서다
    UPROPERTY(EditDefaultsOnly, Category = "Mission")
    TArray<FSpyMissionEntry> Missions;

public:
    UFUNCTION(BlueprintPure, Category = "Mission")
    int32 GetMissionCount() const;

    UFUNCTION(BlueprintPure, Category = "Mission")
    bool IsValidMissionIndex(int32 InIndex) const;

    //# 범위 밖이면 nullptr
    const FSpyMissionEntry* GetMission(int32 InIndex) const;

    //# 진행 판정 — 부수효과 없음. 한 번의 호출로 최대 1개 미션만 완료한다
    UFUNCTION(BlueprintPure, Category = "Mission")
    FSpyMissionProgressResult ResolveMissionProgress(int32 InIndex, int32 InCount, FGameplayTag InEventTag, int32 InAmount) const;
};
```

- [ ] **Step 4: `SpyMissionConfig.cpp` 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/SpyMissionConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyMissionConfig)

int32 USpyMissionConfig::GetMissionCount() const
{
    return Missions.Num();
}

bool USpyMissionConfig::IsValidMissionIndex(int32 InIndex) const
{
    return Missions.IsValidIndex(InIndex);
}

const FSpyMissionEntry* USpyMissionConfig::GetMission(int32 InIndex) const
{
    if (Missions.IsValidIndex(InIndex) == false)
    {
        return nullptr;
    }

    return &Missions[InIndex];
}

FSpyMissionProgressResult USpyMissionConfig::ResolveMissionProgress(int32 InIndex, int32 InCount, FGameplayTag InEventTag, int32 InAmount) const
{
    FSpyMissionProgressResult Result;
    Result.MissionIndex = FMath::Max(0, InIndex);
    Result.Count = FMath::Max(0, InCount);
    Result.bCompletedNow = false;
    Result.bAllCompleted = false;

    const FSpyMissionEntry* Entry = GetMission(Result.MissionIndex);

    //# 인덱스가 범위 밖 = 전체 완료 상태. 추가 이벤트에 반응하지 않는다
    if (Entry == nullptr)
    {
        Result.bAllCompleted = true;

        return Result;
    }

    //# 계층 매칭 — 부모 태그 미션이 자식 태그 이벤트를 받아들인다
    if (InEventTag.IsValid() == false || InEventTag.MatchesTag(Entry->MatchTag) == false)
    {
        return Result;
    }

    if (Entry->Mode == ESpyMissionMode::Accumulate)
    {
        Result.Count += FMath::Max(0, InAmount);
    }
    else
    {
        //# Threshold — 누적하지 않고 대치한다
        Result.Count = FMath::Max(0, InAmount);
    }

    if (Result.Count >= Entry->TargetCount)
    {
        Result.bCompletedNow = true;
        Result.MissionIndex += 1;

        //# 초과분은 다음 미션으로 이월하지 않는다 (미션 종류가 서로 달라 의미가 없다)
        Result.Count = 0;

        if (IsValidMissionIndex(Result.MissionIndex) == false)
        {
            Result.bAllCompleted = true;
        }
    }

    return Result;
}
```

- [ ] **Step 5: 빌드 + 테스트 통과 확인 (사용자 수행)**

사용자에게 요청:
1. Visual Studio 에서 `SkillProject` 빌드.
2. 에디터 → `Tools > Session Frontend > Automation` → `SkillProject.System.Mission` 필터 → 9개 실행.

기대: `TagMismatch` / `AccumulatePartial` / `AccumulateExact` / `AccumulateOvershoot` / `Threshold` / `ThresholdNoAccumulate` / `AfterAllCompleted` / `EmptyConfig` / `HierarchicalTag` 전부 PASS.

- [ ] **Step 6: 스테이징**

```bash
git add SkillProject/Source/SkillProject/Data/SpyMissionConfig.h SkillProject/Source/SkillProject/Data/SpyMissionConfig.cpp SkillProject/Source/SkillProject/System/Tests/SpyMissionTests.cpp
```

커밋 메시지(안): `[Feature] SpyMissionConfig — 미션 정의 데이터 + 진행 판정 함수`

---

## Task 2: SpyMissionComponent — 진행도 보유 · 판정 적용 · 보상

**Files:**
- Create: `SkillProject/Source/SkillProject/System/SpyMissionComponent.h`
- Create: `SkillProject/Source/SkillProject/System/SpyMissionComponent.cpp`
- Modify: `SkillProject/Source/SkillProject/System/SpyPlayerState.h`
- Modify: `SkillProject/Source/SkillProject/System/SpyPlayerState.cpp` (생성자 ~29-34행, `PostInitializeComponents` ~63-69행)

**Interfaces:**
- Consumes: Task 1 의 `ResolveMissionProgress` / `GetMission` / `IsValidMissionIndex`, 기존 `USpyGE_ExperienceGain`, 기존 `SpyGameplayTags::Data_Experience_Gain`
- Produces:
  - `USpyMissionComponent::FindMissionComponent(const AActor*)`
  - `InitializeByAbilitySystem(USpyAbilitySystemComponent*)` / `UnInitializeByAbilitySystem()`
  - `void AddProgress(FGameplayTag InEventTag, int32 InAmount)`
  - `int32 GetMissionIndex() const` / `int32 GetCount() const` / `int32 GetTargetCount() const` / `FText GetDisplayName() const` / `bool IsAllCompleted() const`
  - `FSpyMission_ProgressChanged OnMissionProgressChanged(USpyMissionComponent*, int32 Index, int32 Count, int32 Target)`
  - `FSpyMission_Completed OnMissionCompleted(USpyMissionComponent*, int32 CompletedIndex)`
  - `FSpyMission_AllCompleted OnAllMissionsCompleted(USpyMissionComponent*)`

- [ ] **Step 1: `SpyMissionComponent.h` 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameFrameworkComponent.h"
#include "GameplayTagContainer.h"

#include "SpyMissionComponent.generated.h"

class USpyAbilitySystemComponent;
class USpyMissionConfig;
class UGameplayAbility;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FSpyMission_ProgressChanged, USpyMissionComponent*, MissionComponent, int32, MissionIndex, int32, Count, int32, TargetCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSpyMission_Completed, USpyMissionComponent*, MissionComponent, int32, CompletedIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSpyMission_AllCompleted, USpyMissionComponent*, MissionComponent);

//# 재진입 가드에 걸린 진행 이벤트를 보관한다 (버리지 않고 순차 처리)
USTRUCT()
struct FSpyMissionPendingEvent
{
    GENERATED_BODY()

public:
    UPROPERTY()
    FGameplayTag EventTag;

    UPROPERTY()
    int32 Amount = 0;
};

//# 복제 단위 — 인덱스와 카운트를 한 구조체로 묶어 둘 중 하나만 바뀌어도 OnRep이 발화하게 한다
USTRUCT()
struct FSpyMissionState
{
    GENERATED_BODY()

public:
    UPROPERTY()
    int32 MissionIndex = 0;

    UPROPERTY()
    int32 Count = 0;
};

UCLASS()
class SKILLPROJECT_API USpyMissionComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()

public:
	USpyMissionComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure)
	static USpyMissionComponent* FindMissionComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<USpyMissionComponent>() : nullptr); }

	void InitializeByAbilitySystem(USpyAbilitySystemComponent* InASC);
	void UnInitializeByAbilitySystem();

	//# 모든 진행 신호의 단일 진입점. 서버 권한에서만 동작한다
	void AddProgress(FGameplayTag InEventTag, int32 InAmount);

	UFUNCTION(BlueprintPure)
	int32 GetMissionIndex() const { return MissionState.MissionIndex; }

	UFUNCTION(BlueprintPure)
	int32 GetCount() const { return MissionState.Count; }

	UFUNCTION(BlueprintPure)
	int32 GetTargetCount() const;

	UFUNCTION(BlueprintPure)
	FText GetDisplayName() const;

	UFUNCTION(BlueprintPure)
	bool IsAllCompleted() const;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnUnregister() override;

	//# 어빌리티 활성화 → 어빌리티 태그를 진행 이벤트로 변환
	void HandleAbilityActivated(UGameplayAbility* InAbility);

	//# 실제 판정 1회. AddProgress가 가드와 큐를 관리하고 이 함수를 호출한다
	void ProcessProgress(FGameplayTag InEventTag, int32 InAmount);

	//# 완료 보상 지급 (서버)
	void GrantReward(int32 InCompletedIndex);

	UFUNCTION()
	void OnRep_MissionState();

public:
	UPROPERTY(BlueprintAssignable)
	FSpyMission_ProgressChanged OnMissionProgressChanged;

	UPROPERTY(BlueprintAssignable)
	FSpyMission_Completed OnMissionCompleted;

	UPROPERTY(BlueprintAssignable)
	FSpyMission_AllCompleted OnAllMissionsCompleted;

protected:
	//# PlayerState BP 기본값에서 지정한다
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	TObjectPtr<USpyMissionConfig> MissionConfig;

	UPROPERTY(ReplicatedUsing = OnRep_MissionState)
	FSpyMissionState MissionState;

	UPROPERTY()
	TObjectPtr<USpyAbilitySystemComponent> AbilitySystemComponent;

	//# 재진입 가드 — 보상 XP가 레벨업을 유발하고 그것이 다시 AddProgress를 부르는 경로가 실재한다
	bool bProcessingProgress = false;

	//# 가드에 걸린 이벤트 (유실 방지)
	UPROPERTY()
	TArray<FSpyMissionPendingEvent> PendingEvents;

	FDelegateHandle AbilityActivatedHandle;
};
```

- [ ] **Step 2: `SpyMissionComponent.cpp` — 초기화 / 조회 / 정리**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.


#include "System/SpyMissionComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystem/Effect/SpyGE_ExperienceGain.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "Data/SpyMissionConfig.h"
#include "Util/SpyGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyMissionComponent)

USpyMissionComponent::USpyMissionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void USpyMissionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//# 자기 진행도만 보면 되므로 소유 클라이언트에만 복제한다
	DOREPLIFETIME_CONDITION(USpyMissionComponent, MissionState, COND_OwnerOnly);
}

void USpyMissionComponent::InitializeByAbilitySystem(USpyAbilitySystemComponent* InASC)
{
	if (InASC == nullptr)
		return;

	AbilitySystemComponent = InASC;

	//# 어빌리티 활성화 콜백 — 파쿠르/벽타기/그래플 미션의 신호원
	//# SKGAS 본체를 수정하지 않고 엔진이 제공하는 public 델리게이트를 쓴다 (unreal-infra §3)
	AbilityActivatedHandle = AbilitySystemComponent->AbilityActivatedCallbacks.AddUObject(this, &ThisClass::HandleAbilityActivated);
}

void USpyMissionComponent::UnInitializeByAbilitySystem()
{
	if (AbilitySystemComponent && AbilityActivatedHandle.IsValid())
	{
		AbilitySystemComponent->AbilityActivatedCallbacks.Remove(AbilityActivatedHandle);
	}

	AbilityActivatedHandle.Reset();
	AbilitySystemComponent = nullptr;
}

void USpyMissionComponent::OnUnregister()
{
	UnInitializeByAbilitySystem();

	Super::OnUnregister();
}

int32 USpyMissionComponent::GetTargetCount() const
{
	if (MissionConfig == nullptr)
		return 0;

	const FSpyMissionEntry* Entry = MissionConfig->GetMission(MissionState.MissionIndex);

	return (Entry ? Entry->TargetCount : 0);
}

FText USpyMissionComponent::GetDisplayName() const
{
	if (MissionConfig == nullptr)
		return FText::GetEmpty();

	const FSpyMissionEntry* Entry = MissionConfig->GetMission(MissionState.MissionIndex);

	return (Entry ? Entry->DisplayName : FText::GetEmpty());
}

bool USpyMissionComponent::IsAllCompleted() const
{
	if (MissionConfig == nullptr)
		return false;

	return (MissionConfig->IsValidMissionIndex(MissionState.MissionIndex) == false);
}

void USpyMissionComponent::OnRep_MissionState()
{
	//# 클라이언트 표시 갱신
	OnMissionProgressChanged.Broadcast(this, MissionState.MissionIndex, MissionState.Count, GetTargetCount());

	if (IsAllCompleted())
	{
		OnAllMissionsCompleted.Broadcast(this);
	}
}
```

- [ ] **Step 3: `.cpp` — 진행 처리와 재진입 가드**

```cpp
void USpyMissionComponent::HandleAbilityActivated(UGameplayAbility* InAbility)
{
	if (InAbility == nullptr)
		return;

	//# GA가 태그를 여러 개 가질 수 있으므로 전부 넘긴다.
	//# 현재 미션의 MatchTag와 매칭되는 것만 반영되므로 중복 카운트는 없다
	for (const FGameplayTag& Tag : InAbility->GetAssetTags())
	{
		AddProgress(Tag, 1);
	}
}

void USpyMissionComponent::AddProgress(FGameplayTag InEventTag, int32 InAmount)
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr || Owner->HasAuthority() == false)
		return;

	if (MissionConfig == nullptr)
		return;

	//# 처리 중에 들어온 이벤트는 버리지 않고 큐에 쌓는다.
	//# (보상 XP → 레벨업 → OnLevelChanged → AddProgress 경로가 실재한다)
	if (bProcessingProgress)
	{
		FSpyMissionPendingEvent Pending;
		Pending.EventTag = InEventTag;
		Pending.Amount = InAmount;
		PendingEvents.Add(Pending);

		return;
	}

	TGuardValue<bool> ReentryGuard(bProcessingProgress, true);

	ProcessProgress(InEventTag, InAmount);

	//# 가드 안에서 쌓인 이벤트를 순차 처리한다
	while (PendingEvents.Num() > 0)
	{
		const FSpyMissionPendingEvent Next = PendingEvents[0];
		PendingEvents.RemoveAt(0);

		ProcessProgress(Next.EventTag, Next.Amount);
	}
}

void USpyMissionComponent::ProcessProgress(FGameplayTag InEventTag, int32 InAmount)
{
	if (MissionConfig == nullptr)
		return;

	const FSpyMissionProgressResult Result = MissionConfig->ResolveMissionProgress(
		MissionState.MissionIndex, MissionState.Count, InEventTag, InAmount);

	const bool bChanged = (Result.MissionIndex != MissionState.MissionIndex) || (Result.Count != MissionState.Count);
	if (bChanged == false)
		return;

	const int32 CompletedIndex = MissionState.MissionIndex;

	MissionState.MissionIndex = Result.MissionIndex;
	MissionState.Count = Result.Count;

	//# 서버 브로드캐스트 (클라이언트는 OnRep_MissionState에서 처리)
	OnMissionProgressChanged.Broadcast(this, MissionState.MissionIndex, MissionState.Count, GetTargetCount());

	if (Result.bCompletedNow)
	{
		OnMissionCompleted.Broadcast(this, CompletedIndex);

		GrantReward(CompletedIndex);

		UE_LOG(LogTemp, Log, TEXT("# [SpyMissionComponent] Mission %d completed by %s"), CompletedIndex, *GetNameSafe(GetOwner()));
	}

	if (Result.bAllCompleted)
	{
		OnAllMissionsCompleted.Broadcast(this);
	}
}

void USpyMissionComponent::GrantReward(int32 InCompletedIndex)
{
	if (MissionConfig == nullptr || AbilitySystemComponent == nullptr)
		return;

	const FSpyMissionEntry* Entry = MissionConfig->GetMission(InCompletedIndex);
	if (Entry == nullptr || Entry->ExperienceReward <= 0.f)
		return;

	//# 기존 경험치 이펙트를 재사용한다 (새 GE를 만들지 않는다)
	FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
	ContextHandle.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
		USpyGE_ExperienceGain::StaticClass(), 1.f, ContextHandle);
	if (SpecHandle.IsValid() == false)
		return;

	SpecHandle.Data->SetSetByCallerMagnitude(SpyGameplayTags::Data_Experience_Gain, Entry->ExperienceReward);
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}
```

- [ ] **Step 4: `SpyPlayerState` 에 컴포넌트 연결**

`SpyPlayerState.h` — `CharacterAttributeSet` 선언 근처에 추가한다. 가시성 지정자는 기존 `AbilitySystemComponent` 선언(`UPROPERTY(VisibleAnywhere)`)에 맞춘다.

```cpp
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class USpyMissionComponent> MissionComponent;
```

`SpyPlayerState.cpp` 생성자, `CharacterAttributeSet` 생성(34행) 아래:

```cpp
	MissionComponent = CreateDefaultSubobject<USpyMissionComponent>(TEXT("MissionComponent"));
```

`PostInitializeComponents()` 의 `AbilitySystemComponent->InitAbilityActorInfo(this, GetPawn());`(69행) **직후**:

```cpp
	//# ASC 초기화 이후에만 미션 컴포넌트를 붙인다 (plugin-modulargameplayactors §InitState)
	MissionComponent->InitializeByAbilitySystem(AbilitySystemComponent);
```

`SpyPlayerState.cpp` include 에 `#include "System/SpyMissionComponent.h"` 추가.

- [ ] **Step 5: 빌드 확인 (사용자 수행)**

컴파일 통과 + Task 1 의 9개 테스트가 계속 PASS 인지 확인(회귀 없음). 이 시점에는 아직 미션 Config 에셋이 없어 인게임 동작은 확인할 수 없다.

- [ ] **Step 6: 스테이징**

```bash
git add SkillProject/Source/SkillProject/System/SpyMissionComponent.h SkillProject/Source/SkillProject/System/SpyMissionComponent.cpp SkillProject/Source/SkillProject/System/SpyPlayerState.h SkillProject/Source/SkillProject/System/SpyPlayerState.cpp
```

커밋 메시지(안): `[Feature] SpyMissionComponent — 미션 진행도 추적 + 완료 보상`

---

## Task 3: 진행 신호원 3종 — 처치 · 콤보 · 레벨

Task 2 로 파쿠르·벽타기·그래플은 이미 동작한다(어빌리티 태그 경유). 이 Task 는 태그로 표현되지 않는 나머지 3종을 잇는다.

**Files:**
- Modify: `SkillProject/Source/SkillProject/Util/SpyGameplayTags.h`
- Modify: `SkillProject/Source/SkillProject/Util/SpyGameplayTags.cpp`
- Modify: `SkillProject/Source/SkillProject/Character/SpyLevelComponent.cpp` (`HandleDeath` ~138-158행)
- Modify: `SkillProject/Source/SkillProject/AbilitySystem/Skill/SpyGameplayAbility_SkillAction.cpp` (~67-85행)
- Modify: `SkillProject/Source/SkillProject/System/SpyMissionComponent.h|.cpp`

**Interfaces:**
- Consumes: Task 2 의 `AddProgress` / `FindMissionComponent`
- Produces:
  - `SpyGameplayTags::Event_Mission_Kill` / `Event_Mission_Combo` / `Event_Mission_Level`
  - (레벨 신호는 `USpyLevelComponent::TryLevelUp` 에서 직접 발신한다 — 미션 컴포넌트에 전용 핸들러를 추가하지 않는다)

- [ ] **Step 1: 태그 등록**

`SpyGameplayTags.h` 의 `namespace SpyGameplayTags` 안, 기존 `Data_*` 태그 아래에 추가:

```cpp
	//# 미션 진행 이벤트 태그
	SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Mission_Kill);
	SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Mission_Combo);
	SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Mission_Level);
```

`SpyGameplayTags.cpp` 에 정의 추가 (기존 파일의 나열 스타일을 먼저 읽고 맞출 것):

```cpp
	UE_DEFINE_GAMEPLAY_TAG(Event_Mission_Kill, "Event.Mission.Kill");
	UE_DEFINE_GAMEPLAY_TAG(Event_Mission_Combo, "Event.Mission.Combo");
	UE_DEFINE_GAMEPLAY_TAG(Event_Mission_Level, "Event.Mission.Level");
```

- [ ] **Step 2: 처치 신호 — `SpyLevelComponent::HandleDeath`**

경험치 지급 블록에 이미 킬러 ASC 해석(PlayerState → Pawn 폴백, 자살 제외)과 `bDeathRewardGranted` 1회 보장이 있다. **같은 블록 끝에 미션 진행을 함께 보낸다.**

`SpyLevelComponent.cpp` 의 `HandleDeath` 마지막(`KillerASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);` 다음 줄)에 추가:

```cpp
	//# 미션 진행 — 킬러의 미션 컴포넌트는 PlayerState에 있다
	if (USpyMissionComponent* KillerMission = USpyMissionComponent::FindMissionComponent(KillerASC->GetOwnerActor()))
	{
		KillerMission->AddProgress(SpyGameplayTags::Event_Mission_Kill, 1);
	}
```

include 추가: `#include "System/SpyMissionComponent.h"`

**주의**: `GetOwnerActor()` 는 ASC 의 소유 액터(= `ASpyPlayerState`)를 돌려준다. `GetAvatarActor()`(= Pawn)가 아니다 — 미션 컴포넌트는 PlayerState 에 있으므로 **`GetOwnerActor()` 를 써야 한다.**

- [ ] **Step 3: 콤보 신호 — `SpyGameplayAbility_SkillAction::ActivateAbility`**

**⚠ 이 단계는 초안에서 정정됐다. `InputPressed` 에 훅을 두면 안 된다.**

초안은 `InputPressed`(51행)의 콤보 확정 블록을 지목했으나, **그 함수는 데디케이티드 서버에서 원격 플레이어에 대해 실행되지 않는다.** `InputPressed` 가 서버 인스턴스까지 전달되려면 `bReplicateInputDirectly` 가 참이어야 하는데, MCP 로 확인한 결과 `GA_SkillA` ~ `GA_SkillF` **6개 전부 `False`** 이고 C++ 설정 코드도 없다. 그대로 두면 콤보 미션이 서버에서 0 에 고정되고 순차 체인이 거기서 막힌다.

대신 **`ActivateAbility`(15행, 이미 오버라이드됨)** 에서 잡는다. 콤보 연결은 `HandleGameplayEvent` 로 다음 GA 를 발동시키고, GAS 가 이를 `ServerTryActivateAbilityWithEventData` 로 TriggerEventData 와 함께 서버에 전달하므로 서버에서 확정적으로 잡힌다.

**먼저 `SpyGameplayTags` 에 콤보 태그 컨테이너 헬퍼를 추가한다.**

`SpyGameplayTags.h` 의 namespace 안:

```cpp
	//# 콤보 태그 5종 묶음. 부모 태그(Skill.Util) 매칭은 Skill.Util.Death 까지 걸리므로 쓸 수 없다
	SKILLPROJECT_API const FGameplayTagContainer& GetComboTags();
```

`SpyGameplayTags.cpp`:

```cpp
	const FGameplayTagContainer& GetComboTags()
	{
		static FGameplayTagContainer ComboTags = []()
		{
			FGameplayTagContainer Container;
			Container.AddTag(Skill_Util_Combo1);
			Container.AddTag(Skill_Util_Combo2);
			Container.AddTag(Skill_Util_Combo3);
			Container.AddTag(Skill_Util_Combo4);
			Container.AddTag(Skill_Util_Combo5);

			return Container;
		}();

		return ComboTags;
	}
```

`SpyGameplayTags.h` include 에 `#include "GameplayTagContainer.h"` 가 없으면 추가한다.

**`SpyGameplayAbility_SkillAction.cpp` 의 `ActivateAbility`**, `Super::ActivateAbility(...)` 호출 다음에 추가:

```cpp
	//# 미션 진행 — 콤보로 연결되어 활성화된 경우만 센다.
	//# 최초 입력 활성화는 TriggerEventData가 없으므로 세지 않는다 (3연타 = 연결 2회)
	if (HasAuthority(&ActivationInfo) && TriggerEventData != nullptr)
	{
		if (SpyGameplayTags::GetComboTags().HasTagExact(TriggerEventData->EventTag))
		{
			if (ASpyCharacter* OwnerCharacter = Cast<ASpyCharacter>(GetAvatarActorFromActorInfo()))
			{
				if (USpyMissionComponent* MissionComp = USpyMissionComponent::FindMissionComponent(OwnerCharacter->GetPlayerState()))
				{
					MissionComp->AddProgress(SpyGameplayTags::Event_Mission_Combo, 1);
				}
			}
		}
	}
```

include 추가: `#include "System/SpyMissionComponent.h"`, `#include "Character/SpyCharacter.h"`

**`InputPressed` 에는 미션 관련 코드를 넣지 않는다.** 기존 콤보 체인 로직은 그대로 둔다 — 건드리는 것은 `ActivateAbility` 뿐이다.

**게이트 2 는 확인 완료**: 콤보 창을 여는 `SpyAnimNotify_State_Combo` 는 서버에서도 실행된다. `BP_SpyCharacter` 메시가 `VisibilityBasedAnimTickOption = AlwaysTickPoseAndRefreshBones`(MCP 확인)라 서버에서 항상 pose 를 틱한다.

- [ ] **Step 4: 레벨 신호 — `SpyLevelComponent::TryLevelUp` 에서 발신**

**⚠ 이 단계는 초안에서 정정됐다. `USpyCharacterAttributeSet::OnLevelChanged` 를 구독하면 안 된다.**

그 델리게이트는 **서버에서 발화하지 않는다** — 브로드캐스트 지점이 `SpyCharacterAttributeSet.cpp:43`(`PostGameplayEffectExecute` 의 Level 분기, `Level` 을 수정하는 GE 가 없어 실행 안 됨)과 `:72`(`OnRep_Level`, 클라이언트 전용) 둘뿐이다. 서버는 `SetNumericAttributeBase` 로 레벨을 바꾸므로 GE 경로를 타지 않는다. 구독하면 **레벨 미션이 영원히 진행되지 않고 순차 체인이 거기서 막힌다.**

대신 **서버에서 실제로 발화하는 지점**(`SpyLevelComponent.cpp:243` 승급 블록)에서 미션 컴포넌트로 직접 밀어 넣는다. 처치 신호(Step 2)와 같은 배선 패턴이다.

`SpyLevelComponent.cpp` 의 `TryLevelUp` 안, 기존 `OnLevelChanged.Broadcast(this, OldLevel, Result.Level);` **다음 줄**에 추가:

```cpp
		//# 미션 진행 — 미션 컴포넌트는 PlayerState에 있다.
		//# AttributeSet의 OnLevelChanged는 서버에서 발화하지 않으므로 이 지점에서 직접 보낸다
		if (APawn* OwnerPawn = Cast<APawn>(Owner))
		{
			if (USpyMissionComponent* OwnerMission = USpyMissionComponent::FindMissionComponent(OwnerPawn->GetPlayerState()))
			{
				//# Threshold 모드이므로 누적이 아니라 도달한 레벨값을 그대로 넘긴다
				OwnerMission->AddProgress(SpyGameplayTags::Event_Mission_Level, Result.Level);
			}
		}
```

`Owner` 는 같은 함수 상단에서 이미 확보돼 있다(`AActor* Owner = GetOwner();`). include 는 Step 2 에서 이미 `#include "System/SpyMissionComponent.h"` 를 추가했으므로 추가 작업이 없다.

**`USpyMissionComponent` 에 레벨 전용 핸들러·멤버를 만들지 않는다.** `LevelSet` 멤버도 필요 없다 — 진입점은 `AddProgress` 하나다.

**주의**: 이 블록은 `TryLevelUp` 의 `Result.LevelsGained > 0` 조건 안에 있어야 한다(승급이 실제로 일어난 경우만). 한 번에 2레벨 이상 오르면 `Result.Level` 은 최종 레벨이므로 `Threshold` 판정에 그대로 쓸 수 있다.

- [ ] **Step 5: ~~`GA_GrappleHook` 태그 부여~~ — 불필요해짐 (2026-07-22)**

이 단계는 **범용 어빌리티 태그 훅을 전제로 한 것**이었다. spec §6-1 정정으로 이동 미션이 GA 의 `AbilityTags` 를 읽지 않게 되어(각 GA 가 실제 수행 지점에서 직접 신호 발신), **그래플 미션 동작에 이 태그 부여가 필요하지 않다.**

`Skill.Move.GrappleHook` 태그 자체는 미션 `MatchTag` 로 계속 쓰인다. 2026-07-22 에 MCP 로 이미 부여해 두었으므로 되돌리지 않는다(GA 식별용으로 형제 GA 들과 일관되기도 하다).

- [ ] **Step 6: 빌드 + 인게임 확인 (사용자 수행)**

1. 빌드.
2. `SpyMissionConfig` DataAsset 생성 + 미션 배열 입력, `BP_SpyPlayerState` 의 `MissionComponent → MissionConfig` 에 지정.
3. PIE 에서 각 행동 수행 → 출력 로그에 `# [SpyMissionComponent] Mission N completed` 가 순서대로 찍히는지 확인.

- [ ] **Step 7: 스테이징**

```bash
git add SkillProject/Source/SkillProject/Util/SpyGameplayTags.h SkillProject/Source/SkillProject/Util/SpyGameplayTags.cpp SkillProject/Source/SkillProject/Character/SpyLevelComponent.cpp SkillProject/Source/SkillProject/AbilitySystem/Skill/SpyGameplayAbility_SkillAction.cpp SkillProject/Source/SkillProject/System/SpyMissionComponent.h SkillProject/Source/SkillProject/System/SpyMissionComponent.cpp
```

커밋 메시지(안): `[Feature] SpyMissionComponent — 처치·콤보·레벨 진행 신호 연결`

---

## Task 4: MainHUD 미션 표시

**Files:**
- Modify: `SkillProject/Source/SkillProject/UI/SpyMainHUD.h`
- Modify: `SkillProject/Source/SkillProject/UI/SpyMainHUD.cpp`

**Interfaces:**
- Consumes: Task 2 의 `USpyMissionComponent` 델리게이트·조회 함수
- Produces: 없음 (말단)

**바인딩 대상이 경험치 HUD 와 다르다**: 미션 컴포넌트는 **PlayerState** 에 있으므로 `GetOwningPlayerState()` 로 찾는다. 폰을 기다릴 필요가 없어 경험치 바보다 빨리 바인딩된다.

- [ ] **Step 1: 헤더 확장**

기존 `PB_Exp` / `Txt_Level` 선언 아래에 추가:

```cpp
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_MissionName;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_MissionProgress;
```

`protected:` 에 추가:

```cpp
	bool TryBindMissionComponent();
	void UnbindMissionComponent();

	UFUNCTION()
	void HandleMissionProgressChanged(class USpyMissionComponent* InMissionComponent, int32 InMissionIndex, int32 InCount, int32 InTargetCount);

	UFUNCTION()
	void HandleAllMissionsCompleted(class USpyMissionComponent* InMissionComponent);

	void RefreshMission();

	UPROPERTY()
	TObjectPtr<class USpyMissionComponent> BoundMissionComponent;
```

- [ ] **Step 2: `.cpp` 구현**

`NativeConstruct` 의 기존 재시도 람다 안에서 미션 바인딩도 함께 시도한다. 기존 코드가 `TryBindLevelComponent()` 성공 시 타이머를 끄므로, **두 바인딩이 모두 성공해야 타이머를 끄도록** 조건을 바꾼다.

```cpp
//# NativeConstruct 안 — 최초 1회 시도
const bool bLevelBound = TryBindLevelComponent();
const bool bMissionBound = TryBindMissionComponent();

if (bLevelBound == false || bMissionBound == false)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(BindRetryTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
        {
            const bool bLevelOk = TryBindLevelComponent();
            const bool bMissionOk = TryBindMissionComponent();

            if (bLevelOk && bMissionOk)
            {
                if (UWorld* InnerWorld = GetWorld())
                {
                    InnerWorld->GetTimerManager().ClearTimer(BindRetryTimerHandle);
                }

                return;
            }

            BindRetryCount++;
            if (BindRetryCount >= BindRetryMaxCount)
            {
                UE_LOG(LogTemp, Warning, TEXT("# [SpyMainHUD] 위젯 바인딩에 실패해 재시도를 중단합니다. Level=%d Mission=%d — 캐릭터 BP의 LevelConfig, PlayerState BP의 MissionConfig 지정을 확인하세요."),
                    bLevelOk, bMissionOk);

                if (UWorld* InnerWorld = GetWorld())
                {
                    InnerWorld->GetTimerManager().ClearTimer(BindRetryTimerHandle);
                }
            }
        }), 0.25f, true);
    }
}
```

바인딩·갱신 함수:

```cpp
bool USpyMainHUD::TryBindMissionComponent()
{
    if (BoundMissionComponent)
        return true;

    APlayerController* OwningController = GetOwningPlayer();
    if (OwningController == nullptr)
        return false;

    APlayerState* OwningState = OwningController->PlayerState;
    if (OwningState == nullptr)
        return false;

    USpyMissionComponent* MissionComponent = USpyMissionComponent::FindMissionComponent(OwningState);
    if (MissionComponent == nullptr)
        return false;

    BoundMissionComponent = MissionComponent;

    BoundMissionComponent->OnMissionProgressChanged.AddDynamic(this, &USpyMainHUD::HandleMissionProgressChanged);
    BoundMissionComponent->OnAllMissionsCompleted.AddDynamic(this, &USpyMainHUD::HandleAllMissionsCompleted);

    //# 구독 전에 이미 진행된 값을 놓치지 않도록 즉시 1회 갱신
    RefreshMission();

    return true;
}

void USpyMainHUD::UnbindMissionComponent()
{
    if (BoundMissionComponent)
    {
        BoundMissionComponent->OnMissionProgressChanged.RemoveDynamic(this, &USpyMainHUD::HandleMissionProgressChanged);
        BoundMissionComponent->OnAllMissionsCompleted.RemoveDynamic(this, &USpyMainHUD::HandleAllMissionsCompleted);
    }

    BoundMissionComponent = nullptr;
}

void USpyMainHUD::HandleMissionProgressChanged(USpyMissionComponent* InMissionComponent, int32 InMissionIndex, int32 InCount, int32 InTargetCount)
{
    RefreshMission();
}

void USpyMainHUD::HandleAllMissionsCompleted(USpyMissionComponent* InMissionComponent)
{
    RefreshMission();
}

void USpyMainHUD::RefreshMission()
{
    if (BoundMissionComponent == nullptr)
        return;

    const bool bAllDone = BoundMissionComponent->IsAllCompleted();

    if (Txt_MissionName)
    {
        Txt_MissionName->SetText(bAllDone
            ? NSLOCTEXT("SpyMainHUD", "MissionAllCompleted", "모든 미션 완료")
            : BoundMissionComponent->GetDisplayName());
    }

    if (Txt_MissionProgress)
    {
        if (bAllDone)
        {
            Txt_MissionProgress->SetText(FText::GetEmpty());
        }
        else
        {
            Txt_MissionProgress->SetText(FText::Format(
                NSLOCTEXT("SpyMainHUD", "MissionProgressFormat", "{0} / {1}"),
                FText::AsNumber(BoundMissionComponent->GetCount()),
                FText::AsNumber(BoundMissionComponent->GetTargetCount())));
        }
    }
}
```

`NativeDestruct` 에 `UnbindMissionComponent();` 를 기존 `UnbindLevelComponent();` 옆에 추가.

include 추가: `#include "System/SpyMissionComponent.h"`

- [ ] **Step 3: 위젯 배치 (사용자 수행)**

`WBP_MainHUD` 에 `Txt_MissionName`(TextBlock), `Txt_MissionProgress`(TextBlock) 을 **정확한 이름으로** 배치하고 컴파일·저장한다.

**스크립트(MCP)로 위젯을 생성하지 말 것.** 생성은 되지만 GUID 가 부여되지 않아 `Error: Widget [X] was added but did not get a GUID` 가 뜨고, 결국 디자이너에서 한 번 더 컴파일해야 한다.

- [ ] **Step 4: 인게임 확인 (사용자 수행)**

1. **1인 PIE** 로 순차 체인을 처음부터 완주 — 미션 이름이 순서대로 바뀌고 진행도가 `N / M` 으로 올라가는지, 완료 시 경험치가 오르는지.
2. **2인 PIE** 로 클라이언트 창에서도 자기 미션이 표시·갱신되는지 (`COND_OwnerOnly` 복제 확인).

- [ ] **Step 5: 스테이징**

```bash
git add SkillProject/Source/SkillProject/UI/SpyMainHUD.h SkillProject/Source/SkillProject/UI/SpyMainHUD.cpp
```

커밋 메시지(안): `[Feature] SpyMainHUD — 미션 이름/진행도 표시 추가`

---

## 완료 조건

- [ ] `SkillProject.System.Mission` 9개 테스트 PASS
- [ ] 파쿠르·벽타기·그래플 수행 시 해당 미션 진행도가 오른다 (그래플은 `GA_GrappleHook` 태그 부여 후)
- [ ] 봇 처치 / 콤보 연결 / 레벨 달성이 각각 미션에 반영된다
- [ ] 미션 완료 시 경험치가 오르고 다음 미션으로 넘어간다
- [ ] 마지막 미션 완료 후 추가 행동에도 크래시·이상 동작이 없다
- [ ] MainHUD 가 **리슨 서버와 클라이언트 양쪽에서** 자기 미션을 표시한다
- [ ] `MissionConfig` 미지정 상태에서도 크래시 없이 동작한다 (미션이 진행되지 않을 뿐)
- [ ] **`GA_Vault` / `GA_WallClimb` / `GA_HanpUp` / `GA_GrappleHook` 의 `NetExecutionPolicy` 가 `LocalOnly` 가 아닐 것.**
  `LocalOnly` 면 서버가 GA 를 실행하지 않아 `AbilityActivatedCallbacks` 가 서버에서 발화하지 않고, 해당 이동 미션이 **무증상으로 0 에 고정**되어 순차 체인이 거기서 막힌다.
  **2026-07-22 MCP 실측: 4개 전부 `LocalPredicted` 로 확인됨.** 이동 GA 를 새로 추가하거나 기존 GA 의 정책을 바꿀 때 이 조건을 다시 확인할 것.
