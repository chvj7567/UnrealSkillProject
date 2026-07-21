# 경험치 · 레벨 시스템 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 적을 처치해 경험치를 얻고, 임계치를 넘으면 레벨이 올라 `MaxHealth`/`MaxMana` 가 성장하며, 현재 레벨과 경험치 진행도를 MainHUD 에 표시한다.

**Architecture:** 경험치·레벨을 GAS 어트리뷰트(`USpyCharacterAttributeSet`)로 두고 레플리케이션을 GAS 에 위임한다. 레벨업 판정·경험치 지급은 `USpyHealthComponent` 와 대칭 구조의 신규 `USpyLevelComponent` 가 **서버 권한**에서 담당한다. 레벨 커브 계산은 부수효과 없는 순수 함수(`USpyLevelConfig::ResolveLevelUp`)로 분리해 Unreal Automation 으로 검증한다.

**Tech Stack:** Unreal Engine 5.7 / C++ / GameplayAbilities(GAS) / SKGAS·SKAssetCore·SKUICore 플러그인 / UMG / Unreal Automation

**Spec:** `docs/superpowers/specs/2026-07-21-experience-level-design.md`

## Global Constraints

이 섹션의 요구사항은 **모든 Task 에 암묵적으로 포함**된다.

- **주석은 `//#` 로 시작한다.** 일반 `//`, `///`, `/* */` 금지. UE 자동 생성 저작권 헤더(`// Fill out your copyright notice...`)는 예외로 그대로 둔다. (`.claude/rules/cpp-style.md`)
- **`!` 단항 부정 금지.** `bFlag == false`, `Ptr == nullptr`, `IsValid(X) == false` 로 명시 비교한다.
- **UObject 포인터는 `TObjectPtr<>`**, `UPROPERTY` 없는 raw pointer 보관 금지.
- **include 순서**: 자기 자신 → UE 헤더 → 프로젝트 헤더 → `*.generated.h` (항상 마지막).
- **`.cpp` 마지막 include 는 `#include UE_INLINE_GENERATED_CPP_BY_NAME(<ClassName>)`** — 이 모듈의 기존 파일 전부가 이 패턴을 쓴다.
- **레플리케이션**: `Replicated`/`ReplicatedUsing` 프로퍼티는 `GetLifetimeReplicatedProps` 등록 필수, 오버라이드에서 `Super::` 호출 필수.
- **게임플레이 태그는 문자열 리터럴 금지.** `UE_DECLARE_GAMEPLAY_TAG_EXTERN`(.h) + `UE_DEFINE_GAMEPLAY_TAG`(.cpp) 로 등록해 참조한다. (`.claude/rules/plugin-skgas.md` §2)
- **서버 권한**: 게임플레이 상태 변경은 서버에서만 실행하고 클라이언트에 레플리케이트한다.
- **하드코딩 수치 금지**: 밸런스 수치는 전부 `USpyLevelConfig` DataAsset 으로 뺀다.
- **`git commit` 금지.** 각 Task 끝에서 `git add` 까지만 하고 커밋 메시지(안)를 남긴다. (`.claude/rules/git-conventions.md`, 사용자 지시)
- **빌드·테스트 실행은 사용자 몫.** 이 환경에는 컴파일러/에디터 실행 커맨드가 없다. 각 Task 의 "검증" 단계는 사용자가 Visual Studio 빌드 + 에디터 Session Frontend 에서 수행한다. 에이전트는 검증 결과를 지어내지 않는다.

---

## File Structure

**신규 파일**

| 파일 | 책임 |
|---|---|
| `Data/SpyLevelConfig.h\|.cpp` | 레벨 커브 + 성장 수치 데이터, 순수 함수 `ResolveLevelUp` |
| `Character/SpyLevelComponent.h\|.cpp` | 경험치 지급 · 레벨업 적용 (서버 권한), HUD 용 델리게이트 |
| `AbilitySystem/Effect/SpyGE_ExperienceGain.h\|.cpp` | 경험치 가산 Instant GE |
| `AbilitySystem/Effect/SpyGE_LevelGrowth.h\|.cpp` | MaxHealth/MaxMana 성장 Instant GE |
| `Character/Tests/SpyLevelTests.cpp` | `ResolveLevelUp` Automation 테스트 |

**수정 파일**

| 파일 | 변경 |
|---|---|
| `Character/SpyCharacterAttributeSet.h\|.cpp` | `Experience`/`MaxExperience`/`Level` 어트리뷰트 + `PostGameplayEffectExecute` 오버라이드 |
| `Util/SpyGameplayTags.h\|.cpp` | SetByCaller 데이터 태그 3개 |
| `Character/SpyCharacter.h\|.cpp` | `SpyLevelComponent` 생성 + ASC 초기화/해제 연결 |
| `UI/SpyMainHUD.h\|.cpp` | `PB_Exp`/`Txt_Level` 바인딩 + 구독 |

---

## Task 1: SpyLevelConfig — 레벨 커브 데이터 + 순수 계산 함수

레벨업 계산 전체를 **부수효과 없는 const 함수**로 먼저 만든다. 이게 이 기능에서 유일하게 자동 테스트 가능한 부분이라 제일 먼저, TDD 로 간다.

**Files:**
- Create: `SkillProject/Source/SkillProject/Data/SpyLevelConfig.h`
- Create: `SkillProject/Source/SkillProject/Data/SpyLevelConfig.cpp`
- Test: `SkillProject/Source/SkillProject/Character/Tests/SpyLevelTests.cpp`

**Interfaces:**
- Consumes: 없음 (이 Task 가 파이프라인의 뿌리)
- Produces:
  - `struct FSpyLevelUpResult { int32 Level; float Experience; float MaxExperience; int32 LevelsGained; }`
  - `int32 USpyLevelConfig::GetMaxLevel() const`
  - `float USpyLevelConfig::GetExperienceToNextLevel(int32 InLevel) const`
  - `FSpyLevelUpResult USpyLevelConfig::ResolveLevelUp(int32 InLevel, float InExperience) const`
  - 프로퍼티: `ExperienceToNextLevel`, `MaxHealthPerLevel`, `MaxManaPerLevel`, `bFullHealOnLevelUp`, `ExperienceRewardPerLevel`

- [ ] **Step 1: 실패하는 테스트 작성**

