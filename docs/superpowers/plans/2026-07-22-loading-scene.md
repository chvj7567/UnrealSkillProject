# 로딩 씬 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 게임 부팅 시 전용 로딩 레벨에서 로딩바 + 퍼센트를 표시하고, 2단계(에셋 프리로드 → 맵 비동기 스트리밍) 진행률이 100% 이며 최소 표시 시간이 지나면 게임플레이 맵으로 자동 전환한다.

**Architecture:** 진행률 로직은 전부 `USpyLoadingSubsystem`(GameInstanceSubsystem + FTickableGameObject)이 소유한다. 서브시스템은 위젯을 알지 못하고 멀티캐스트 델리게이트만 브로드캐스트하므로, 위젯·World 없이 순수 함수 3개(`CombineProgress` / `ClampDisplayed` / `ShouldTransition`)로 진행률·전환 로직을 Automation 테스트할 수 있다. SKAssetCore 에는 프로젝트 비의존 범용 API 2개(`GetAllAssetPaths`, `LoadAssetsAsync`)만 추가하고 부팅 동기 로드 동작은 건드리지 않는다.

**Tech Stack:** Unreal Engine 5.7 / C++ / SKAssetCore(`USKAssetManager`·`USKAssetData`) / SKUICore(`USKUserWidget`·`USKUIManager`) / ModularGameplayActors(`AModularGameModeBase`) / Unreal Automation (`IMPLEMENT_SIMPLE_AUTOMATION_TEST`) / unreal-mcp (에셋 생성)

**Spec:** `docs/superpowers/specs/2026-07-22-loading-scene-design.md`

## Global Constraints

- 코딩 룰은 `.claude/rules/cpp-style.md` 를 그대로 따른다. 특히:
  - 한 줄 주석은 항상 `//#` 로 시작. `//`, `///`, `/* */` 금지 (UE 자동 생성 저작권 헤더는 예외).
  - `!` 단항 부정 금지 — `bFlag == false`, `Ptr == nullptr`, `IsValid(Obj) == false` 로 명시 비교.
  - UObject 포인터는 `TObjectPtr<>`.
  - include 순서: 자기 자신 → UE 헤더 → 프로젝트 헤더 → `*.generated.h` (마지막). `SortIncludes: Never` 이므로 작성자가 지킨다.
  - `Super::` 호출 누락 금지 (`BeginPlay`, `NativeConstruct`, `NativeDestruct`, `Deinitialize`).
- 하드코딩된 `/Game/...` 경로 리터럴 금지. 에셋 접근은 `USpyAssetManager::GetAssetByName<T>()` 등 이름 룩업 경유 (`.claude/rules/plugin-skassetcore.md` §2).
- 재사용 플러그인(SKAssetCore)은 게임 모듈 헤더를 include 하지 않는다 — 역방향 참조 금지 (`.claude/rules/unreal-infra.md` §1).
- 부팅 시 `USKAssetManager::StartInitialLoading` → `LoadAllPrimaryAssetsSync` 동작은 **변경하지 않는다.**
- 커밋 메시지 형식: `[Tag] ClassName — 요약` (`.claude/rules/git-conventions.md`). **`git commit` 은 사람이 실행한다** — 각 Task 의 마지막 스텝은 `git add` 까지만 하고 커밋 메시지(안)를 제시한다.
- 빌드/에디터 실행은 사람이 수행한다. 에이전트는 컴파일·테스트를 직접 돌리지 않고, 각 Task 끝에서 사람이 실행할 정확한 검증 절차를 제시한다.

## 스펙 대비 해석/보정 사항 (구현자가 알아야 할 3가지)

1. **`USpyLoadingConfig` 베이스는 `UDataAsset`** — 스펙 §4-2 표기는 `UPrimaryDataAsset` 이지만, 스펙 §8-4 는 이 에셋을 `SpyAssetData` 이름 맵에 등록해 `GetAssetByName` (secondary 에셋 경로)으로 로드하도록 지시한다. 형제 Config(`USpyMovementConfig`, `USpyMissionConfig`, `USpyAIConfig`)도 전부 `UDataAsset` 이다. `UPrimaryDataAsset` 으로 선언하면 부팅 `LoadAllPrimaryAssetsSync` 스캔에 딸려 들어갈 수 있어 §4-1 "부팅 동작 불변" 과 충돌한다. → `UDataAsset` 사용.
2. **GameMode 베이스는 `AModularGameModeBase`** — 스펙 §4-2 표기는 `AModularGameMode` 이지만, 프로젝트의 `ASpyGameMode` 가 `AModularGameModeBase` 를 상속한다. 동일 베이스로 맞춘다.
3. **2단계 완료 플래그 추가** — 스펙 §5 는 `GetAsyncLoadPercentage` 가 "미시작" 시 `-1` 을 반환하므로 0 으로 바닥을 잡으라고 한다. 그런데 이 함수는 **로드 완료 후에도** `-1` 을 반환한다. 바닥만 잡으면 `Raw` 가 영원히 1.0 에 도달하지 못해 전환이 일어나지 않는다. → `LoadPackageAsync` 완료 콜백에서 `bMapLoadComplete` 를 세우고, 완료 시 `MapPercent = 100.f` 로 공급한다 (Task 3).

---

## File Structure

**SKAssetCore 플러그인 (범용 API 2개 추가)**

| 파일 | 책임 |
|---|---|
| `SkillProject/Plugins/SKAssetCore/Source/SKAssetCore/Public/SKAssetData.h` | `GetAllAssetPaths` 선언 추가 |
| `SkillProject/Plugins/SKAssetCore/Source/SKAssetCore/Private/SKAssetData.cpp` | `GetAllAssetPaths` 구현 |
| `SkillProject/Plugins/SKAssetCore/Source/SKAssetCore/Public/SKAssetManager.h` | `FSKAssetBatchProgressDelegate` + `LoadAssetsAsync` 선언 |
| `SkillProject/Plugins/SKAssetCore/Source/SKAssetCore/Private/SKAssetManager.cpp` | `LoadAssetsAsync` 구현 (배치 진행률 카운터) |

**게임 모듈 신규/수정**

| 파일 | 책임 |
|---|---|
| `SkillProject/Source/SkillProject/Data/SpyLoadingConfig.h/.cpp` | 로딩 설정 DataAsset (맵 경로·최소 표시 시간·1단계 가중치) |
| `SkillProject/Source/SkillProject/Data/SpyAssetNames.h` | `LoadingConfig` 이름 상수 추가 |
| `SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.h/.cpp` | 로딩 파이프라인 소유 — 순수 함수 3개 + 틱 + 전환 |
| `SkillProject/Source/SkillProject/Manager/Tests/SpyLoadingProgressTests.cpp` | 순수 함수·Config 검증 Automation 테스트 |
| `SkillProject/Source/SkillProject/System/SpyLoadingGameMode.h/.cpp` | 로딩 맵 GameMode — UI 오픈 + 킥오프 |
| `SkillProject/Source/SkillProject/UI/SpyLoadingWidget.h/.cpp` | 순수 뷰 — 델리게이트 구독 → ProgressBar/Text 갱신 |
| `SkillProject/Source/SkillProject/Util/DefineEnum.h` | `ESpyUIType::Loading` 추가 |
| `SkillProject/Config/DefaultEngine.ini` | `GameDefaultMap`·`EditorStartupMap`·`ServerDefaultMap` |

**에디터 에셋 (unreal-mcp, Task 7)**

`/Game/Spy/Maps/LoadingMap`, `/Game/Spy/UI/WBP_Loading`, `/Game/Spy/Data/DA_SpyLoadingConfig`, `SpyAssetData` 등록 2건.

**테스트 가능성 경계 (중요)**

- 순수 함수(`CombineProgress` / `ClampDisplayed` / `ShouldTransition`)와 `ApplyConfig` 검증 — **Automation 유닛 테스트 대상** (World·위젯 불필요).
- `LoadAssetsAsync`, 위젯 바인딩, 실제 맵 전환 — 에셋·World 가 필요해 유닛 테스트하지 않는다. **에디터 수동 검증**(Task 8)으로 확인한다. 플랜은 이 구분을 숨기지 않는다.

---

## Task 1: SKAssetCore — 경로 열거 + 배치 비동기 로드 API

**Files:**
- Modify: `SkillProject/Plugins/SKAssetCore/Source/SKAssetCore/Public/SKAssetData.h:45-53`
- Modify: `SkillProject/Plugins/SKAssetCore/Source/SKAssetCore/Private/SKAssetData.cpp`
- Modify: `SkillProject/Plugins/SKAssetCore/Source/SKAssetCore/Public/SKAssetManager.h:8` (델리게이트), `:35-38` (public 섹션)
- Modify: `SkillProject/Plugins/SKAssetCore/Source/SKAssetCore/Private/SKAssetManager.cpp`