`SkillProject/Source/SkillProject/Character/Tests/SpyLevelTests.cpp` 생성. 등록 문자열·구조·`#if WITH_DEV_AUTOMATION_TESTS` 래핑은 기존 `SkillProject/Source/SkillProject/AI/Tests/SpyAICircleStrafeTests.cpp` 스타일을 따른다.

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "Misc/AutomationTest.h"
#include "Data/SpyLevelConfig.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SpyLevelTestHelpers
{
    //# 커브 {100, 200, 300} — 최대 레벨 4
    static USpyLevelConfig* MakeConfig()
    {
        USpyLevelConfig* Config = NewObject<USpyLevelConfig>();
        Config->ExperienceToNextLevel = { 100.f, 200.f, 300.f };
        Config->MaxHealthPerLevel = 10.f;
        Config->MaxManaPerLevel = 5.f;
        Config->ExperienceRewardPerLevel = 20.f;

        return Config;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpyLevelBelowThresholdTest,
    "SkillProject.Character.Level.BelowThreshold",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpyLevelBelowThresholdTest::RunTest(const FString& Parameters)
{
    const USpyLevelConfig* Config = SpyLevelTestHelpers::MakeConfig();

    const FSpyLevelUpResult Result = Config->ResolveLevelUp(1, 50.f);

    TestEqual(TEXT("Level stays 1"), Result.Level, 1);
    TestEqual(TEXT("Experience kept"), Result.Experience, 50.f);
    TestEqual(TEXT("MaxExperience is first curve entry"), Result.MaxExperience, 100.f);
    TestEqual(TEXT("No level gained"), Result.LevelsGained, 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpyLevelExactThresholdTest,
    "SkillProject.Character.Level.ExactThreshold",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpyLevelExactThresholdTest::RunTest(const FString& Parameters)
{
    const USpyLevelConfig* Config = SpyLevelTestHelpers::MakeConfig();

    const FSpyLevelUpResult Result = Config->ResolveLevelUp(1, 100.f);

    TestEqual(TEXT("Level up to 2"), Result.Level, 2);
    TestEqual(TEXT("Experience consumed exactly"), Result.Experience, 0.f);
    TestEqual(TEXT("MaxExperience advances"), Result.MaxExperience, 200.f);
    TestEqual(TEXT("One level gained"), Result.LevelsGained, 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpyLevelMultiLevelTest,
    "SkillProject.Character.Level.MultiLevel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpyLevelMultiLevelTest::RunTest(const FString& Parameters)
{
    const USpyLevelConfig* Config = SpyLevelTestHelpers::MakeConfig();

    //# 350 = 100(→Lv2) + 200(→Lv3) + 잔여 50
    const FSpyLevelUpResult Result = Config->ResolveLevelUp(1, 350.f);

    TestEqual(TEXT("Level up to 3"), Result.Level, 3);
    TestEqual(TEXT("Remainder carried over"), Result.Experience, 50.f);
    TestEqual(TEXT("MaxExperience is third curve entry"), Result.MaxExperience, 300.f);
    TestEqual(TEXT("Two levels gained"), Result.LevelsGained, 2);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpyLevelMaxClampTest,
    "SkillProject.Character.Level.MaxClamp",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpyLevelMaxClampTest::RunTest(const FString& Parameters)
{
    const USpyLevelConfig* Config = SpyLevelTestHelpers::MakeConfig();

    TestEqual(TEXT("Max level is curve size + 1"), Config->GetMaxLevel(), 4);

    const FSpyLevelUpResult Result = Config->ResolveLevelUp(4, 9999.f);

    TestEqual(TEXT("Level stays at max"), Result.Level, 4);
    TestEqual(TEXT("Experience clamped to MaxExperience"), Result.Experience, 300.f);
    TestEqual(TEXT("No level gained"), Result.LevelsGained, 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpyLevelEmptyCurveTest,
    "SkillProject.Character.Level.EmptyCurve",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpyLevelEmptyCurveTest::RunTest(const FString& Parameters)
{
    USpyLevelConfig* Config = NewObject<USpyLevelConfig>();
    Config->ExperienceToNextLevel.Empty();

    const FSpyLevelUpResult Result = Config->ResolveLevelUp(1, 500.f);

    TestEqual(TEXT("Level stays 1 without a curve"), Result.Level, 1);
    TestEqual(TEXT("MaxExperience is zero"), Result.MaxExperience, 0.f);
    TestEqual(TEXT("No level gained"), Result.LevelsGained, 0);

    return true;
}

#endif //# WITH_DEV_AUTOMATION_TESTS
```

- [ ] **Step 2: 테스트가 실패(= 컴파일 실패)하는지 확인**

이 시점엔 `SpyLevelConfig.h` 가 없으므로 **컴파일 에러가 나는 것이 정상**이다. 사용자에게 빌드를 요청해 `Cannot open include file: 'Data/SpyLevelConfig.h'` 를 확인한다. 확인 없이 다음 단계로 넘어가도 되지만, 결과를 지어내지 않는다.

- [ ] **Step 3: `SpyLevelConfig.h` 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "SpyLevelConfig.generated.h"

//# 레벨업 계산 결과 — 부수효과 없는 계산의 출력
USTRUCT(BlueprintType)
struct FSpyLevelUpResult
{
    GENERATED_BODY()

public:
    //# 계산 후 레벨
    UPROPERTY(BlueprintReadOnly)
    int32 Level = 1;

    //# 계산 후 현재 레벨 구간 내 잔여 경험치
    UPROPERTY(BlueprintReadOnly)
    float Experience = 0.f;

    //# 계산 후 다음 레벨까지 필요 경험치
    UPROPERTY(BlueprintReadOnly)
    float MaxExperience = 0.f;

    //# 이번 계산에서 오른 레벨 수 (0 이면 레벨업 없음)
    UPROPERTY(BlueprintReadOnly)
    int32 LevelsGained = 0;
};

UCLASS()
class SKILLPROJECT_API USpyLevelConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    //# 인덱스 i = 레벨 (i+1) → (i+2) 승급에 필요한 경험치. 배열 길이 + 1 이 최대 레벨
    UPROPERTY(EditDefaultsOnly, Category = "Level")
    TArray<float> ExperienceToNextLevel;

    //# 레벨업 1회당 MaxHealth 증가량
    UPROPERTY(EditDefaultsOnly, Category = "Level")
    float MaxHealthPerLevel = 10.f;

    //# 레벨업 1회당 MaxMana 증가량
    UPROPERTY(EditDefaultsOnly, Category = "Level")
    float MaxManaPerLevel = 5.f;

    //# 레벨업 시 Health/Mana 를 최대치로 회복할지
    UPROPERTY(EditDefaultsOnly, Category = "Level")
    bool bFullHealOnLevelUp = true;

    //# 처치 보상 경험치 = 처치당한 대상의 Level × 이 값
    UPROPERTY(EditDefaultsOnly, Category = "Level")
    float ExperienceRewardPerLevel = 20.f;

public:
    //# 커브 길이 + 1. 커브가 비었으면 1
    UFUNCTION(BlueprintPure, Category = "Level")
    int32 GetMaxLevel() const;

    //# InLevel 에서 다음 레벨까지 필요한 경험치. 최대 레벨이면 마지막 커브값을 반환(진행도 바 고정용)
    UFUNCTION(BlueprintPure, Category = "Level")
    float GetExperienceToNextLevel(int32 InLevel) const;

    //# 레벨업 판정 — 부수효과 없음. 한 번에 여러 레벨 상승과 잔여 경험치 이월을 함께 처리한다
    UFUNCTION(BlueprintPure, Category = "Level")
    FSpyLevelUpResult ResolveLevelUp(int32 InLevel, float InExperience) const;
};
```

- [ ] **Step 4: `SpyLevelConfig.cpp` 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/SpyLevelConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyLevelConfig)

int32 USpyLevelConfig::GetMaxLevel() const
{
    return ExperienceToNextLevel.Num() + 1;
}

float USpyLevelConfig::GetExperienceToNextLevel(int32 InLevel) const
{
    if (ExperienceToNextLevel.Num() == 0)
    {
        return 0.f;
    }

    const int32 Index = InLevel - 1;
    if (ExperienceToNextLevel.IsValidIndex(Index))
    {
        return ExperienceToNextLevel[Index];
    }

    //# 최대 레벨 도달 — 마지막 커브값을 유지해 진행도 바를 꽉 찬 상태로 고정한다
    return ExperienceToNextLevel.Last();
}

FSpyLevelUpResult USpyLevelConfig::ResolveLevelUp(int32 InLevel, float InExperience) const
{
    FSpyLevelUpResult Result;
    Result.Level = FMath::Max(1, InLevel);
    Result.Experience = FMath::Max(0.f, InExperience);
    Result.LevelsGained = 0;

    //# 커브 미설정 — 레벨업 판정을 하지 않는다
    if (ExperienceToNextLevel.Num() == 0)
    {
        Result.MaxExperience = 0.f;

        return Result;
    }

    const int32 MaxLevel = GetMaxLevel();
    float Required = GetExperienceToNextLevel(Result.Level);

    //# Required > 0 조건은 커브에 0 이 들어갔을 때 무한 루프를 막는다
    while (Result.Level < MaxLevel && Required > 0.f && Result.Experience >= Required)
    {
        Result.Experience -= Required;
        Result.Level += 1;
        Result.LevelsGained += 1;

        Required = GetExperienceToNextLevel(Result.Level);
    }

    Result.MaxExperience = Required;

    if (Result.Level >= MaxLevel)
    {
        Result.Experience = FMath::Min(Result.Experience, Result.MaxExperience);
    }

    return Result;
}
```

- [ ] **Step 5: 빌드 + 테스트 통과 확인 (사용자 수행)**

사용자에게 요청:
1. Visual Studio 에서 `SkillProject` 빌드 (또는 에디터 Live Coding).
2. 에디터 → `Tools > Session Frontend > Automation` 탭 → `SkillProject.Character.Level` 필터 → 5개 테스트 실행.

기대: `BelowThreshold` / `ExactThreshold` / `MultiLevel` / `MaxClamp` / `EmptyCurve` 전부 PASS.

에디터 없이 돌리려면:
```
UnrealEditor-Cmd.exe <경로>/SkillProject.uproject -ExecCmds="Automation RunTests SkillProject.Character.Level; Quit" -unattended -nopause -testexit="Automation Test Queue Empty" -log
```

- [ ] **Step 6: 스테이징 (커밋하지 않음)**

```bash
git add SkillProject/Source/SkillProject/Data/SpyLevelConfig.h SkillProject/Source/SkillProject/Data/SpyLevelConfig.cpp SkillProject/Source/SkillProject/Character/Tests/SpyLevelTests.cpp
```

커밋 메시지(안): `[Feature] SpyLevelConfig — 레벨 커브 데이터 + 레벨업 계산 함수`

---

## Task 2: AttributeSet — Experience / MaxExperience / Level

**Files:**
- Modify: `SkillProject/Source/SkillProject/Character/SpyCharacterAttributeSet.h`
- Modify: `SkillProject/Source/SkillProject/Character/SpyCharacterAttributeSet.cpp`

**Interfaces:**
- Consumes: 없음
- Produces:
  - `USpyCharacterAttributeSet::GetExperienceAttribute()` / `GetMaxExperienceAttribute()` / `GetLevelAttribute()` (+ `Get*` / `Set*` / `Init*` — `ATTRIBUTE_ACCESSORS` 생성분)
  - `mutable FSKAttributeEvent OnExperienceChanged` / `OnMaxExperienceChanged` / `OnLevelChanged`

**주의 — 기존 버그를 답습하지 말 것:** 현재 `SpyCharacterAttributeSet.h` 의 `MoveNormalSpeed` 는 `ReplicatedUsing = OnRep_Health` 로 잘못 적혀 있다(자기 OnRep 이 아닌 Health 의 OnRep 을 가리킴). **이 Task 의 범위 밖이라 고치지 않는다** — 다만 새로 추가하는 3개는 각자의 `OnRep_` 을 정확히 가리켜야 한다. 발견 사실은 마무리 보고에 한 줄로 남긴다.

- [ ] **Step 1: 헤더에 어트리뷰트 3개 추가**

`SpyCharacterAttributeSet.h` 의 `MoveNormalSpeed` 블록 아래에 추가한다. `protected:` 의 `GetLifetimeReplicatedProps` 선언 아래에 `PostGameplayEffectExecute` 오버라이드도 함께 선언한다.

```cpp
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

public:
	UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData MoveNormalSpeed;
	ATTRIBUTE_ACCESSORS(USpyCharacterAttributeSet, MoveNormalSpeed);

	//# 현재 레벨 구간 내 누적 경험치
	UPROPERTY(ReplicatedUsing = OnRep_Experience, BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Experience;
	ATTRIBUTE_ACCESSORS(USpyCharacterAttributeSet, Experience);

	//# 다음 레벨까지 필요한 경험치 — 클라이언트가 커브를 몰라도 진행도를 계산할 수 있게 복제한다
	UPROPERTY(ReplicatedUsing = OnRep_MaxExperience, BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData MaxExperience;
	ATTRIBUTE_ACCESSORS(USpyCharacterAttributeSet, MaxExperience);

	//# 현재 레벨 (1 시작)
	UPROPERTY(ReplicatedUsing = OnRep_Level, BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Level;
	ATTRIBUTE_ACCESSORS(USpyCharacterAttributeSet, Level);

protected:
	UFUNCTION()
	void OnRep_MoveNormalSpeed(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Experience(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxExperience(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Level(const FGameplayAttributeData& OldValue);

public:
	mutable FSKAttributeEvent OnMoveNormalSpeedChanged;
	mutable FSKAttributeEvent OnExperienceChanged;
	mutable FSKAttributeEvent OnMaxExperienceChanged;
	mutable FSKAttributeEvent OnLevelChanged;
```

- [ ] **Step 2: `.cpp` 에 레플리케이션 등록 + OnRep + PostGameplayEffectExecute 구현**

`SpyCharacterAttributeSet.cpp` 전체를 아래로 교체한다.

```cpp
// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyCharacterAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

void USpyCharacterAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USpyCharacterAttributeSet, MoveNormalSpeed);
	DOREPLIFETIME(USpyCharacterAttributeSet, Experience);
	DOREPLIFETIME(USpyCharacterAttributeSet, MaxExperience);
	DOREPLIFETIME(USpyCharacterAttributeSet, Level);
}

void USpyCharacterAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	AActor* InstigatorActor = Data.EffectSpec.GetContext().GetInstigator();
	AActor* EffectCauser = Data.EffectSpec.GetContext().GetEffectCauser();
	const float DeltaValue = Data.EvaluatedData.Magnitude;

	//# 경험치는 음수로 내려가지 않는다
	if (Data.EvaluatedData.Attribute == GetExperienceAttribute())
	{
		SetExperience(FMath::Max(0.f, GetExperience()));

		//# 서버에서 브로드캐스트 (클라이언트는 OnRep_Experience에서 처리)
		const float NewExperience = GetExperience();
		OnExperienceChanged.Broadcast(InstigatorActor, EffectCauser, &Data.EffectSpec, DeltaValue, NewExperience - DeltaValue, NewExperience);
	}
	else if (Data.EvaluatedData.Attribute == GetMaxExperienceAttribute())
	{
		const float NewMaxExperience = GetMaxExperience();
		OnMaxExperienceChanged.Broadcast(InstigatorActor, EffectCauser, &Data.EffectSpec, DeltaValue, NewMaxExperience - DeltaValue, NewMaxExperience);
	}
	else if (Data.EvaluatedData.Attribute == GetLevelAttribute())
	{
		const float NewLevel = GetLevel();
		OnLevelChanged.Broadcast(InstigatorActor, EffectCauser, &Data.EffectSpec, DeltaValue, NewLevel - DeltaValue, NewLevel);
	}
}

void USpyCharacterAttributeSet::OnRep_MoveNormalSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USpyCharacterAttributeSet, MoveNormalSpeed, OldValue);

	OnMoveNormalSpeedChanged.Broadcast(nullptr, nullptr, nullptr, GetMoveNormalSpeed() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetMoveNormalSpeed());
}

void USpyCharacterAttributeSet::OnRep_Experience(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USpyCharacterAttributeSet, Experience, OldValue);

	OnExperienceChanged.Broadcast(nullptr, nullptr, nullptr, GetExperience() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetExperience());
}

void USpyCharacterAttributeSet::OnRep_MaxExperience(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USpyCharacterAttributeSet, MaxExperience, OldValue);

	OnMaxExperienceChanged.Broadcast(nullptr, nullptr, nullptr, GetMaxExperience() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetMaxExperience());
}

void USpyCharacterAttributeSet::OnRep_Level(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USpyCharacterAttributeSet, Level, OldValue);

	OnLevelChanged.Broadcast(nullptr, nullptr, nullptr, GetLevel() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetLevel());
}
```

**참고**: 기존 `.cpp` 에는 `UE_INLINE_GENERATED_CPP_BY_NAME` 이 없다. 파일의 기존 상태를 유지한다(추가하지 않는다) — 이 Task 는 어트리뷰트 추가만 다룬다.

- [ ] **Step 3: 빌드 확인 (사용자 수행)**

컴파일만 통과하면 된다. Task 1 의 5개 테스트도 계속 PASS 인지 확인한다 (회귀 없음).

- [ ] **Step 4: 스테이징**

```bash
git add SkillProject/Source/SkillProject/Character/SpyCharacterAttributeSet.h SkillProject/Source/SkillProject/Character/SpyCharacterAttributeSet.cpp
```

커밋 메시지(안): `[Feature] SpyCharacterAttributeSet — 경험치/레벨 어트리뷰트 추가`

---

## Task 3: 태그 + GameplayEffect 클래스

**Files:**
- Modify: `SkillProject/Source/SkillProject/Util/SpyGameplayTags.h`
- Modify: `SkillProject/Source/SkillProject/Util/SpyGameplayTags.cpp`
- Create: `SkillProject/Source/SkillProject/AbilitySystem/Effect/SpyGE_ExperienceGain.h|.cpp`
- Create: `SkillProject/Source/SkillProject/AbilitySystem/Effect/SpyGE_LevelGrowth.h|.cpp`

**Interfaces:**
- Consumes: Task 2 의 `GetExperienceAttribute()` / `GetMaxHealthAttribute()` / `GetMaxManaAttribute()`
- Produces:
  - `SpyGameplayTags::Data_Experience_Gain` / `Data_Level_MaxHealthGrowth` / `Data_Level_MaxManaGrowth`
  - `USpyGE_ExperienceGain` / `USpyGE_LevelGrowth` (둘 다 Instant, SetByCaller 매그니튜드)

- [ ] **Step 1: 태그 등록**

`SpyGameplayTags.h` 의 `namespace SpyGameplayTags` 안에 추가:

```cpp
	//# SetByCaller 데이터 태그
	SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Experience_Gain);
	SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Level_MaxHealthGrowth);
	SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Level_MaxManaGrowth);
```

`SpyGameplayTags.cpp` 에 정의 추가 (기존 파일의 `UE_DEFINE_GAMEPLAY_TAG` 나열 스타일·태그 문자열 컨벤션을 먼저 읽고 맞출 것):

```cpp
	UE_DEFINE_GAMEPLAY_TAG(Data_Experience_Gain, "Data.Experience.Gain");
	UE_DEFINE_GAMEPLAY_TAG(Data_Level_MaxHealthGrowth, "Data.Level.MaxHealthGrowth");
	UE_DEFINE_GAMEPLAY_TAG(Data_Level_MaxManaGrowth, "Data.Level.MaxManaGrowth");
```

- [ ] **Step 2: ⚠ 엔진 API 확인 — `UGameplayEffect::Modifiers` 접근성**

**구현 전 반드시 확인한다.** UE 5.3 부터 `UGameplayEffect` 의 상당수 프로퍼티가 `private` 으로 바뀌고 접근자(`GetModifiers()` 등)가 생겼다. UE 5.7 설치본의 `Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Public/GameplayEffect.h` 에서 `Modifiers` 선언부를 직접 확인한다.

- **생성자에서 `Modifiers.Add(...)` 가 가능하면** → Step 3 의 코드를 그대로 쓴다.
- **`private` 이라 서브클래스 생성자에서 접근 불가하면** → **폴백**: GE 클래스를 만들지 않고 `USpyLevelComponent` 에서 서버 권한으로 `UAbilitySystemComponent::ApplyModToAttribute(Attribute, EGameplayModOp::Additive, Delta)` 를 직접 호출한다. 이 경우 `PostGameplayEffectExecute` 가 발화하지 않으므로, **Task 4 의 컴포넌트가 어트리뷰트 변경 후 `OnExperienceChanged` 상당의 자체 델리게이트를 직접 브로드캐스트해야 한다** (Task 4 Step 4 에 해당 분기 명시).

어느 쪽을 택했는지 마무리 보고에 반드시 남긴다.

- [ ] **Step 3: `SpyGE_ExperienceGain` 작성**

`SpyGE_ExperienceGain.h`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"

#include "SpyGE_ExperienceGain.generated.h"

//# 경험치 가산 Instant 이펙트 — 매그니튜드는 SetByCaller(Data.Experience.Gain)로 전달한다
UCLASS()
class SKILLPROJECT_API USpyGE_ExperienceGain : public UGameplayEffect
{
	GENERATED_BODY()

public:
	USpyGE_ExperienceGain();
};
```

`SpyGE_ExperienceGain.cpp`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Effect/SpyGE_ExperienceGain.h"
#include "Character/SpyCharacterAttributeSet.h"
#include "Util/SpyGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGE_ExperienceGain)

USpyGE_ExperienceGain::USpyGE_ExperienceGain()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = SpyGameplayTags::Data_Experience_Gain;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = USpyCharacterAttributeSet::GetExperienceAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	Modifiers.Add(Modifier);
}
```

- [ ] **Step 4: `SpyGE_LevelGrowth` 작성**

`SpyGE_LevelGrowth.h`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"

#include "SpyGE_LevelGrowth.generated.h"

//# 레벨업 성장 Instant 이펙트 — MaxHealth/MaxMana 증가량을 SetByCaller로 전달한다
UCLASS()
class SKILLPROJECT_API USpyGE_LevelGrowth : public UGameplayEffect
{
	GENERATED_BODY()

public:
	USpyGE_LevelGrowth();
};
```

`SpyGE_LevelGrowth.cpp`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Effect/SpyGE_LevelGrowth.h"
#include "Character/SpyCharacterAttributeSet.h"
#include "Util/SpyGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGE_LevelGrowth)

USpyGE_LevelGrowth::USpyGE_LevelGrowth()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat HealthSetByCaller;
	HealthSetByCaller.DataTag = SpyGameplayTags::Data_Level_MaxHealthGrowth;

	FGameplayModifierInfo MaxHealthModifier;
	MaxHealthModifier.Attribute = USpyCharacterAttributeSet::GetMaxHealthAttribute();
	MaxHealthModifier.ModifierOp = EGameplayModOp::Additive;
	MaxHealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(HealthSetByCaller);

	Modifiers.Add(MaxHealthModifier);

	FSetByCallerFloat ManaSetByCaller;
	ManaSetByCaller.DataTag = SpyGameplayTags::Data_Level_MaxManaGrowth;

	FGameplayModifierInfo MaxManaModifier;
	MaxManaModifier.Attribute = USpyCharacterAttributeSet::GetMaxManaAttribute();
	MaxManaModifier.ModifierOp = EGameplayModOp::Additive;
	MaxManaModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(ManaSetByCaller);

	Modifiers.Add(MaxManaModifier);
}
```

- [ ] **Step 5: 빌드 확인 (사용자 수행)**

컴파일 통과 + 에디터 실행 시 태그가 프로젝트 세팅 > GameplayTags 목록에 `Data.Experience.Gain` 등으로 보이는지 확인.

- [ ] **Step 6: 스테이징**

```bash
git add SkillProject/Source/SkillProject/Util/SpyGameplayTags.h SkillProject/Source/SkillProject/Util/SpyGameplayTags.cpp SkillProject/Source/SkillProject/AbilitySystem/Effect/
```

커밋 메시지(안): `[Feature] SpyGE_ExperienceGain — 경험치/성장 이펙트 + SetByCaller 태그`

---

## Task 4: SpyLevelComponent — 경험치 지급 · 레벨업 적용

이 Task 가 시스템의 심장이다. 서버 권한, 재진입 가드, 중복 지급 방지가 전부 여기 있다.

**Files:**
- Create: `SkillProject/Source/SkillProject/Character/SpyLevelComponent.h`
- Create: `SkillProject/Source/SkillProject/Character/SpyLevelComponent.cpp`
- Modify: `SkillProject/Source/SkillProject/Character/SpyCharacter.h`
- Modify: `SkillProject/Source/SkillProject/Character/SpyCharacter.cpp` (생성자 ~78-80행, `OnAbilitySystemInitialized` / `OnAbilitySystemUninitialized`)

**Interfaces:**
- Consumes: Task 1 `ResolveLevelUp`/`GetExperienceToNextLevel`, Task 2 어트리뷰트 접근자·`OnExperienceChanged`, Task 3 GE 클래스·태그
- Produces:
  - `USpyLevelComponent::FindLevelComponent(const AActor*)`
  - `InitializeByAbilitySystem(USpyAbilitySystemComponent*)` / `UnInitializeByAbilitySystem()`
  - `GetLevel()` / `GetExperience()` / `GetMaxExperience()` / `GetExperienceNormalized()`
  - `FSpyLevel_ExperienceChanged OnExperienceChanged(USpyLevelComponent*, float OldValue, float NewValue)`
  - `FSpyLevel_LevelChanged OnLevelChanged(USpyLevelComponent*, int32 OldLevel, int32 NewLevel)`

- [ ] **Step 1: `SpyLevelComponent.h` 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameFrameworkComponent.h"

#include "SpyLevelComponent.generated.h"

class USpyAbilitySystemComponent;
class USpyCharacterAttributeSet;
class USpyLevelConfig;
struct FGameplayEffectSpec;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSpyLevel_ExperienceChanged, USpyLevelComponent*, LevelComponent, float, OldValue, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSpyLevel_LevelChanged, USpyLevelComponent*, LevelComponent, int32, OldLevel, int32, NewLevel);

UCLASS()
class SKILLPROJECT_API USpyLevelComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure)
	static USpyLevelComponent* FindLevelComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<USpyLevelComponent>() : nullptr); }

	void InitializeByAbilitySystem(USpyAbilitySystemComponent* InASC);
	void UnInitializeByAbilitySystem();

	UFUNCTION(BlueprintPure)
	int32 GetLevel() const;

	UFUNCTION(BlueprintPure)
	float GetExperience() const;

	UFUNCTION(BlueprintPure)
	float GetMaxExperience() const;

	//# 진행도 0~1. MaxExperience가 0 이하면 0
	UFUNCTION(BlueprintPure)
	float GetExperienceNormalized() const;

protected:
	virtual void OnUnregister() override;

	//# AttributeSet 델리게이트 수신
	void HandleExperienceChanged(AActor* Instigator, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue);

	//# 자기 캐릭터 사망 — 서버에서 킬러에게 경험치를 지급한다
	UFUNCTION()
	void HandleDeath(AActor* OwningActor, AActor* CauserActor);

	//# 서버 전용 — 레벨업 판정 후 어트리뷰트·성장 이펙트 적용
	void TryLevelUp();

	//# 킬러 액터에서 ASC를 해석한다 (PlayerState로 넘어온 경우 Pawn 폴백)
	USpyAbilitySystemComponent* ResolveKillerAbilitySystem(AActor* InInstigator) const;

public:
	UPROPERTY(BlueprintAssignable)
	FSpyLevel_ExperienceChanged OnExperienceChanged;

	UPROPERTY(BlueprintAssignable)
	FSpyLevel_LevelChanged OnLevelChanged;

protected:
	//# 캐릭터 BP 기본값에서 지정한다 (SpyMovementConfig 선례와 동일)
	UPROPERTY(EditDefaultsOnly, Category = "Level")
	TObjectPtr<USpyLevelConfig> LevelConfig;

	UPROPERTY()
	TObjectPtr<USpyAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<const USpyCharacterAttributeSet> LevelSet;

	//# 레벨업 처리 중 어트리뷰트 변경으로 재진입하는 것을 막는다
	bool bProcessingLevelUp = false;

	//# 사망 보상 1회 지급 보장 (Health가 0 이하로 여러 번 갱신될 수 있음)
	bool bDeathRewardGranted = false;
};
```

- [ ] **Step 2: `SpyLevelComponent.cpp` — 초기화 / 조회 / 정리**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/SpyLevelComponent.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "AbilitySystem/Effect/SpyGE_ExperienceGain.h"
#include "AbilitySystem/Effect/SpyGE_LevelGrowth.h"
#include "Character/SpyCharacterAttributeSet.h"
#include "Character/SpyHealthComponent.h"
#include "Data/SpyLevelConfig.h"
#include "Util/SpyGameplayTags.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyLevelComponent)

void USpyLevelComponent::InitializeByAbilitySystem(USpyAbilitySystemComponent* InASC)
{
	AActor* Owner = GetOwner();
	check(Owner);

	if (InASC == nullptr)
		return;

	AbilitySystemComponent = InASC;
	LevelSet = AbilitySystemComponent->GetSet<USpyCharacterAttributeSet>();

	if (LevelSet == nullptr)
		return;

	LevelSet->OnExperienceChanged.AddUObject(this, &ThisClass::HandleExperienceChanged);

	//# 사망 보상은 자기 캐릭터의 HealthComponent 사망 이벤트에서 출발한다
	if (USpyHealthComponent* HealthComponent = USpyHealthComponent::FindHealthComponent(Owner))
	{
		HealthComponent->OnDeath.AddDynamic(this, &ThisClass::HandleDeath);
	}

	//# 초기 상태는 서버에서만 세팅한다 (클라이언트는 복제로 받는다)
	if (Owner->HasAuthority() && LevelConfig)
	{
		AbilitySystemComponent->SetNumericAttributeBase(USpyCharacterAttributeSet::GetLevelAttribute(), 1.f);
		AbilitySystemComponent->SetNumericAttributeBase(USpyCharacterAttributeSet::GetExperienceAttribute(), 0.f);
		AbilitySystemComponent->SetNumericAttributeBase(USpyCharacterAttributeSet::GetMaxExperienceAttribute(), LevelConfig->GetExperienceToNextLevel(1));
	}

	bDeathRewardGranted = false;
}

void USpyLevelComponent::UnInitializeByAbilitySystem()
{
	if (LevelSet)
	{
		LevelSet->OnExperienceChanged.RemoveAll(this);
	}

	if (USpyHealthComponent* HealthComponent = USpyHealthComponent::FindHealthComponent(GetOwner()))
	{
		HealthComponent->OnDeath.RemoveDynamic(this, &ThisClass::HandleDeath);
	}

	LevelSet = nullptr;
	AbilitySystemComponent = nullptr;
}

void USpyLevelComponent::OnUnregister()
{
	UnInitializeByAbilitySystem();

	Super::OnUnregister();
}

int32 USpyLevelComponent::GetLevel() const
{
	return (LevelSet ? FMath::FloorToInt(LevelSet->GetLevel()) : 1);
}

float USpyLevelComponent::GetExperience() const
{
	return (LevelSet ? LevelSet->GetExperience() : 0.f);
}

float USpyLevelComponent::GetMaxExperience() const
{
	return (LevelSet ? LevelSet->GetMaxExperience() : 0.f);
}

float USpyLevelComponent::GetExperienceNormalized() const
{
	const float MaxExperience = GetMaxExperience();

	return ((MaxExperience > 0.f) ? FMath::Clamp(GetExperience() / MaxExperience, 0.f, 1.f) : 0.f);
}
```

**어트리뷰트 직접 세팅 방식**: `GetSet<T>()` 는 `const T*` 를 돌려주므로 `LevelSet->Set*()` 를 직접 부르려면 `const_cast` 가 필요하다. 대신 ASC 의 `SetNumericAttributeBase(FGameplayAttribute, float)` 를 쓴다 — `const_cast` 없이 BaseValue 를 세팅하고 레플리케이션도 정상 동작한다. **이 플랜의 모든 어트리뷰트 직접 세팅은 `SetNumericAttributeBase` 로 통일한다.**

- [ ] **Step 3: `.cpp` — 사망 보상 지급**

```cpp
USpyAbilitySystemComponent* USpyLevelComponent::ResolveKillerAbilitySystem(AActor* InInstigator) const
{
	if (InInstigator == nullptr)
		return nullptr;

	//# Instigator가 PlayerState로 넘어오는 경우가 있어 Pawn으로 폴백한다
	//# (SpyHealthComponent::HandleHealthChanged의 기존 폴백과 동일한 방식)
	AActor* KillerActor = InInstigator;
	if (APlayerState* PlayerState = Cast<APlayerState>(InInstigator))
	{
		if (APawn* KillerPawn = PlayerState->GetPawn())
		{
			KillerActor = KillerPawn;
		}
	}

	//# 자기 자신을 죽인 경우(자살·환경 피해)는 보상 없음
	if (KillerActor == GetOwner())
		return nullptr;

	return Cast<USpyAbilitySystemComponent>(
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(KillerActor));
}

void USpyLevelComponent::HandleDeath(AActor* OwningActor, AActor* CauserActor)
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr || Owner->HasAuthority() == false)
		return;

	//# OnDeath는 Health가 0 이하로 여러 번 갱신되면 반복 발화할 수 있다 — 1회만 지급
	if (bDeathRewardGranted)
		return;

	if (LevelConfig == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("# [SpyLevelComponent] LevelConfig가 지정되지 않아 경험치를 지급하지 않습니다: %s"), *GetNameSafe(Owner));
		return;
	}

	//# OnDeath의 첫 인자는 DamageInstigator다 (SpyHealthComponent::HandleHealthChanged 참고)
	USpyAbilitySystemComponent* KillerASC = ResolveKillerAbilitySystem(OwningActor);
	if (KillerASC == nullptr)
		return;

	bDeathRewardGranted = true;

	//# 보상 = 처치당한 쪽(자신)의 레벨 × 계수
	const float RewardAmount = GetLevel() * LevelConfig->ExperienceRewardPerLevel;
	if (RewardAmount <= 0.f)
		return;

	FGameplayEffectContextHandle ContextHandle = KillerASC->MakeEffectContext();
	ContextHandle.AddSourceObject(Owner);

	const FGameplayEffectSpecHandle SpecHandle = KillerASC->MakeOutgoingSpec(USpyGE_ExperienceGain::StaticClass(), 1.f, ContextHandle);
	if (SpecHandle.IsValid() == false)
		return;

	SpecHandle.Data->SetSetByCallerMagnitude(SpyGameplayTags::Data_Experience_Gain, RewardAmount);
	KillerASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}