**Interfaces:**
- Consumes: 없음 (첫 Task)
- Produces:
  - `void USKAssetData::GetAllAssetPaths(TArray<FSoftObjectPath>& OutPaths) const`
  - `DECLARE_DELEGATE_TwoParams(FSKAssetBatchProgressDelegate, int32 /*Loaded*/, int32 /*Total*/)`
  - `void USKAssetManager::LoadAssetsAsync(const TArray<FSoftObjectPath>& AssetPaths, const FSKAssetBatchProgressDelegate& OnProgress, const FSimpleDelegate& OnComplete)` — 멤버 함수 (static 아님). 호출부는 `USKAssetManager::Get().LoadAssetsAsync(...)`.

- [ ] **Step 1: `USKAssetData::GetAllAssetPaths` 선언**

`SKAssetData.h` 의 public 섹션(`GetAssetPathByName` 아래)에 추가한다.

```cpp
public:
	FSoftObjectPath GetAssetPathByName(const FName& AssetName) const;

	//# 등록된 모든 에셋 경로를 열거한다 (로딩 화면 프리로드 등 배치 로드용)
	void GetAllAssetPaths(TArray<FSoftObjectPath>& OutPaths) const;
```

- [ ] **Step 2: `USKAssetData::GetAllAssetPaths` 구현**

`SKAssetData.cpp` 파일 끝(`GetAssetPathByName` 구현 아래)에 추가한다.

```cpp
void USKAssetData::GetAllAssetPaths(TArray<FSoftObjectPath>& OutPaths) const
{
	OutPaths.Reset();
	OutPaths.Reserve(AssetNameToPath.Num());

	for (const TPair<FName, FSoftObjectPath>& Pair : AssetNameToPath)
	{
		if (Pair.Value.IsValid())
		{
			OutPaths.Add(Pair.Value);
		}
	}
}
```

- [ ] **Step 3: 배치 진행률 델리게이트 선언**

`SKAssetManager.h:8` 의 기존 델리게이트 아래에 추가한다.

```cpp
DECLARE_DELEGATE_OneParam(FSKAssetAndDelegate, UObject*);

//# 배치 비동기 로드 진행률 — (완료 개수, 전체 개수)
DECLARE_DELEGATE_TwoParams(FSKAssetBatchProgressDelegate, int32, int32);
```

- [ ] **Step 4: `LoadAssetsAsync` 선언**

`SKAssetManager.h` 의 `public:` 섹션(`UnloadAsset` 선언 아래)에 추가한다. static 이 아니라 멤버 함수다 — 내부에서 `AddLoadedAsset` (protected) 을 호출하기 때문이다.

```cpp
public:
	static UObject* LoadAssetSync(const FSoftObjectPath& AssetPath);
	static void LoadAssetAsync(const FSoftObjectPath& AssetPath, const FSKAssetAndDelegate& OnComplete);
	static void UnloadAsset(const FSoftObjectPath& AssetPath);

	//# 경로 배열을 한 번에 비동기 로드한다. 개별 실패해도 완료 카운트는 증가한다(진행률이 멈추면 안 됨).
	//# 배열이 비었으면 OnProgress 없이 즉시 OnComplete 를 호출한다.
	void LoadAssetsAsync(const TArray<FSoftObjectPath>& AssetPaths, const FSKAssetBatchProgressDelegate& OnProgress, const FSimpleDelegate& OnComplete);
```

- [ ] **Step 5: `LoadAssetsAsync` 구현**

`SKAssetManager.cpp` 의 `UnloadAsset` 구현 아래에 추가한다. 완료 카운터는 `TSharedRef<int32>` 로 각 콜백이 공유한다(콜백은 모두 게임 스레드에서 실행된다).

```cpp
void USKAssetManager::LoadAssetsAsync(const TArray<FSoftObjectPath>& AssetPaths, const FSKAssetBatchProgressDelegate& OnProgress, const FSimpleDelegate& OnComplete)
{
	const int32 TotalCount = AssetPaths.Num();

	//# 대상이 없으면 진행률 없이 즉시 완료
	if (TotalCount == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Batch Async Load: 대상 0개 — 즉시 완료"));
		OnComplete.ExecuteIfBound();
		return;
	}

	TSharedRef<int32> LoadedCount = MakeShared<int32>(0);

	//# 성공/실패 공통 완료 처리 — 실패해도 카운트는 반드시 증가시킨다
	auto CompleteOne = [this, LoadedCount, TotalCount, OnProgress, OnComplete](const FSoftObjectPath& AssetPath)
	{
		if (UObject* Asset = AssetPath.ResolveObject())
		{
			//# GC 참조 유지
			AddLoadedAsset(Asset);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("# [SKAssetManager] Batch Async Load Failed: %s"), *AssetPath.ToString());
		}

		(*LoadedCount)++;
		OnProgress.ExecuteIfBound(*LoadedCount, TotalCount);

		if (*LoadedCount >= TotalCount)
		{
			UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Batch Async Load Complete (%d)"), TotalCount);
			OnComplete.ExecuteIfBound();
		}
	};

	for (const FSoftObjectPath& AssetPath : AssetPaths)
	{
		//# 경로가 무효하거나 이미 메모리에 있으면 즉시 완료로 카운트
		if (AssetPath.IsValid() == false || AssetPath.ResolveObject() != nullptr)
		{
			CompleteOne(AssetPath);
			continue;
		}

		GetStreamableManager().RequestAsyncLoad(AssetPath, FStreamableDelegate::CreateLambda([CompleteOne, AssetPath]()
			{
				CompleteOne(AssetPath);
			}));
	}
}
```

**주의:** 모든 경로가 이미 로드돼 있으면 `OnComplete` 가 `LoadAssetsAsync` 호출 스택 안에서 동기적으로 실행된다. 호출자(Task 3)는 이 재진입을 견뎌야 한다 — Task 3 의 `StartLoading` 은 상태 플래그를 **전부 세운 뒤에** `LoadAssetsAsync` 를 호출하도록 작성한다.

- [ ] **Step 6: 컴파일 검증 (사람 수행)**

Visual Studio 또는 Unreal Editor 에서 `SkillProject` 솔루션을 빌드한다.
Expected: `SKAssetCore` 모듈이 에러 없이 컴파일된다. 기존 호출부 변경이 없으므로 다른 모듈에 영향이 없어야 한다.

- [ ] **Step 7: Stage + 커밋 메시지(안)**

```bash
git add SkillProject/Plugins/SKAssetCore/Source/SKAssetCore/Public/SKAssetData.h \
        SkillProject/Plugins/SKAssetCore/Source/SKAssetCore/Private/SKAssetData.cpp \
        SkillProject/Plugins/SKAssetCore/Source/SKAssetCore/Public/SKAssetManager.h \
        SkillProject/Plugins/SKAssetCore/Source/SKAssetCore/Private/SKAssetManager.cpp
```

커밋 메시지(안):
```
[Feature] SKAssetManager — 배치 비동기 로드 + 경로 열거 API 추가
```

---

## Task 2: `USpyLoadingConfig` + 이름 상수 + `ESpyUIType::Loading`

**Files:**
- Create: `SkillProject/Source/SkillProject/Data/SpyLoadingConfig.h`
- Create: `SkillProject/Source/SkillProject/Data/SpyLoadingConfig.cpp`
- Modify: `SkillProject/Source/SkillProject/Data/SpyAssetNames.h:8-13`
- Modify: `SkillProject/Source/SkillProject/Util/DefineEnum.h:9-15`

**Interfaces:**
- Consumes: 없음
- Produces:
  - `class USpyLoadingConfig : public UDataAsset` — 필드 `TSoftObjectPtr<UWorld> GameplayMap`, `float MinDisplaySeconds = 2.f`, `float AssetPhaseWeight = 0.5f`
  - `SpyAssetNames::LoadingConfig` (`FName`, 값 `TEXT("SpyLoadingConfig")`)
  - `ESpyUIType::Loading` — enum 항목명이 곧 위젯 등록명(`USpyUIManager::OpenSpyUI` 가 enum 이름 문자열을 키로 사용)

- [ ] **Step 1: `SpyLoadingConfig.h` 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "SpyLoadingConfig.generated.h"

//# 로딩 씬 설정 — 전환 대상 맵·최소 표시 시간·1단계 가중치
UCLASS()
class SKILLPROJECT_API USpyLoadingConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	//# 로딩 완료 후 전환할 게임플레이 맵
	UPROPERTY(EditDefaultsOnly, Category = "Loading")
	TSoftObjectPtr<UWorld> GameplayMap;

	//# 로딩 화면 최소 표시 시간(초). 0 이하이면 시간 클램프를 건너뛴다
	UPROPERTY(EditDefaultsOnly, Category = "Loading")
	float MinDisplaySeconds = 2.f;

	//# 1단계(에셋 프리로드) 가중치. 2단계(맵 스트리밍) 가중치는 1 - AssetPhaseWeight
	UPROPERTY(EditDefaultsOnly, Category = "Loading", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AssetPhaseWeight = 0.5f;
};
```

- [ ] **Step 2: `SpyLoadingConfig.cpp` 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/SpyLoadingConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyLoadingConfig)
```

- [ ] **Step 3: `SpyAssetNames::LoadingConfig` 추가**

`SpyAssetNames.h` 의 "DataAsset 등록명" 블록에 한 줄 추가한다.

```cpp
	//# DataAsset 등록명
	inline const FName CharacterAssetData = TEXT("SpyCharacterAssetData");
	inline const FName AnimAssetData      = TEXT("SpyAnimAssetData");
	inline const FName MovementConfig     = TEXT("SpyMovementConfig");
	inline const FName LoadingConfig      = TEXT("SpyLoadingConfig");
	inline const FName GrapplePromptWidget = TEXT("WBP_GrapplePrompt");
```

- [ ] **Step 4: `ESpyUIType::Loading` 추가**

`DefineEnum.h` 의 `ESpyUIType` 에 항목을 추가한다. 이 이름(`Loading`)이 그대로 `SpyAssetData` 위젯 등록명이 되므로 Task 7 의 등록명과 반드시 일치해야 한다.

```cpp
enum ESpyUIType : uint8
{
    None            UMETA(DisplayName = "None"),
    MainHUD         UMETA(DisplayName = "MainHUD"),
    HpBar           UMETA(DisplayName = "HpBar"),
    Menu            UMETA(DisplayName = "Menu"),
    Loading         UMETA(DisplayName = "Loading"),
};
```

- [ ] **Step 5: 프로젝트 파일 재생성 + 컴파일 검증 (사람 수행)**

신규 `.h`/`.cpp` 를 추가했으므로 `SkillProject/Launch.bat` 을 실행하거나 `SkillProject.uproject` 우클릭 > Generate Visual Studio project files 후 빌드한다.
Expected: 빌드 성공. `USpyLoadingConfig` 가 에디터 클래스 목록에 나타난다.

- [ ] **Step 6: Stage + 커밋 메시지(안)**

```bash
git add SkillProject/Source/SkillProject/Data/SpyLoadingConfig.h \
        SkillProject/Source/SkillProject/Data/SpyLoadingConfig.cpp \
        SkillProject/Source/SkillProject/Data/SpyAssetNames.h \
        SkillProject/Source/SkillProject/Util/DefineEnum.h
```

커밋 메시지(안):
```
[Feature] SpyLoadingConfig — 로딩 씬 설정 DataAsset 추가
```

---

## Task 3: `USpyLoadingSubsystem` — 순수 함수 3개 (TDD)

이 Task 는 서브시스템의 **테스트 가능한 순수 로직만** 만든다. 틱·비동기 로드·전환은 Task 4 에서 붙인다. 클래스 껍데기와 순수 static 함수를 먼저 만들어 테스트를 통과시킨다.

**Files:**
- Create: `SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.h`
- Create: `SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.cpp`
- Test: `SkillProject/Source/SkillProject/Manager/Tests/SpyLoadingProgressTests.cpp`

**Interfaces:**
- Consumes: `USpyLoadingConfig` (Task 2)
- Produces:
  - `static float USpyLoadingSubsystem::CombineProgress(int32 Loaded, int32 Total, float MapPercent, float Weight)` → `Raw` (0~1)
  - `static float USpyLoadingSubsystem::ClampDisplayed(float Raw, float Elapsed, float MinDisplaySeconds)` → `Displayed` (0~1)
  - `static bool USpyLoadingSubsystem::ShouldTransition(float Raw, float Elapsed, float MinDisplaySeconds)`
  - `bool USpyLoadingSubsystem::ApplyConfig(const USpyLoadingConfig* InConfig)` — 유효하면 내부 상태에 반영하고 `true`, 아니면 `Error` 로그 후 `false`
  - `float USpyLoadingSubsystem::GetDisplayedProgress() const`

- [ ] **Step 1: 실패하는 테스트 작성**

`SkillProject/Source/SkillProject/Manager/Tests/SpyLoadingProgressTests.cpp` 를 새로 만든다. 기존 `SkillProject/Source/SkillProject/System/Tests/SpyMissionTests.cpp` 스타일을 그대로 따른다(파일 전체 `#if WITH_DEV_AUTOMATION_TESTS`, 등록 문자열 `SkillProject.<도메인>.<기능>.<케이스>`).

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Data/SpyLoadingConfig.h"
#include "Manager/SpyLoadingSubsystem.h"

//# CombineProgress 경계값 — Total == 0 이면 1단계는 완료로 본다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingCombineEmptyTotalTest,
	"SkillProject.Manager.Loading.CombineEmptyTotal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingCombineEmptyTotalTest::RunTest(const FString& Parameters)
{
	//# 1단계 대상 0개 + 2단계 미시작(-1) + 가중치 0.5 → 0.5
	const float Raw = USpyLoadingSubsystem::CombineProgress(0, 0, -1.f, 0.5f);

	TestEqual(TEXT("Phase1 counts as complete"), Raw, 0.5f, KINDA_SMALL_NUMBER);

	return true;
}

//# Loaded == 0 이면 1단계 기여는 0
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingCombineZeroLoadedTest,
	"SkillProject.Manager.Loading.CombineZeroLoaded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingCombineZeroLoadedTest::RunTest(const FString& Parameters)
{
	const float Raw = USpyLoadingSubsystem::CombineProgress(0, 10, -1.f, 0.5f);

	TestEqual(TEXT("Nothing loaded yet"), Raw, 0.f, KINDA_SMALL_NUMBER);

	return true;
}