```

`.cpp` 상단 include 에 `#include "AbilitySystemBlueprintLibrary.h"` 를 추가한다.

**Task 3 Step 2 에서 폴백을 택했다면** 위 마지막 4줄을 아래로 대체한다:

```cpp
	//# 폴백 — GE 없이 서버에서 직접 어트리뷰트 가산
	KillerASC->ApplyModToAttribute(USpyCharacterAttributeSet::GetExperienceAttribute(), EGameplayModOp::Additive, RewardAmount);

	//# ApplyModToAttribute는 PostGameplayEffectExecute를 발화시키지 않으므로 직접 레벨업 판정을 돌린다
	if (USpyLevelComponent* KillerLevelComponent = USpyLevelComponent::FindLevelComponent(KillerASC->GetAvatarActor()))
	{
		KillerLevelComponent->TryLevelUp();
	}
```

폴백을 쓰면 `TryLevelUp()` 을 `public` 으로 올린다.

- [ ] **Step 4: `.cpp` — 레벨업 판정**

```cpp
void USpyLevelComponent::HandleExperienceChanged(AActor* Instigator, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue)
{
	//# UI·연출용 브로드캐스트는 서버/클라이언트 양쪽에서 발생한다
	OnExperienceChanged.Broadcast(this, OldValue, NewValue);

	AActor* Owner = GetOwner();
	if (Owner == nullptr || Owner->HasAuthority() == false)
		return;

	TryLevelUp();
}

void USpyLevelComponent::TryLevelUp()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr || Owner->HasAuthority() == false)
		return;

	//# 루프 안에서 어트리뷰트를 바꾸면 HandleExperienceChanged가 재진입한다
	if (bProcessingLevelUp)
		return;

	if (LevelConfig == nullptr || LevelSet == nullptr || AbilitySystemComponent == nullptr)
		return;

	if (LevelConfig->ExperienceToNextLevel.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("# [SpyLevelComponent] ExperienceToNextLevel 커브가 비어 레벨업을 건너뜁니다: %s"), *GetNameSafe(Owner));
		return;
	}

	const int32 OldLevel = GetLevel();
	const FSpyLevelUpResult Result = LevelConfig->ResolveLevelUp(OldLevel, GetExperience());

	//# 레벨업이 없어도 최대 레벨 클램프로 경험치가 조정될 수 있다
	const bool bExperienceChanged = FMath::IsNearlyEqual(Result.Experience, GetExperience()) == false;
	const bool bMaxExperienceChanged = FMath::IsNearlyEqual(Result.MaxExperience, GetMaxExperience()) == false;

	if (Result.LevelsGained == 0 && bExperienceChanged == false && bMaxExperienceChanged == false)
		return;

	TGuardValue<bool> ReentryGuard(bProcessingLevelUp, true);

	AbilitySystemComponent->SetNumericAttributeBase(USpyCharacterAttributeSet::GetLevelAttribute(), static_cast<float>(Result.Level));
	AbilitySystemComponent->SetNumericAttributeBase(USpyCharacterAttributeSet::GetExperienceAttribute(), Result.Experience);
	AbilitySystemComponent->SetNumericAttributeBase(USpyCharacterAttributeSet::GetMaxExperienceAttribute(), Result.MaxExperience);

	if (Result.LevelsGained > 0)
	{
		//# 성장 이펙트 — 오른 레벨 수만큼 곱해 한 번에 적용
		FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
		ContextHandle.AddSourceObject(Owner);

		const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(USpyGE_LevelGrowth::StaticClass(), 1.f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(SpyGameplayTags::Data_Level_MaxHealthGrowth, LevelConfig->MaxHealthPerLevel * Result.LevelsGained);
			SpecHandle.Data->SetSetByCallerMagnitude(SpyGameplayTags::Data_Level_MaxManaGrowth, LevelConfig->MaxManaPerLevel * Result.LevelsGained);

			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
		}

		if (LevelConfig->bFullHealOnLevelUp)
		{
			//# 성장 이펙트 적용 뒤이므로 MaxHealth/MaxMana는 이미 증가한 값이다
			AbilitySystemComponent->SetNumericAttributeBase(USpyCharacterAttributeSet::GetHealthAttribute(), LevelSet->GetMaxHealth());
			AbilitySystemComponent->SetNumericAttributeBase(USpyCharacterAttributeSet::GetManaAttribute(), LevelSet->GetMaxMana());
		}

		OnLevelChanged.Broadcast(this, OldLevel, Result.Level);

		UE_LOG(LogTemp, Log, TEXT("# [SpyLevelComponent] LevelUp %s: %d -> %d"), *GetNameSafe(Owner), OldLevel, Result.Level);
	}
}
```