//# 가중치 경계 — W == 0 이면 2단계만, W == 1 이면 1단계만 반영
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingCombineWeightBoundsTest,
	"SkillProject.Manager.Loading.CombineWeightBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingCombineWeightBoundsTest::RunTest(const FString& Parameters)
{
	//# W == 0 → 1단계 100% 여도 2단계(50%)만 반영
	TestEqual(TEXT("W == 0 uses map only"), USpyLoadingSubsystem::CombineProgress(10, 10, 50.f, 0.f), 0.5f, KINDA_SMALL_NUMBER);

	//# W == 1 → 2단계 100% 여도 1단계(50%)만 반영
	TestEqual(TEXT("W == 1 uses assets only"), USpyLoadingSubsystem::CombineProgress(5, 10, 100.f, 1.f), 0.5f, KINDA_SMALL_NUMBER);

	//# 범위 밖 가중치는 클램프된다
	TestEqual(TEXT("Weight clamped high"), USpyLoadingSubsystem::CombineProgress(10, 10, 0.f, 5.f), 1.f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Weight clamped low"), USpyLoadingSubsystem::CombineProgress(0, 10, 100.f, -5.f), 1.f, KINDA_SMALL_NUMBER);

	return true;
}

//# 2단계 미시작(-1)은 0 으로 바닥을 잡는다 — 음수가 진행률을 끌어내리면 안 된다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingCombineNegativeMapPercentTest,
	"SkillProject.Manager.Loading.CombineNegativeMapPercent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingCombineNegativeMapPercentTest::RunTest(const FString& Parameters)
{
	const float Raw = USpyLoadingSubsystem::CombineProgress(10, 10, -1.f, 0.5f);

	TestEqual(TEXT("Negative map percent floored to 0"), Raw, 0.5f, KINDA_SMALL_NUMBER);
	TestTrue(TEXT("Never below zero"), Raw >= 0.f);

	return true;
}

//# Displayed 는 시간 클램프를 받고 1.0 을 넘지 않는다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingClampDisplayedTest,
	"SkillProject.Manager.Loading.ClampDisplayed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingClampDisplayedTest::RunTest(const FString& Parameters)
{
	//# 로드는 끝났지만 1초만 지났으면 절반까지만 보여준다
	TestEqual(TEXT("Time clamped"), USpyLoadingSubsystem::ClampDisplayed(1.f, 1.f, 2.f), 0.5f, KINDA_SMALL_NUMBER);

	//# 시간이 충분해도 Raw 를 넘지 않는다
	TestEqual(TEXT("Raw is the ceiling"), USpyLoadingSubsystem::ClampDisplayed(0.3f, 10.f, 2.f), 0.3f, KINDA_SMALL_NUMBER);

	//# 1.0 초과 금지
	TestEqual(TEXT("Never exceeds one"), USpyLoadingSubsystem::ClampDisplayed(1.f, 10.f, 2.f), 1.f, KINDA_SMALL_NUMBER);

	//# MinDisplaySeconds <= 0 이면 시간 클램프를 건너뛴다
	TestEqual(TEXT("No time clamp when min is zero"), USpyLoadingSubsystem::ClampDisplayed(1.f, 0.f, 0.f), 1.f, KINDA_SMALL_NUMBER);

	return true;
}

//# Displayed 는 시간이 흐를수록 단조 증가한다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingDisplayedMonotonicTest,
	"SkillProject.Manager.Loading.DisplayedMonotonic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingDisplayedMonotonicTest::RunTest(const FString& Parameters)
{
	float Previous = 0.f;

	for (int32 Step = 0; Step <= 40; ++Step)
	{
		const float Elapsed = (float)Step * 0.1f;
		const float Displayed = USpyLoadingSubsystem::ClampDisplayed(1.f, Elapsed, 2.f);

		TestTrue(TEXT("Monotonic increase"), Displayed >= Previous);
		TestTrue(TEXT("Never exceeds one"), Displayed <= 1.f + KINDA_SMALL_NUMBER);

		Previous = Displayed;
	}

	return true;
}

//# 최소 표시 시간 전에는 전환하지 않는다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingTransitionBeforeMinTimeTest,
	"SkillProject.Manager.Loading.TransitionBeforeMinTime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingTransitionBeforeMinTimeTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("Not yet at min display time"), USpyLoadingSubsystem::ShouldTransition(1.f, 1.9f, 2.f));
	TestTrue(TEXT("At min display time"), USpyLoadingSubsystem::ShouldTransition(1.f, 2.f, 2.f));

	return true;
}

//# Raw 가 1.0 미만이면 시간이 아무리 지나도 전환하지 않는다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingTransitionIncompleteRawTest,
	"SkillProject.Manager.Loading.TransitionIncompleteRaw",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingTransitionIncompleteRawTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("Raw below one blocks transition"), USpyLoadingSubsystem::ShouldTransition(0.99f, 1000.f, 2.f));

	return true;
}

//# Config 누락 — 크래시 없이 false 를 반환하고 전환을 시작하지 않는다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingMissingConfigTest,
	"SkillProject.Manager.Loading.MissingConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingMissingConfigTest::RunTest(const FString& Parameters)
{
	USpyLoadingSubsystem* Subsystem = NewObject<USpyLoadingSubsystem>();

	//# Config 자체가 없음
	TestFalse(TEXT("Null config rejected"), Subsystem->ApplyConfig(nullptr));

	//# Config 는 있으나 GameplayMap 미설정
	USpyLoadingConfig* Config = NewObject<USpyLoadingConfig>();
	TestFalse(TEXT("Unset gameplay map rejected"), Subsystem->ApplyConfig(Config));

	TestEqual(TEXT("Progress untouched"), Subsystem->GetDisplayedProgress(), 0.f, KINDA_SMALL_NUMBER);

	return true;
}

#endif
```

- [ ] **Step 2: 테스트가 실패하는지 확인 (사람 수행)**

`SpyLoadingSubsystem.h` 가 아직 없으므로 컴파일이 실패한다.
Run: Visual Studio 빌드
Expected: FAIL — `Cannot open include file: 'Manager/SpyLoadingSubsystem.h'`

- [ ] **Step 3: `SpyLoadingSubsystem.h` 작성 (Task 3 범위: 순수 함수 + ApplyConfig)**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "SpyLoadingSubsystem.generated.h"

class USpyLoadingConfig;

//# 표시용 진행률(0~1) 변경 알림 — 위젯이 구독한다
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLoadingProgressChanged, float);

//# 로딩 씬 파이프라인 소유주. 위젯을 알지 못하고 델리게이트만 브로드캐스트한다.
UCLASS()
class SKILLPROJECT_API USpyLoadingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//# 데디케이티드 서버는 로딩 화면을 거치지 않으므로 생성 자체를 막는다
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

public:
	//# 진행률 합성 — Raw(0~1). MapPercent 는 -1(미시작)을 0 으로 바닥 잡는다
	static float CombineProgress(int32 Loaded, int32 Total, float MapPercent, float Weight);

	//# 표시용 진행률 — Raw 를 경과 시간으로 클램프. MinDisplaySeconds <= 0 이면 클램프 생략
	static float ClampDisplayed(float Raw, float Elapsed, float MinDisplaySeconds);

	//# 전환 조건 — Raw 가 1.0 이고 최소 표시 시간을 채웠을 때만 true
	static bool ShouldTransition(float Raw, float Elapsed, float MinDisplaySeconds);

public:
	//# Config 유효성 검사 + 내부 상태 반영. 무효하면 Error 로그 후 false
	bool ApplyConfig(const USpyLoadingConfig* InConfig);

	float GetDisplayedProgress() const { return DisplayedProgress; }

public:
	FOnLoadingProgressChanged OnProgressChanged;

protected:
	UPROPERTY(Transient)
	TObjectPtr<const USpyLoadingConfig> LoadingConfig;

	//# 전환 대상 맵의 롱 패키지명 (GetAsyncLoadPercentage / OpenLevel 공용)
	FName MapPackageName;

	float DisplayedProgress = 0.f;
};
```

- [ ] **Step 4: `SpyLoadingSubsystem.cpp` 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "Manager/SpyLoadingSubsystem.h"

#include "Data/SpyLoadingConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyLoadingSubsystem)

bool USpyLoadingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	//# 데디케이티드 서버는 ServerDefaultMap 으로 직행하므로 로딩 서브시스템이 필요 없다
	if (IsRunningDedicatedServer())
	{
		return false;
	}

	return Super::ShouldCreateSubsystem(Outer);
}

float USpyLoadingSubsystem::CombineProgress(int32 Loaded, int32 Total, float MapPercent, float Weight)
{
	//# 1단계 대상이 0개면 완료로 본다
	const float Phase1Ratio = (Total > 0) ? ((float)Loaded / (float)Total) : 1.f;

	//# GetAsyncLoadPercentage 는 미시작 시 -1 을 반환한다 — 반드시 0 으로 바닥을 잡는다
	const float Phase2Ratio = FMath::Max(MapPercent, 0.f) / 100.f;

	const float ClampedWeight = FMath::Clamp(Weight, 0.f, 1.f);

	return FMath::Clamp(Phase1Ratio * ClampedWeight + Phase2Ratio * (1.f - ClampedWeight), 0.f, 1.f);
}

float USpyLoadingSubsystem::ClampDisplayed(float Raw, float Elapsed, float MinDisplaySeconds)
{
	const float ClampedRaw = FMath::Clamp(Raw, 0.f, 1.f);

	//# 최소 표시 시간이 없으면 Raw 를 그대로 보여준다
	if (MinDisplaySeconds <= 0.f)
	{
		return ClampedRaw;
	}

	return FMath::Min(ClampedRaw, Elapsed / MinDisplaySeconds);
}

bool USpyLoadingSubsystem::ShouldTransition(float Raw, float Elapsed, float MinDisplaySeconds)
{
	return (Raw >= 1.f) && (Elapsed >= MinDisplaySeconds);
}

bool USpyLoadingSubsystem::ApplyConfig(const USpyLoadingConfig* InConfig)
{
	if (InConfig == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] LoadingConfig 를 찾을 수 없습니다 — 맵 전환을 중단합니다"));
		return false;
	}

	if (InConfig->GameplayMap.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] LoadingConfig 의 GameplayMap 이 비어 있습니다 — 맵 전환을 중단합니다"));
		return false;
	}

	LoadingConfig = InConfig;
	MapPackageName = FName(*InConfig->GameplayMap.ToSoftObjectPath().GetLongPackageName());

	return true;
}
```

- [ ] **Step 5: 테스트 통과 확인 (사람 수행)**

빌드 후 에디터에서 Tools > Session Frontend > Automation 탭을 열고 `SkillProject.Manager.Loading` 필터로 9개 테스트를 실행한다.
Expected: `CombineEmptyTotal`, `CombineZeroLoaded`, `CombineWeightBounds`, `CombineNegativeMapPercent`, `ClampDisplayed`, `DisplayedMonotonic`, `TransitionBeforeMinTime`, `TransitionIncompleteRaw`, `MissingConfig` — 전부 PASS.

- [ ] **Step 6: Stage + 커밋 메시지(안)**

```bash
git add SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.h \
        SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.cpp \
        SkillProject/Source/SkillProject/Manager/Tests/SpyLoadingProgressTests.cpp