**Task 3 폴백을 택했다면** 성장 이펙트 블록을 아래로 대체한다:

```cpp
		AbilitySystemComponent->ApplyModToAttribute(USpyCharacterAttributeSet::GetMaxHealthAttribute(), EGameplayModOp::Additive, LevelConfig->MaxHealthPerLevel * Result.LevelsGained);
		AbilitySystemComponent->ApplyModToAttribute(USpyCharacterAttributeSet::GetMaxManaAttribute(), EGameplayModOp::Additive, LevelConfig->MaxManaPerLevel * Result.LevelsGained);
```

- [ ] **Step 5: `SpyCharacter` 에 컴포넌트 연결**

`SpyCharacter.h` — `SpyHealthComponent` 선언 근처에 추가:

```cpp
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spy|Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USpyLevelComponent> SpyLevelComponent;
```

(가시성 지정자·`meta` 는 기존 `SpyHealthComponent` 선언과 정확히 맞춘다 — 헤더를 먼저 읽고 동일하게 쓸 것.)

`SpyCharacter.cpp` 생성자, `SpyHealthComponent` 생성 바로 아래 (현재 80행 근처):

```cpp
	SpyLevelComponent = CreateDefaultSubobject<USpyLevelComponent>(TEXT("LevelComponent"));
```