```

커밋 메시지(안):
```
[Feature] SpyLoadingSubsystem — 진행률 합성·전환 조건 순수 함수 + 테스트
```

---

## Task 4: `USpyLoadingSubsystem` — 파이프라인(2단계 로드 + 틱 + 전환)

**Files:**
- Modify: `SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.h` (Task 3 결과물)
- Modify: `SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.cpp` (Task 3 결과물)

**Interfaces:**
- Consumes: `USKAssetManager::LoadAssetsAsync` / `FSKAssetBatchProgressDelegate` / `USKAssetData::GetAllAssetPaths` (Task 1), `USpyLoadingConfig` · `SpyAssetNames::LoadingConfig` (Task 2), `ApplyConfig` · 순수 함수 3개 (Task 3)
- Produces:
  - `void USpyLoadingSubsystem::StartLoading()` — 파이프라인 시작. Task 5 의 GameMode 가 호출한다.
  - `FOnLoadingProgressChanged OnProgressChanged` (Task 3 에서 이미 선언) — Task 6 의 위젯이 구독한다.

- [ ] **Step 1: 헤더에 FTickableGameObject + 파이프라인 상태 추가**

`SpyLoadingSubsystem.h` **전체를 아래 내용으로 교체한다** (Task 3 헤더의 확장판이다 — 이어붙이면 `ShouldCreateSubsystem` 등이 중복 선언된다).

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"

#include "SpyLoadingSubsystem.generated.h"

class USpyLoadingConfig;

//# 표시용 진행률(0~1) 변경 알림 — 위젯이 구독한다
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLoadingProgressChanged, float);

//# 로딩 씬 파이프라인 소유주. 위젯을 알지 못하고 델리게이트만 브로드캐스트한다.
UCLASS()
class SKILLPROJECT_API USpyLoadingSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	//# 데디케이티드 서버는 로딩 화면을 거치지 않으므로 생성 자체를 막는다
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;

public:
	//# FTickableGameObject — 로딩 중일 때만 틱한다(전환 후에는 GameInstance 에 남아도 유휴 상태)
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return bLoading; }
	virtual bool IsTickableInEditor() const override { return false; }
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(USpyLoadingSubsystem, STATGROUP_Tickables); }

public:
	//# 로딩 파이프라인 시작 — 1단계(에셋 프리로드) → 2단계(맵 스트리밍) → 전환
	void StartLoading();

public:
	static float CombineProgress(int32 Loaded, int32 Total, float MapPercent, float Weight);
	static float ClampDisplayed(float Raw, float Elapsed, float MinDisplaySeconds);
	static bool ShouldTransition(float Raw, float Elapsed, float MinDisplaySeconds);

public:
	bool ApplyConfig(const USpyLoadingConfig* InConfig);

	float GetDisplayedProgress() const { return DisplayedProgress; }

public:
	FOnLoadingProgressChanged OnProgressChanged;

protected:
	//# 1단계 진행률 콜백
	void HandleAssetProgress(int32 Loaded, int32 Total);

	//# 1단계 완료 → 2단계(맵 패키지) 비동기 로드 시작
	void HandleAssetPhaseComplete();

	//# 2단계 완료 콜백
	void HandleMapPackageLoaded(const FName& PackageName, UPackage* Package, EAsyncLoadingResult::Type Result);

	//# 게임플레이 맵으로 전환
	void TransitionToGameplayMap();

	//# 현재 2단계 진행률(%) — 미시작 0, 완료 100
	float GetMapPercent() const;

protected:
	UPROPERTY(Transient)
	TObjectPtr<const USpyLoadingConfig> LoadingConfig;

	FName MapPackageName;

	float DisplayedProgress = 0.f;

	bool bLoading = false;
	bool bAssetPhaseComplete = false;
	bool bMapLoadComplete = false;
	bool bTransitionStarted = false;

	int32 AssetLoadedCount = 0;
	int32 AssetTotalCount = 0;

	float ElapsedSeconds = 0.f;
};
```

- [ ] **Step 2: cpp 에 include 추가**

`SpyLoadingSubsystem.cpp` 상단 include 블록을 아래로 교체한다.

```cpp
#include "Manager/SpyLoadingSubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "SKAssetManager.h"
#include "SKAssetData.h"
#include "Data/SpyLoadingConfig.h"
#include "Data/SpyAssetNames.h"
#include "Manager/SpyAssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyLoadingSubsystem)
```

- [ ] **Step 3: `StartLoading` 구현**

`ApplyConfig` 구현 아래에 추가한다. **상태 플래그를 모두 세운 뒤에** `LoadAssetsAsync` 를 호출한다 — 모든 에셋이 이미 메모리에 있으면 완료 콜백이 이 함수 스택 안에서 동기 실행되기 때문이다(Task 1 주의사항).

```cpp
void USpyLoadingSubsystem::StartLoading()
{
	if (bLoading)
	{
		return;
	}

	const USpyLoadingConfig* Config = USpyAssetManager::GetAssetByName<USpyLoadingConfig>(SpyAssetNames::LoadingConfig);
	if (ApplyConfig(Config) == false)
	{
		//# Config 이상 — 임의 맵 이름으로 fallback 하지 않는다 (하드코딩 금지)
		return;
	}

	//# 1단계 대상 = 이름 맵에 등록된 secondary 에셋 전체
	TArray<FSoftObjectPath> AssetPaths;
	USKAssetManager::Get().GetAssetData().GetAllAssetPaths(AssetPaths);

	AssetTotalCount = AssetPaths.Num();
	AssetLoadedCount = 0;
	ElapsedSeconds = 0.f;
	DisplayedProgress = 0.f;
	bAssetPhaseComplete = false;
	bMapLoadComplete = false;
	bTransitionStarted = false;

	//# 콜백이 동기 실행될 수 있으므로 마지막에 켠다
	bLoading = true;

	UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 로딩 시작 — 1단계 대상 %d 개, 대상 맵 %s"), AssetTotalCount, *MapPackageName.ToString());

	USKAssetManager::Get().LoadAssetsAsync(
		AssetPaths,
		FSKAssetBatchProgressDelegate::CreateUObject(this, &USpyLoadingSubsystem::HandleAssetProgress),
		FSimpleDelegate::CreateUObject(this, &USpyLoadingSubsystem::HandleAssetPhaseComplete));
}
```

- [ ] **Step 4: 1단계 콜백 + 2단계 시작 구현**

```cpp
void USpyLoadingSubsystem::HandleAssetProgress(int32 Loaded, int32 Total)
{
	AssetLoadedCount = Loaded;
	AssetTotalCount = Total;
}

void USpyLoadingSubsystem::HandleAssetPhaseComplete()
{
	bAssetPhaseComplete = true;

	//# 2단계 — 맵 패키지 비동기 스트리밍 시작
	const int32 RequestId = LoadPackageAsync(
		MapPackageName.ToString(),
		FLoadPackageAsyncDelegate::CreateUObject(this, &USpyLoadingSubsystem::HandleMapPackageLoaded));

	if (RequestId == INDEX_NONE)
	{
		//# 요청 자체가 실패해도 전환은 시도한다 — OpenLevel 의 엔진 동기 로드로 커버된다
		UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] 맵 패키지 비동기 로드 요청 실패: %s"), *MapPackageName.ToString());
		bMapLoadComplete = true;
	}
}

void USpyLoadingSubsystem::HandleMapPackageLoaded(const FName& PackageName, UPackage* Package, EAsyncLoadingResult::Type Result)
{
	if (Result != EAsyncLoadingResult::Succeeded)
	{
		UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] 맵 패키지 비동기 로드 실패: %s"), *PackageName.ToString());
	}

	//# 성공/실패 무관하게 2단계는 종료로 처리한다 (진행률이 멈추면 안 된다)
	bMapLoadComplete = true;
}
```

- [ ] **Step 5: 틱·진행률 갱신·전환 구현**

`GetAsyncLoadPercentage` 는 로드 **완료 후에도** `-1` 을 반환하므로 완료 플래그로 100 을 공급한다.