`OnAbilitySystemInitialized()` — `SpyHealthComponent->InitializeByAbilitySystem(SpyASC);` 바로 아래:

```cpp
	SpyLevelComponent->InitializeByAbilitySystem(SpyASC);
```

`OnAbilitySystemUninitialized()` — `SpyHealthComponent->UnInitializeByAbilitySystem();` 바로 아래:

```cpp
	SpyLevelComponent->UnInitializeByAbilitySystem();
```

`SpyCharacter.cpp` include 에 `#include "Character/SpyLevelComponent.h"` 추가.

**초기화 순서 주의**: `InitializeByAbilitySystem` 안에서 `FindHealthComponent` 로 HealthComponent 를 찾아 `OnDeath` 를 구독한다. HealthComponent 는 생성자에서 만들어지므로 이 시점에 반드시 존재한다.

- [ ] **Step 6: 빌드 + 인게임 확인 (사용자 수행)**

1. 빌드.
2. 에디터에서 `DA_SpyLevelConfig` 를 만들고 `ExperienceToNextLevel = [100, 200, 300]` 입력.
3. 캐릭터 BP → `LevelComponent` → `LevelConfig` 에 위 DataAsset 지정.
4. PIE 로 봇을 처치하고 출력 로그에서 `# [SpyLevelComponent] LevelUp` 이 찍히는지 확인.

- [ ] **Step 7: 스테이징**

```bash
git add SkillProject/Source/SkillProject/Character/SpyLevelComponent.h SkillProject/Source/SkillProject/Character/SpyLevelComponent.cpp SkillProject/Source/SkillProject/Character/SpyCharacter.h SkillProject/Source/SkillProject/Character/SpyCharacter.cpp
```

커밋 메시지(안): `[Feature] SpyLevelComponent — 처치 경험치 지급 + 레벨업 성장 적용`

---

## Task 5: MainHUD 표시

**Files:**
- Modify: `SkillProject/Source/SkillProject/UI/SpyMainHUD.h`
- Modify: `SkillProject/Source/SkillProject/UI/SpyMainHUD.cpp`

**Interfaces:**
- Consumes: Task 4 의 `USpyLevelComponent` 델리게이트·조회 함수
- Produces: 없음 (말단)

**핵심 리스크**: 클라이언트에서는 `NativeConstruct` 시점에 PlayerState/ASC 가 아직 도착하지 않았을 수 있다. 구축 시점에 어트리뷰트가 있다고 가정하면 "리슨 서버에서는 되는데 클라이언트에서는 빈칸" 버그가 난다. 반드시 **폴링이 아니라 준비 신호**에 붙는다.

- [ ] **Step 1: 헤더 확장**