```cpp
float USpyLoadingSubsystem::GetMapPercent() const
{
	//# 2단계 완료 — GetAsyncLoadPercentage 는 완료 후에도 -1 을 반환하므로 플래그로 판정한다
	if (bMapLoadComplete)
	{
		return 100.f;
	}

	//# 1단계가 끝나기 전에는 2단계가 시작되지 않았으므로 0
	if (bAssetPhaseComplete == false)
	{
		return 0.f;
	}

	return GetAsyncLoadPercentage(MapPackageName);
}

void USpyLoadingSubsystem::Tick(float DeltaTime)
{
	if (bLoading == false)
	{
		return;
	}

	if (LoadingConfig == nullptr)
	{
		bLoading = false;
		return;
	}

	ElapsedSeconds += DeltaTime;

	const float Raw = CombineProgress(AssetLoadedCount, AssetTotalCount, GetMapPercent(), LoadingConfig->AssetPhaseWeight);
	const float NewDisplayed = ClampDisplayed(Raw, ElapsedSeconds, LoadingConfig->MinDisplaySeconds);

	//# 단조 증가 보장 — 값이 커질 때만 반영·브로드캐스트한다
	if (NewDisplayed > DisplayedProgress)
	{
		DisplayedProgress = NewDisplayed;
		OnProgressChanged.Broadcast(DisplayedProgress);
	}

	if (bTransitionStarted == false && ShouldTransition(Raw, ElapsedSeconds, LoadingConfig->MinDisplaySeconds))
	{
		TransitionToGameplayMap();
	}
}

void USpyLoadingSubsystem::TransitionToGameplayMap()
{
	bTransitionStarted = true;

	//# 전환 후에는 유휴 상태로 — 서브시스템은 GameInstance 수명이라 맵이 바뀌어도 살아남는다
	bLoading = false;

	//# 마지막 프레임에 100% 를 확실히 보여준다
	if (DisplayedProgress < 1.f)
	{
		DisplayedProgress = 1.f;
		OnProgressChanged.Broadcast(DisplayedProgress);
	}

	UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 게임플레이 맵 전환: %s"), *MapPackageName.ToString());

	UGameplayStatics::OpenLevel(GetGameInstance(), MapPackageName);
}

void USpyLoadingSubsystem::Deinitialize()
{
	bLoading = false;
	OnProgressChanged.Clear();
	LoadingConfig = nullptr;

	Super::Deinitialize();
}
```

- [ ] **Step 6: 기존 테스트가 여전히 통과하는지 확인 (사람 수행)**

빌드 후 Automation 탭에서 `SkillProject.Manager.Loading` 9개 테스트를 재실행한다.
Expected: 전부 PASS (Task 3 의 순수 함수·`ApplyConfig` 동작은 변하지 않았다).

- [ ] **Step 7: Stage + 커밋 메시지(안)**

```bash
git add SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.h \
        SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.cpp
```

커밋 메시지(안):
```
[Feature] SpyLoadingSubsystem — 2단계 로딩 파이프라인·맵 자동 전환 구현
```

---

## Task 5: `ASpyLoadingGameMode` — 로딩 맵 킥오프

**Files:**
- Create: `SkillProject/Source/SkillProject/System/SpyLoadingGameMode.h`
- Create: `SkillProject/Source/SkillProject/System/SpyLoadingGameMode.cpp`

**Interfaces:**
- Consumes: `USpyLoadingSubsystem::StartLoading` (Task 4), `USpyUIManager::OpenSpyUI` + `ESpyUIType::Loading` (Task 2)
- Produces: `class ASpyLoadingGameMode : public AModularGameModeBase` — Task 7 의 LoadingMap World Settings 에서 GameMode Override 로 지정한다.

- [ ] **Step 1: `SpyLoadingGameMode.h` 작성**

```cpp
// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ModularGameMode.h"

#include "SpyLoadingGameMode.generated.h"

//# 로딩 맵 전용 GameMode — 로딩 UI 오픈과 파이프라인 킥오프만 담당한다
UCLASS(minimalapi)
class ASpyLoadingGameMode : public AModularGameModeBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
};
```

- [ ] **Step 2: `SpyLoadingGameMode.cpp` 작성**

UI 를 **먼저** 연다. `StartLoading` 은 모든 에셋이 이미 메모리에 있으면 동기적으로 완료까지 진행할 수 있어, 위젯이 구독하기 전에 호출하면 첫 브로드캐스트를 놓친다.

```cpp
// Copyright Epic Games, Inc. All Rights Reserved.

#include "System/SpyLoadingGameMode.h"

#include "Engine/GameInstance.h"
#include "Manager/SpyUIManager.h"
#include "Manager/SpyLoadingSubsystem.h"
#include "Util/DefineEnum.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyLoadingGameMode)

void ASpyLoadingGameMode::BeginPlay()
{
	Super::BeginPlay();

	//# 위젯이 먼저 구독해야 첫 진행률 브로드캐스트를 놓치지 않는다
	if (USpyUIManager* UIManager = USpyUIManager::Get(this))
	{
		UIManager->OpenSpyUI(ESpyUIType::Loading);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("# [SpyLoadingGameMode] SpyUIManager 를 찾을 수 없습니다 — 로딩 UI 없이 진행합니다"));
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance == nullptr)
	{
		return;
	}

	//# 데디케이티드 서버에서는 서브시스템이 생성되지 않는다 — null 이면 조용히 무시
	if (USpyLoadingSubsystem* LoadingSubsystem = GameInstance->GetSubsystem<USpyLoadingSubsystem>())
	{
		LoadingSubsystem->StartLoading();
	}
}
```

- [ ] **Step 3: 프로젝트 파일 재생성 + 컴파일 검증 (사람 수행)**

`SkillProject/Launch.bat` 실행 후 빌드한다.
Expected: 빌드 성공. 에디터 World Settings 의 GameMode Override 드롭다운에 `SpyLoadingGameMode` 가 보인다.

- [ ] **Step 4: Stage + 커밋 메시지(안)**

```bash
git add SkillProject/Source/SkillProject/System/SpyLoadingGameMode.h \
        SkillProject/Source/SkillProject/System/SpyLoadingGameMode.cpp
```

커밋 메시지(안):
```
[Feature] SpyLoadingGameMode — 로딩 맵 UI 오픈·파이프라인 킥오프
```

---

## Task 6: `USpyLoadingWidget` — 순수 뷰

**Files:**
- Create: `SkillProject/Source/SkillProject/UI/SpyLoadingWidget.h`
- Create: `SkillProject/Source/SkillProject/UI/SpyLoadingWidget.cpp`

**Interfaces:**
- Consumes: `USpyLoadingSubsystem::OnProgressChanged` / `GetDisplayedProgress` (Task 3·4)
- Produces: `class USpyLoadingWidget : public USKUserWidget` — Task 7 의 `WBP_Loading` 부모 클래스. BindWidget 이름은 `LoadingBar`(ProgressBar), `PercentText`(TextBlock).

- [ ] **Step 1: `SpyLoadingWidget.h` 작성**

`USpyHPBar` 와 동일하게 `USKUserWidget` 을 직접 상속한다.

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SKUserWidget.h"

#include "SpyLoadingWidget.generated.h"

class UProgressBar;
class UTextBlock;

//# 로딩 화면 뷰 — 서브시스템 진행률을 구독해 바/퍼센트만 갱신한다
UCLASS()
class SKILLPROJECT_API USpyLoadingWidget : public USKUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	//# 진행률(0~1) 반영
	void HandleProgressChanged(float InDisplayed);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UProgressBar> LoadingBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UTextBlock> PercentText;

	FDelegateHandle ProgressChangedHandle;
};
```

- [ ] **Step 2: `SpyLoadingWidget.cpp` 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SpyLoadingWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Manager/SpyLoadingSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyLoadingWidget)

void USpyLoadingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance == nullptr)
	{
		return;
	}

	USpyLoadingSubsystem* LoadingSubsystem = GameInstance->GetSubsystem<USpyLoadingSubsystem>();
	if (LoadingSubsystem == nullptr)
	{
		return;
	}

	ProgressChangedHandle = LoadingSubsystem->OnProgressChanged.AddUObject(this, &USpyLoadingWidget::HandleProgressChanged);

	//# 구독 이전에 진행된 분량을 즉시 반영한다
	HandleProgressChanged(LoadingSubsystem->GetDisplayedProgress());
}

void USpyLoadingWidget::NativeDestruct()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USpyLoadingSubsystem* LoadingSubsystem = GameInstance->GetSubsystem<USpyLoadingSubsystem>())
		{
			LoadingSubsystem->OnProgressChanged.Remove(ProgressChangedHandle);
		}
	}

	ProgressChangedHandle.Reset();

	Super::NativeDestruct();
}

void USpyLoadingWidget::HandleProgressChanged(float InDisplayed)
{
	const float Clamped = FMath::Clamp(InDisplayed, 0.f, 1.f);

	if (IsValid(LoadingBar))
	{
		LoadingBar->SetPercent(Clamped);
	}

	if (IsValid(PercentText))
	{
		const int32 Percent = FMath::RoundToInt(Clamped * 100.f);
		PercentText->SetText(FText::AsNumber(Percent));
	}
}
```