```cpp
class UProgressBar;
class UButton;
class UTextBlock;
class USpyLevelComponent;

UCLASS()
class SKILLPROJECT_API USpyMainHUD : public USpyUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UButton> Btn_Menu;

	//# 아직 WBP에 배치되지 않았을 수 있어 Optional로 바인딩한다
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> PB_Exp;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Level;

protected:
	UFUNCTION(BlueprintCallable)
	void ShowMenu();

	//# 로컬 폰의 LevelComponent를 찾아 구독한다. 아직 없으면 false
	bool TryBindLevelComponent();

	void UnbindLevelComponent();

	UFUNCTION()
	void HandleExperienceChanged(USpyLevelComponent* InLevelComponent, float InOldValue, float InNewValue);

	UFUNCTION()
	void HandleLevelChanged(USpyLevelComponent* InLevelComponent, int32 InOldLevel, int32 InNewLevel);

	void RefreshAll();

protected:
	UPROPERTY()
	TObjectPtr<USpyLevelComponent> BoundLevelComponent;

	FTimerHandle BindRetryTimerHandle;
};
```

- [ ] **Step 2: `.cpp` 구현**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SpyMainHUD.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Character/SpyLevelComponent.h"
#include "Manager/SpyUIManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyMainHUD)

void USpyMainHUD::NativeConstruct()
{
    Super::NativeConstruct();

    if (Btn_Menu)
    {
        Btn_Menu->OnClicked.AddDynamic(this, &USpyMainHUD::ShowMenu);
    }

    //# 클라이언트에서는 이 시점에 Pawn/PlayerState/ASC가 아직 없을 수 있다.
    //# 준비될 때까지 짧은 주기로 재시도하고, 성공하면 타이머를 끈다.
    if (TryBindLevelComponent() == false)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(BindRetryTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
            {
                if (TryBindLevelComponent())
                {
                    if (UWorld* InnerWorld = GetWorld())
                    {
                        InnerWorld->GetTimerManager().ClearTimer(BindRetryTimerHandle);
                    }
                }
            }), 0.25f, true);
        }
    }
}