- [ ] **Step 3: 프로젝트 파일 재생성 + 컴파일 검증 (사람 수행)**

`SkillProject/Launch.bat` 실행 후 빌드한다.
Expected: 빌드 성공. Widget Blueprint 생성 시 부모 클래스 목록에 `SpyLoadingWidget` 이 보인다.

- [ ] **Step 4: Stage + 커밋 메시지(안)**

```bash
git add SkillProject/Source/SkillProject/UI/SpyLoadingWidget.h \
        SkillProject/Source/SkillProject/UI/SpyLoadingWidget.cpp
```

커밋 메시지(안):
```
[Feature] SpyLoadingWidget — 로딩바·퍼센트 표시 위젯 추가
```

---

## Task 7: 에디터 에셋 생성 + 등록 + ini 설정

**이 Task 는 TDD 대상이 아니다.** 코드가 아니라 에디터 에셋·설정을 만드는 작업이며, 검증은 테스트 실행이 아니라 에셋 속성 확인으로 한다. 에셋 생성은 `unreal-mcp` (`execute_python`, `set_asset_property`, `add_asset_entry`, `save_asset`) 로 수행한다. **에디터에서 열려 있는 에셋을 컴파일하지 말 것** — 에디터가 멈춘다.

**Files:**
- Create (에셋): `/Game/Spy/Maps/LoadingMap`, `/Game/Spy/UI/WBP_Loading`, `/Game/Spy/Data/DA_SpyLoadingConfig`
- Modify (에셋): `SpyAssetData` — 엔트리 2건 추가
- Modify: `SkillProject/Config/DefaultEngine.ini:1-5`

**Interfaces:**
- Consumes: `ASpyLoadingGameMode` (Task 5), `USpyLoadingWidget` (Task 6), `USpyLoadingConfig` (Task 2), `ESpyUIType::Loading` enum 이름 (Task 2)
- Produces: 런타임에 이름으로 해석되는 에셋 2개 — `Loading` → `WBP_Loading`, `SpyLoadingConfig` → `DA_SpyLoadingConfig`

- [ ] **Step 1: `/Game/Spy/Maps/LoadingMap` 생성**

빈 레벨을 만들고 World Settings 의 GameMode Override 를 `ASpyLoadingGameMode` 로 지정한다. MCP `execute_python` 으로:

```python
import unreal

# 빈 레벨 생성 후 저장
unreal.EditorLevelLibrary.new_level("/Game/Spy/Maps/LoadingMap")

world = unreal.EditorLevelLibrary.get_editor_world()
world_settings = world.get_world_settings()
world_settings.set_editor_property(
    "default_game_mode",
    unreal.load_class(None, "/Script/SkillProject.SpyLoadingGameMode"),
)

unreal.EditorLevelLibrary.save_current_level()
print(world_settings.get_editor_property("default_game_mode"))
```

**주의:** UE 5.7 에서 `EditorLevelLibrary` 는 deprecated 다 (deprecation 경고가 뜬다). 경고로 끝나지 않고 실패하면 `unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)` 의 `new_level` / `save_current_level` 로 대체한다.

Expected 출력: `<Class '/Script/SkillProject.SpyLoadingGameMode'>` 형태로 GameMode 가 찍힌다.

- [ ] **Step 2: `/Game/Spy/UI/WBP_Loading` 생성 (에디터 수동 작업)**

**위젯 트리는 파이썬으로 편집하지 않는다** — 프로그래밍 방식 UMG 트리 편집은 에디터를 멈추게 하는 사례가 있다. 에셋 생성만 MCP 로 하고, 자식 위젯 배치는 에디터에서 손으로 한다.

에셋 생성 (MCP `execute_python`):

```python
import unreal

factory = unreal.WidgetBlueprintFactory()
factory.set_editor_property("parent_class", unreal.load_class(None, "/Script/SkillProject.SpyLoadingWidget"))

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
wbp = asset_tools.create_asset("WBP_Loading", "/Game/Spy/UI", unreal.WidgetBlueprint, factory)

unreal.EditorAssetLibrary.save_asset("/Game/Spy/UI/WBP_Loading")
print(wbp.get_path_name())
```

이어서 에디터에서 `WBP_Loading` 을 열고 위젯 트리를 아래 순서로 구성한다.

**루트는 `CanvasPanel` 이어야 한다.** 기획서 §5-2 의 좌표(앵커 `(0.5, 1.0)` + Alignment + 오프셋 `Y -120`)는 **`CanvasPanelSlot` 에만 있는 속성**이라, 캔버스가 없으면 그 레이아웃을 지정할 수 없다. 팩토리로 만든 WBP 에 루트가 비어 있으면 `CanvasPanel` 을 먼저 넣는다 (이미 있으면 그대로 사용).

| 순서 | 위젯 | 변수명 | 비고 |
|---|---|---|---|
| 루트 | `CanvasPanel` | (기본명 무방) | 아래 3개의 부모 |
| ① | `Border` | (기본명 무방) | 배경 `#08090B`, 전체 채움. **반드시 첫 자식** |
| ② | `ProgressBar` | **`LoadingBar`** | BindWidget — 이름 정확히 |
| ③ | `TextBlock` | **`PercentText`** | BindWidget — 이름 정확히 |

②③ 의 변수명이 다르면 `BindWidget` 이 컴파일 에러를 낸다. ① 배경을 마지막에 넣으면 바·텍스트를 덮어 화면이 통째로 검게 나온다 (기획서 §5-2 z-order 항).

배치 후 저장 + 컴파일한다.

Expected: `WBP_Loading` 컴파일 시 BindWidget 경고/에러가 없다. (변수명이 틀리면 "A widget named LoadingBar is required" 에러가 뜬다.)

- [ ] **Step 3: `/Game/Spy/Data/DA_SpyLoadingConfig` 생성 + 값 설정**

`USpyLoadingConfig` DataAsset 을 만들고 `GameplayMap = /Game/Spy/Maps/DevMap`, **`MinDisplaySeconds = 2.5`**, **`AssetPhaseWeight = 0.25`** 로 설정한다.

**주의:** 이 두 값은 기획서(`docs/design/loading-scene.md` §9-7)가 플랜 초안(2.0 / 0.5)을 덮어쓴 확정값이다. DataAsset 값이 C++ 기본값을 덮으므로 초안 값으로 만들면 기획 확정이 런타임에서 조용히 무효화된다.

```python
import unreal

factory = unreal.DataAssetFactory()
factory.set_editor_property("data_asset_class", unreal.load_class(None, "/Script/SkillProject.SpyLoadingConfig"))

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
config = asset_tools.create_asset("DA_SpyLoadingConfig", "/Game/Spy/Data", None, factory)

config.set_editor_property("gameplay_map", unreal.SoftObjectPath("/Game/Spy/Maps/DevMap.DevMap"))
config.set_editor_property("min_display_seconds", 2.5)
config.set_editor_property("asset_phase_weight", 0.25)

unreal.EditorAssetLibrary.save_asset("/Game/Spy/Data/DA_SpyLoadingConfig")
print(config.get_editor_property("gameplay_map"))
```

Expected: `GameplayMap` 이 `/Game/Spy/Maps/DevMap.DevMap` 로 찍힌다.

- [ ] **Step 4: `SpyAssetData` 에 엔트리 2건 등록**

MCP `add_asset_entry` 로 등록한다. 등록명은 **정확히** 아래와 같아야 한다 — `Loading` 은 `ESpyUIType::Loading` 의 enum 이름, `SpyLoadingConfig` 는 `SpyAssetNames::LoadingConfig` 값이다.

| AssetName | AssetPath |
|---|---|
| `Loading` | `/Game/Spy/UI/WBP_Loading.WBP_Loading` |
| `SpyLoadingConfig` | `/Game/Spy/Data/DA_SpyLoadingConfig.DA_SpyLoadingConfig` |