void USpyMainHUD::NativeDestruct()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BindRetryTimerHandle);
    }

    UnbindLevelComponent();

    Super::NativeDestruct();

    if (Btn_Menu)
    {
        Btn_Menu->OnClicked.Clear();
    }
}

void USpyMainHUD::ShowMenu()
{
    USpyUIManager::Get(this)->OpenSpyUI(ESpyUIType::Menu);
}

bool USpyMainHUD::TryBindLevelComponent()
{
    if (BoundLevelComponent)
        return true;

    APlayerController* OwningController = GetOwningPlayer();
    if (OwningController == nullptr)
        return false;

    APawn* OwningPawn = OwningController->GetPawn();
    if (OwningPawn == nullptr)
        return false;

    USpyLevelComponent* LevelComponent = USpyLevelComponent::FindLevelComponent(OwningPawn);
    if (LevelComponent == nullptr)
        return false;

    //# MaxExperience가 아직 복제되지 않았으면 어트리뷰트 준비 전이다 — 다음 주기에 다시 시도
    if (LevelComponent->GetMaxExperience() <= 0.f)
        return false;

    BoundLevelComponent = LevelComponent;

    BoundLevelComponent->OnExperienceChanged.AddDynamic(this, &USpyMainHUD::HandleExperienceChanged);
    BoundLevelComponent->OnLevelChanged.AddDynamic(this, &USpyMainHUD::HandleLevelChanged);

    //# 구독 전에 이미 변한 값을 놓치지 않도록 즉시 1회 갱신
    RefreshAll();

    return true;
}

void USpyMainHUD::UnbindLevelComponent()
{
    if (BoundLevelComponent)
    {
        BoundLevelComponent->OnExperienceChanged.RemoveDynamic(this, &USpyMainHUD::HandleExperienceChanged);
        BoundLevelComponent->OnLevelChanged.RemoveDynamic(this, &USpyMainHUD::HandleLevelChanged);
    }

    BoundLevelComponent = nullptr;
}

void USpyMainHUD::HandleExperienceChanged(USpyLevelComponent* InLevelComponent, float InOldValue, float InNewValue)
{
    RefreshAll();
}

void USpyMainHUD::HandleLevelChanged(USpyLevelComponent* InLevelComponent, int32 InOldLevel, int32 InNewLevel)
{
    RefreshAll();
}

void USpyMainHUD::RefreshAll()
{
    if (BoundLevelComponent == nullptr)
        return;

    if (PB_Exp)
    {
        //# 0 나눗셈 방어는 USpyHPBar::UpdateHP와 동일한 처리
        PB_Exp->SetPercent(BoundLevelComponent->GetExperienceNormalized());
    }

    if (Txt_Level)
    {
        Txt_Level->SetText(FText::Format(NSLOCTEXT("SpyMainHUD", "LevelFormat", "Lv.{0}"), FText::AsNumber(BoundLevelComponent->GetLevel())));
    }
}
```

**설계 노트 — 왜 타이머 재시도인가**: spec 초안은 `UGameFrameworkComponentManager` 확장 핸들러(`NAME_AbilityReady`)를 쓰기로 했다. 그 경로가 이 위젯에서 깔끔하게 잡히면 그쪽이 더 낫다 — 폴링이 없기 때문이다. 다만 확장 핸들러는 **액터 단위** 등록이라 위젯이 Pawn 을 이미 알고 있어야 하는데, 클라이언트에서는 위젯 생성 시점에 Pawn 자체가 없을 수 있어 닭-달걀 문제가 생긴다. 구현자가 `ASpyPlayerController` 의 `OnPossessedPawnChanged` 등 더 나은 신호를 찾으면 타이머를 그것으로 교체한다. **어느 쪽이든 0.25초 폴링이 영구히 도는 일은 없어야 한다** — 바인딩 성공 시 반드시 타이머를 끈다.

- [ ] **Step 3: 위젯 배치 (사용자 수행)**

`WBP_MainHUD` 에 `PB_Exp`(ProgressBar) 와 `Txt_Level`(TextBlock) 을 **정확한 이름으로** 배치한다. `BindWidgetOptional` 이라 배치하지 않아도 컴파일·실행은 되지만 표시는 되지 않는다.

- [ ] **Step 4: 인게임 확인 (사용자 수행)**

1. PIE(리슨 서버 + 클라이언트 2) 로 실행.
2. 봇 처치 → 경험치 바 증가 확인.
3. 레벨업 시 `Lv.` 숫자 증가 + 체력 최대치 상승 + 풀회복 확인.
4. **클라이언트 창에서도 동일하게 보이는지 확인** (이 Task 의 핵심 검증 포인트).

- [ ] **Step 5: 스테이징**

```bash
git add SkillProject/Source/SkillProject/UI/SpyMainHUD.h SkillProject/Source/SkillProject/UI/SpyMainHUD.cpp
```

커밋 메시지(안): `[Feature] SpyMainHUD — 경험치 바/레벨 표시 추가`

---

## 완료 조건

- [ ] `SkillProject.Character.Level` 5개 테스트 PASS
- [ ] 봇 처치 시 킬러의 경험치가 오른다 (서버 로그 확인)
- [ ] 임계치 도달 시 레벨업 + `MaxHealth`/`MaxMana` 상승 + 풀회복
- [ ] 최대 레벨에서 추가 처치해도 크래시·무한 루프 없음
- [ ] MainHUD 경험치 바·레벨 텍스트가 **리슨 서버와 클라이언트 양쪽에서** 갱신됨
- [ ] `LevelConfig` 미지정 캐릭터에서도 크래시 없이 경고 로그만 남음