등록 후 `save_spy_asset_data` (또는 `save_asset`) 로 저장한다.

Expected: `get_spy_asset_data` 결과에 두 엔트리가 보인다.

- [ ] **Step 5: `DefaultEngine.ini` 맵 설정 변경**

`SkillProject/Config/DefaultEngine.ini` 의 `[/Script/EngineSettings.GameMapsSettings]` 섹션을 아래로 만든다. 데디케이티드 서버는 로딩 맵을 거치지 않는다.

```ini
[/Script/EngineSettings.GameMapsSettings]
GameDefaultMap=/Game/Spy/Maps/LoadingMap.LoadingMap
EditorStartupMap=/Game/Spy/Maps/LoadingMap.LoadingMap
ServerDefaultMap=/Game/Spy/Maps/DevMap.DevMap
GlobalDefaultGameMode=/Script/SkillProject.SpyGameMode
GameInstanceClass=/Script/SkillProject.SpyGameInstance
```

- [ ] **Step 6: 에셋 검증 (사람 수행)**

에디터에서 확인한다.
1. `LoadingMap` 을 열고 World Settings > GameMode Override 가 `SpyLoadingGameMode` 인지 확인.
2. `WBP_Loading` 을 열고 컴파일 — BindWidget 에러가 없는지 확인.
3. `DA_SpyLoadingConfig` 의 `GameplayMap` 이 `DevMap` 인지 확인.
4. `SpyAssetData` 에 `Loading`·`SpyLoadingConfig` 엔트리가 있는지 확인.

- [ ] **Step 7: Stage + 커밋 메시지(안)**

```bash
git add SkillProject/Config/DefaultEngine.ini \
        SkillProject/Content/Spy/Maps/LoadingMap.umap \
        SkillProject/Content/Spy/UI/WBP_Loading.uasset \
        SkillProject/Content/Spy/Data/DA_SpyLoadingConfig.uasset
```

`SpyAssetData` 에셋 파일(경로는 `git status` 로 확인)도 함께 stage 한다.

커밋 메시지(안):
```
[Chore] LoadingMap — 로딩 씬 에셋·부팅 맵 설정 추가
```

---

## Task 8: 통합 검증 (수동)

**Files:** 없음 (검증 전용). 문제 발견 시 해당 Task 로 돌아간다.

**Interfaces:**
- Consumes: Task 1~7 전부
- Produces: 없음

- [ ] **Step 1: 에디터 재시작 후 PIE 실행 (사람 수행)**

`DefaultEngine.ini` 의 `EditorStartupMap` 변경은 에디터 재시작 후 반영된다. 재시작하면 `LoadingMap` 이 열린다. PIE 를 실행한다.

Expected:
- 로딩바가 0 에서 시작해 약 2초에 걸쳐 100% 까지 찬다.
- 퍼센트 텍스트가 함께 증가한다.
- 100% 도달 후 `DevMap` 으로 전환되고 플레이어 캐릭터가 스폰된다.
- Output Log 에 `# [SpyLoadingSubsystem] 로딩 시작 — 1단계 대상 N 개`, `# [SpyLoadingSubsystem] 게임플레이 맵 전환: /Game/Spy/Maps/DevMap` 이 찍힌다.

**바가 0 에서 움직이지 않으면:** `FTickableGameObject` 를 `UGameInstanceSubsystem` 에 얹으면 PIE 에서 어느 World 기준으로 틱하는지가 보장되지 않는다(기본 `GetTickableGameObjectWorld()` 가 null). 진행률 계산 자체는 Task 3 테스트로 검증돼 있으므로, 이 경우 원인은 틱이 안 도는 것이다. 대안: `StartLoading` 에서 `GetGameInstance()->GetTimerManager()` 로 0.05초 반복 타이머를 걸어 `Tick` 본문을 구동하거나, `ASpyLoadingGameMode::Tick` 에서 서브시스템을 구동한다.

**알려진 한계:** PIE 는 맵을 `/Temp/UEDPIE_0_...` 로 복제해 열기 때문에 2단계는 실측이 아니다. PIE 에서는 사실상 `MinDisplaySeconds` 페이스로 흐르는 바를 보게 된다 (스펙 §5 "알려진 한계"). 2단계 진행률·프리로드 이득은 Standalone/패키징 빌드에서만 유효하다.

- [ ] **Step 2: Standalone 실행 검증 (사람 수행)**

에디터 Play 드롭다운 > Standalone Game 으로 실행한다.

Expected: PIE 와 동일하되, Output Log 의 진행률이 `MinDisplaySeconds` 페이스가 아니라 실제 로드 진척을 따라간다. 전환 후 정상 플레이.

- [ ] **Step 3: 예외 경로 검증 — Config 누락 (사람 수행)**

`DA_SpyLoadingConfig` 의 `GameplayMap` 을 일시적으로 비우고 PIE 를 실행한다.

Expected:
- 크래시 없음.
- Output Log 에 `# [SpyLoadingSubsystem] LoadingConfig 의 GameplayMap 이 비어 있습니다 — 맵 전환을 중단합니다` (Error).
- 로딩 화면에 머무르고 임의 맵으로 넘어가지 않는다.

검증 후 `GameplayMap` 을 `/Game/Spy/Maps/DevMap` 로 되돌리고 저장한다.

- [ ] **Step 4: 데디케이티드 서버 경로 확인 (사람 수행)**

에디터 Play 설정에서 Net Mode 를 `Play As Client` (Run Dedicated Server 체크)로 두고 실행한다.

Expected:
- 서버는 `ServerDefaultMap` (`DevMap`) 으로 바로 뜨고 로딩 맵을 거치지 않는다.
- 서버 로그에 `SpyLoadingSubsystem` 로그가 전혀 없다 (`ShouldCreateSubsystem` 이 `false`).
- 클라이언트는 정상 접속된다.

- [ ] **Step 5: 전체 Automation 테스트 재실행 (사람 수행)**

Session Frontend > Automation 에서 `SkillProject` 전체를 실행한다.
Expected: 기존 테스트(`SkillProject.AI.*`, `SkillProject.System.Mission.*`, 레벨 테스트) + 신규 `SkillProject.Manager.Loading.*` 9개 모두 PASS.

---

## Task 커버리지 — 스펙 대조

| 스펙 항목 | 담당 Task |
|---|---|
| §4-1 `GetAllAssetPaths` | Task 1 Step 1-2 |
| §4-1 `FSKAssetBatchProgressDelegate` + `LoadAssetsAsync` (4개 요구사항 전부) | Task 1 Step 3-5 |
| §4-2 `USpyLoadingConfig` (3개 필드) | Task 2 Step 1-2 |
| §4-2 `ESpyUIType::Loading` | Task 2 Step 4 |
| §4-2 `USpyLoadingSubsystem` 공개 API 4종 | Task 3 (순수 함수·`GetDisplayedProgress`) / Task 4 (`StartLoading`·`OnProgressChanged`) |
| §4-2 `ASpyLoadingGameMode` | Task 5 |
| §4-2 `USpyLoadingWidget` | Task 6 |
| §4-3 의존 방향 (서브시스템이 위젯을 모름) | Task 4·6 — 위젯이 구독자, 서브시스템은 델리게이트만 |
| §4-4 `ShouldCreateSubsystem` 서버 차단 | Task 3 Step 3-4 |
| §4-4 `ServerDefaultMap` | Task 7 Step 5 |
| §5 진행률 합성 공식 | Task 3 Step 4 (`CombineProgress`/`ClampDisplayed`/`ShouldTransition`), Task 4 Step 5 (`Tick`) |
| §5 퍼센트 표기 `RoundToInt(Displayed * 100)` | Task 6 Step 2 |
| §6 Config 없음 → Error + 중단 | Task 3 `ApplyConfig` + 테스트 `MissingConfig` + Task 8 Step 3 |
| §6 개별 에셋 실패 → 카운트 증가 + Warning | Task 1 Step 5 |
| §6 1단계 대상 0개 | Task 1 Step 5 (즉시 `OnComplete`), Task 3 `CombineProgress` `Total == 0` 테스트 |
| §6 맵 로드 실패 → Error 후에도 전환 시도 | Task 4 Step 4 (`bMapLoadComplete = true`) |
| §6 서브시스템 미생성 → null 무시 | Task 5 Step 2 |
| §7 테스트 5종 | Task 3 Step 1 (9개 테스트) |
| §8 에셋·등록·ini 6항목 | Task 7 |
| §9 범위 밖 | 어떤 Task 도 구현하지 않음 |
