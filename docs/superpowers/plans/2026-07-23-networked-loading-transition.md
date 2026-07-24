# 네트워크 로딩 전환 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 데디케이티드 서버 모델에서 클라이언트가 로컬 `OpenLevel`로 서버와 끊기는 대신, config 서버 주소로 `ClientTravel`(접속)하여 서버 월드에 합류하도록 로딩 전환을 바로잡는다. 접속 실패 시 에러 + 재시도를 지원한다.

**Architecture:** 기존 로딩 파이프라인(프리로드·persistent UI·순수 함수·페이싱)을 유지하고, 전환 메커니즘만 `OpenLevel` → `ClientTravel`로 교체한다. 접속 단계의 진행률·타임아웃은 트래블을 넘어 살아남는 `UGameInstance` 타이머로 구동하고, 접속 실패는 엔진의 `OnNetworkFailure`/`OnTravelFailure` 델리게이트로 감지한다. 순수 판정 로직은 static 함수로 추출해 World 없이 테스트한다.

**Tech Stack:** Unreal Engine 5.7 / C++ / `APlayerController::ClientTravel` / `UGameInstance::GetTimerManager` / `GEngine->OnNetworkFailure`·`OnTravelFailure` / `FCoreUObjectDelegates::PostLoadMapWithWorld` / Unreal Automation

**Spec:** `docs/superpowers/specs/2026-07-23-networked-loading-transition-design.md`
**선행:** `docs/superpowers/plans/2026-07-22-loading-scene.md`(Task 1~8) + `2026-07-23-loading-persistent-ui.md`(Task 9~13). 그 산출물이 구현·검증된 상태를 전제로 하며 이 플랜은 Task 14부터 이어진다.

## Global Constraints

- `.claude/rules/cpp-style.md` 전항: 주석 `//#`, 단항 `!` 금지(`== false`/`== nullptr`/`IsValid(x) == false`), `TObjectPtr`, include 순서(자기자신→UE→프로젝트→generated), `Super::` 누락 금지.
- 하드코딩 `/Game/...` 경로 금지 — 에셋은 이름 룩업 경유. 단, 이 플랜은 새 하드코딩 경로를 도입하지 않는다.
- `plugin-skuicore.md` §2 유지 — 게임 모듈은 매니저 API만 호출, 뷰포트 직접 조작 금지.
- 순수 함수 `CombineProgress`/`ClampDisplayed`/`ShouldTransition`과 기존 20개 테스트를 **바꾸지 않는다.**
- **클라이언트는 절대 `OpenLevel`로 게임플레이 맵을 열지 않는다** — 서버와 끊기는 원인. `OpenLevel` 오프라인 폴백은 `NM_Standalone`에서만.
- `git commit` 금지 — `git add` + 커밋 메시지(안)까지.
- 빌드·검증은 사람이 수행. **PIE "2인"은 데디서버 검증에 쓰지 않는다**(데디서버 별도 실행 + 클라 실행).

## 확정 수치 (기획 확인 대상 표기)

- `ConnectTimeoutSeconds` C++ 기본값 **`15.f`** — 접속 안전 상한. game-designer가 DataAsset 값 확정.
- 접속 단계 바 상한 비율 **`0.95f`** — 도착 전 표시 진행률이 프리로드 지점~(1.0 직전)까지만 차오르고 1.0은 도착 시에만. 이 상수는 UX 판단값(연출), 코드 상수로 고정.

---

## File Structure

| 파일 | 변경 |
|---|---|
| `SkillProject/Source/SkillProject/Data/SpyLoadingConfig.h` | `ServerAddress`, `ConnectTimeoutSeconds` 필드 추가 |
| `SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.h/.cpp` | 접속 전환·접속단계 진행률·실패/타임아웃/재시도·도착판정 강화 |
| `SkillProject/Source/SkillProject/Manager/Tests/SpyLoadingConnectTests.cpp` | 신규 순수 함수 4종 경계 테스트 |
| `SkillProject/Source/SkillProject/UI/SpyLoadingWidget.h/.cpp` | 에러 상태(ErrorText·RetryButton) |
| `SkillProject/Content/Spy/Data/DA_SpyLoadingConfig` | `ServerAddress`·`ConnectTimeoutSeconds` 값 (MCP) |
| `SkillProject/Content/Spy/UI/WBP_Loading` | `ErrorText`·`RetryButton` 배치 (에디터) |

**테스트 가능성 경계:** 순수 판정 함수 4종(`ShouldConnectToServer`/`IsLoadingMapName`/`HasConnectTimedOut`/`ConnectPhaseDisplayed`)만 Automation 유닛 대상. `ClientTravel`·타이머·델리게이트·위젯은 World/뷰포트/네트워크 의존이라 유닛 테스트하지 않고 Task 20 수동 검증으로 확인한다. 이 구분을 숨기지 않는다.

---

## Task 14: Config — ServerAddress + ConnectTimeoutSeconds

**Files:**
- Modify: `SkillProject/Source/SkillProject/Data/SpyLoadingConfig.h`

**Interfaces:**
- Consumes: 없음
- Produces: `USpyLoadingConfig::ServerAddress`(`FString`), `USpyLoadingConfig::ConnectTimeoutSeconds`(`float`, 기본 `15.f`)

- [ ] **Step 1: 필드 추가**

`SpyLoadingConfig.h`의 `AssetPhaseWeight` 아래에 추가한다.

```cpp
	//# 1단계(에셋 프리로드) 가중치. 2단계(맵 스트리밍) 가중치는 1 - AssetPhaseWeight
	UPROPERTY(EditDefaultsOnly, Category = "Loading", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AssetPhaseWeight = 0.9f;

	//# 자동 접속 대상 서버 주소 (예: "127.0.0.1:7777"). 비어 있으면 오프라인 폴백(NM_Standalone 에서만 OpenLevel)
	UPROPERTY(EditDefaultsOnly, Category = "Loading|Network")
	FString ServerAddress;

	//# 접속 타임아웃(초). 이 시간 안에 도착하지 못하면 실패로 간주. 0 이하이면 타임아웃 없음
	UPROPERTY(EditDefaultsOnly, Category = "Loading|Network")
	float ConnectTimeoutSeconds = 15.f;
```

- [ ] **Step 2: 빌드 검증 (사람 수행)**

Visual Studio 빌드.
Expected: 성공. `DA_SpyLoadingConfig` 디테일에 `ServerAddress`·`ConnectTimeoutSeconds`가 나타난다.

- [ ] **Step 3: Stage + 커밋 메시지(안)**

```bash
git add SkillProject/Source/SkillProject/Data/SpyLoadingConfig.h
```
```
[Feature] SpyLoadingConfig — 서버 주소·접속 타임아웃 필드 추가
```

---

## Task 15: 순수 판정 함수 4종 (TDD)

**Files:**
- Modify: `SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.h`
- Modify: `SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.cpp`
- Test: `SkillProject/Source/SkillProject/Manager/Tests/SpyLoadingConnectTests.cpp`

**Interfaces:**
- Consumes: 없음(순수 static)
- Produces:
  - `static bool USpyLoadingSubsystem::ShouldConnectToServer(const FString& ServerAddress)`
  - `static bool USpyLoadingSubsystem::IsLoadingMapName(FName LoadedMapName, FName LoadingMapName)`
  - `static bool USpyLoadingSubsystem::HasConnectTimedOut(float ConnectElapsed, float TimeoutSeconds)`
  - `static float USpyLoadingSubsystem::ConnectPhaseDisplayed(float PreloadWeight, float ConnectElapsed, float ConnectPacingSeconds)`

- [ ] **Step 1: 실패하는 테스트 작성**

`SkillProject/Source/SkillProject/Manager/Tests/SpyLoadingConnectTests.cpp` 신규. 기존 `SpyLoadingProgressTests.cpp` 스타일 준수(파일 전체 `#if WITH_DEV_AUTOMATION_TESTS`, 등록 문자열 `SkillProject.Manager.Loading.<케이스>`).

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Manager/SpyLoadingSubsystem.h"

//# ServerAddress 가 비어 있지 않으면 접속 모드
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingShouldConnectTest,
	"SkillProject.Manager.Loading.ShouldConnect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingShouldConnectTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Non-empty address connects"), USpyLoadingSubsystem::ShouldConnectToServer(TEXT("127.0.0.1:7777")));
	TestFalse(TEXT("Empty address is offline"), USpyLoadingSubsystem::ShouldConnectToServer(TEXT("")));

	return true;
}

//# 도착 판정 — 로드된 맵이 로딩맵이면 무시(true), 다른 맵이면 도착(false)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingIsLoadingMapNameTest,
	"SkillProject.Manager.Loading.IsLoadingMapName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingIsLoadingMapNameTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Same name is loading map"), USpyLoadingSubsystem::IsLoadingMapName(FName("LoadingMap"), FName("LoadingMap")));
	TestFalse(TEXT("Gameplay map is not loading map"), USpyLoadingSubsystem::IsLoadingMapName(FName("DevMap"), FName("LoadingMap")));

	return true;
}

//# 타임아웃 — 경과 >= 타임아웃일 때만 true, 0 이하 타임아웃은 항상 false(무제한)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingConnectTimeoutTest,
	"SkillProject.Manager.Loading.ConnectTimeout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingConnectTimeoutTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("Before timeout"), USpyLoadingSubsystem::HasConnectTimedOut(14.9f, 15.f));
	TestTrue(TEXT("At timeout"), USpyLoadingSubsystem::HasConnectTimedOut(15.f, 15.f));
	TestFalse(TEXT("Zero timeout means unlimited"), USpyLoadingSubsystem::HasConnectTimedOut(1000.f, 0.f));
	TestFalse(TEXT("Negative timeout means unlimited"), USpyLoadingSubsystem::HasConnectTimedOut(1000.f, -1.f));

	return true;
}

//# 접속 단계 표시 진행률 — 프리로드 지점에서 시작해 도착 전 1.0 에 못 닿는다(상한 0.95)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyLoadingConnectPhaseDisplayedTest,
	"SkillProject.Manager.Loading.ConnectPhaseDisplayed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyLoadingConnectPhaseDisplayedTest::RunTest(const FString& Parameters)
{
	//# 접속 시작(경과 0) → 프리로드 가중치에서 시작
	TestEqual(TEXT("Starts at preload weight"), USpyLoadingSubsystem::ConnectPhaseDisplayed(0.9f, 0.f, 15.f), 0.9f, KINDA_SMALL_NUMBER);

	//# 페이싱 절반(7.5s) → 0.9 + 0.1*0.5*0.95 = 0.9475
	TestEqual(TEXT("Half pacing"), USpyLoadingSubsystem::ConnectPhaseDisplayed(0.9f, 7.5f, 15.f), 0.9475f, KINDA_SMALL_NUMBER);

	//# 페이싱 도달/초과 → 상한(0.9 + 0.1*0.95 = 0.995), 절대 1.0 미만
	TestEqual(TEXT("Caps below one"), USpyLoadingSubsystem::ConnectPhaseDisplayed(0.9f, 100.f, 15.f), 0.995f, KINDA_SMALL_NUMBER);
	TestTrue(TEXT("Never reaches one"), USpyLoadingSubsystem::ConnectPhaseDisplayed(0.9f, 1e9f, 15.f) < 1.f);

	//# 페이싱 0 이하 → 즉시 상한
	TestEqual(TEXT("Zero pacing jumps to cap"), USpyLoadingSubsystem::ConnectPhaseDisplayed(0.9f, 0.f, 0.f), 0.995f, KINDA_SMALL_NUMBER);

	return true;
}

#endif
```

- [ ] **Step 2: 실패 확인 (사람 수행)**

빌드.
Expected: FAIL — `ShouldConnectToServer` 등 미선언 컴파일 에러.

- [ ] **Step 3: 헤더에 선언 추가**

`SpyLoadingSubsystem.h`의 `ShouldTransition` 선언 아래에 추가한다.

```cpp
	//# 전환 조건 — Raw 가 1.0 이고 최소 표시 시간을 채웠을 때만 true
	static bool ShouldTransition(float Raw, float Elapsed, float MinDisplaySeconds);

	//# 서버 주소가 지정돼 있으면 접속 모드, 비어 있으면 오프라인 폴백
	static bool ShouldConnectToServer(const FString& ServerAddress);

	//# 도착 판정 보조 — 로드된 맵이 로딩맵과 같으면 true(도착 아님, 무시)
	static bool IsLoadingMapName(FName LoadedMapName, FName LoadingMapName);

	//# 접속 타임아웃 판정 — 경과가 타임아웃 이상이면 true. 타임아웃 0 이하이면 무제한(false)
	static bool HasConnectTimedOut(float ConnectElapsed, float TimeoutSeconds);

	//# 접속 단계 표시 진행률 — 프리로드 가중치에서 시작해 도착 전에는 1.0 에 못 닿는다(상한 0.95 비율)
	static float ConnectPhaseDisplayed(float PreloadWeight, float ConnectElapsed, float ConnectPacingSeconds);
```

- [ ] **Step 4: 구현 추가**

`SpyLoadingSubsystem.cpp`의 `ShouldTransition` 구현 아래에 추가한다.

```cpp
bool USpyLoadingSubsystem::ShouldConnectToServer(const FString& ServerAddress)
{
	return ServerAddress.IsEmpty() == false;
}

bool USpyLoadingSubsystem::IsLoadingMapName(FName LoadedMapName, FName LoadingMapName)
{
	return LoadedMapName == LoadingMapName;
}

bool USpyLoadingSubsystem::HasConnectTimedOut(float ConnectElapsed, float TimeoutSeconds)
{
	if (TimeoutSeconds <= 0.f)
	{
		return false;
	}

	return ConnectElapsed >= TimeoutSeconds;
}

float USpyLoadingSubsystem::ConnectPhaseDisplayed(float PreloadWeight, float ConnectElapsed, float ConnectPacingSeconds)
{
	//# 도착 전에는 1.0 에 닿지 않게 남은 구간의 95% 까지만 채운다
	const float ConnectDisplayCap = 0.95f;
	const float Remaining = FMath::Clamp(1.f - PreloadWeight, 0.f, 1.f);

	//# 페이싱이 0 이하이면 즉시 상한
	const float Fraction = (ConnectPacingSeconds > 0.f)
		? FMath::Clamp(ConnectElapsed / ConnectPacingSeconds, 0.f, 1.f)
		: 1.f;

	return FMath::Clamp(PreloadWeight, 0.f, 1.f) + Remaining * Fraction * ConnectDisplayCap;
}
```

- [ ] **Step 5: 통과 확인 (사람 수행)**

빌드 후 Automation `SkillProject.Manager.Loading` 실행.
Expected: 기존 20 + 신규 4 = **24건 PASS**.

- [ ] **Step 6: Stage + 커밋 메시지(안)**

```bash
git add SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.h \
        SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.cpp \
        SkillProject/Source/SkillProject/Manager/Tests/SpyLoadingConnectTests.cpp
```
```
[Feature] SpyLoadingSubsystem — 접속 판정·접속단계 진행률 순수 함수 + 테스트
```

---

## Task 16: 접속 전환 + 오프라인 폴백 가드 + Config 배선

**Files:**
- Modify: `SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.h`
- Modify: `SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.cpp`

**Interfaces:**
- Consumes: `USpyLoadingConfig::ServerAddress`·`ConnectTimeoutSeconds`(Task 14), 순수 함수(Task 15)
- Produces: `TransitionToGameplayMap`이 접속 모드/오프라인 폴백으로 분기. 접속 상태 멤버(`bConnecting` 등).

- [ ] **Step 1: 헤더에 접속 상태·멤버 추가**

`SpyLoadingSubsystem.h`의 멤버 블록(`ElapsedSeconds` 아래, `PostLoadMapHandle` 위)에 추가한다.

```cpp
	float ElapsedSeconds = 0.f;

	//# Config 에서 복사한 접속 설정
	FString ServerAddress;
	float ConnectTimeoutSeconds = 0.f;

	//# 로딩을 시작한 맵(로딩맵)의 PIE 접두어 제거 이름 — 도착 판정에 쓴다
	FName LoadingMapName;

	//# 접속 진행 중 여부 — 전환(접속 개시) 후 도착/실패까지 true
	bool bConnecting = false;

	//# 접속 개시 시각(월드 무관, FPlatformTime 기준)
	double ConnectStartTime = 0.0;

	//# PostLoadMapWithWorld 구독 핸들
	FDelegateHandle PostLoadMapHandle;
```

- [ ] **Step 2: `IsTickable` 을 접속 상태까지 포함**

`SpyLoadingSubsystem.h`의 `IsTickable`을 아래로 교체한다. (접속 진행률·타임아웃은 Task 17에서 GameInstance 타이머로 구동하므로 Tick 은 로딩 단계에만 필요하지만, `bConnecting` 을 틱 조건에 넣어 두면 오프라인 폴백과 의미가 일관된다. 접속 단계 자체는 타이머가 담당한다.)

```cpp
	virtual bool IsTickable() const override
	{
		return bLoading;
	}
```

(변경 없음 — 명시적으로 확인만. 접속 단계는 타이머 구동이라 `bLoading`으로 충분하다.)

- [ ] **Step 3: `ApplyConfig` 에서 접속 설정 복사**

`SpyLoadingSubsystem.cpp`의 `ApplyConfig` 안, `MapPackageName` 을 세팅하는 곳 근처에 추가한다. (기존 `ApplyConfig` 구현에서 `LoadingConfig = InConfig;` 다음 줄들에 이어서.)

```cpp
	LoadingConfig = InConfig;
	MapPackageName = FName(*InConfig->GameplayMap.ToSoftObjectPath().GetLongPackageName());

	//# 접속 설정 복사
	ServerAddress = InConfig->ServerAddress;
	ConnectTimeoutSeconds = InConfig->ConnectTimeoutSeconds;

	return true;
```

- [ ] **Step 4: `StartLoading` 에서 로딩맵 이름 캡처**

`StartLoading` 안, `bTransitionStarted = false;` 다음에 추가한다. 도착 판정에 쓸 로딩맵 이름을 지금 잡아 둔다(PIE 접두어 제거).

```cpp
	bTransitionStarted = false;
	bConnecting = false;

	//# 도착 판정용 — 현재(로딩맵) 월드 이름을 PIE 접두어 제거해 저장
	if (const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		LoadingMapName = FName(*UWorld::RemovePIEPrefix(World->GetName()));
	}
```

- [ ] **Step 5: `TransitionToGameplayMap` 을 접속/오프라인 분기로 교체**

기존 `TransitionToGameplayMap` 전체를 아래로 교체한다.

```cpp
void USpyLoadingSubsystem::TransitionToGameplayMap()
{
	bTransitionStarted = true;

	//# 로딩 단계 틱 종료 — 이후는 접속(타이머) 또는 오프라인 OpenLevel 이 담당
	bLoading = false;

	//# 접속 모드 — 서버 주소로 ClientTravel. 클라는 자기 월드를 열지 않고 서버에 합류한다.
	if (ShouldConnectToServer(ServerAddress))
	{
		APlayerController* PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr;
		if (PC == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] 접속할 PlayerController 가 없습니다 — 접속 실패 처리"));
			HandleConnectFailed(TEXT("No local PlayerController"));
			return;
		}

		bConnecting = true;
		ConnectStartTime = FPlatformTime::Seconds();
		StartConnectWatch();

		UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 서버 접속 개시: %s"), *ServerAddress);
		PC->ClientTravel(ServerAddress, ETravelType::TRAVEL_Absolute);
		return;
	}

	//# 오프라인 폴백 — NM_Standalone(단일 authority)에서만 OpenLevel. 네트워크 클라는 절대 여기서 열지 않는다.
	const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (World != nullptr && World->GetNetMode() == NM_Standalone)
	{
		//# 마지막 프레임에 100% 를 확실히 보여준다
		if (DisplayedProgress < 1.f)
		{
			DisplayedProgress = 1.f;
			OnProgressChanged.Broadcast(DisplayedProgress);
		}

		UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 오프라인 전환(Standalone): %s"), *MapPackageName.ToString());
		UGameplayStatics::OpenLevel(GetGameInstance(), MapPackageName);
		return;
	}

	//# ServerAddress 비어 있는데 네트워크 클라 — OpenLevel 하면 서버와 끊긴다. 금지.
	UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] ServerAddress 미설정 + 비-Standalone — 전환을 중단합니다(클라 OpenLevel 금지)"));
}
```

- [ ] **Step 5b: `HandleAssetPhaseComplete` 를 접속 모드에서 로컬 스트리밍 생략하도록 분기**

**(블로커 수정 — 이게 없으면 접속 단계 바가 100%로 고정된다.)** 접속 모드에서 로컬 `LoadPackageAsync(DevMap)` 를 돌리면 `bMapLoadComplete`→`GetMapPercent`=100→`Raw`=1.0 으로 바가 접속 전에 이미 100%가 되어, Task 17의 접속 단계 creep(0.9→0.995)이 전부 무시된다. 접속 모드는 로컬 스트리밍을 생략한다 — 1단계 프리로드가 이미 DevMap 폐포를 캐시에 올렸고(측정: DevMap 폐포 ⊂ 프리로드 폐포), 실제 맵은 `ClientTravel` 로 서버에서 받는다.

기존 `HandleAssetPhaseComplete` 전체를 아래로 교체한다.

```cpp
void USpyLoadingSubsystem::HandleAssetPhaseComplete()
{
	bAssetPhaseComplete = true;

	//# 접속 모드 — 로컬 맵 스트리밍을 돌리지 않는다.
	//# 1단계 프리로드가 이미 DevMap 폐포를 캐시에 올렸고(측정), 로컬 LoadPackageAsync 는
	//# 바를 1.0 으로 채워 접속 단계 진행률을 죽인다. 실제 맵은 ClientTravel 로 서버에서 받는다.
	if (ShouldConnectToServer(ServerAddress))
	{
		return;
	}

	//# 오프라인 모드 — 2단계 맵 패키지 비동기 스트리밍
	const int32 RequestId = LoadPackageAsync(
		MapPackageName.ToString(),
		FLoadPackageAsyncDelegate::CreateUObject(this, &USpyLoadingSubsystem::HandleMapPackageLoaded));

	if (RequestId == INDEX_NONE)
	{
		//# 요청 자체가 실패해도 전환은 시도한다
		UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] 맵 패키지 비동기 로드 요청 실패: %s"), *MapPackageName.ToString());
		bMapLoadComplete = true;
	}
}
```

`ServerAddress` 는 `ApplyConfig`(StartLoading 초반)에서 이미 세팅되므로, `HandleAssetPhaseComplete` 가 동기 재진입으로 불려도 값이 유효하다.

- [ ] **Step 5c: `Tick` 의 전환 트리거를 접속/오프라인으로 분기**

**(블로커 수정 연장.)** 접속 모드에서는 `Raw` 가 0.9 를 넘지 않으므로(맵%가 바를 안 채움) 기존 `ShouldTransition(Raw>=1.0)` 이 영원히 거짓이다. 전환 트리거를 모드별로 나눈다 — 접속 모드는 "프리로드 완료 + 최소 표시 시간"에 접속을 시작하고, 그 시점 바는 ~0.9 여서 creep 이 이어받는다.

기존 `Tick` 의 전환 조건 블록:

```cpp
	if (bTransitionStarted == false && ShouldTransition(Raw, ElapsedSeconds, LoadingConfig->MinDisplaySeconds))
	{
		TransitionToGameplayMap();
	}
```

을 아래로 교체한다.

```cpp
	//# 전환 트리거 — 접속 모드는 프리로드 완료 + 최소 표시 시간(맵%가 바를 1.0 으로 채우기 전에 접속 개시),
	//# 오프라인 모드는 기존 Raw>=1.0 조건.
	const bool bReadyToTransition = ShouldConnectToServer(ServerAddress)
		? (bAssetPhaseComplete && ElapsedSeconds >= LoadingConfig->MinDisplaySeconds)
		: ShouldTransition(Raw, ElapsedSeconds, LoadingConfig->MinDisplaySeconds);

	if (bTransitionStarted == false && bReadyToTransition)
	{
		TransitionToGameplayMap();
	}
```

검증: 접속 모드 웜 부팅 시 P1=1·맵%=0 → Raw=0.9 → 바가 `MinDisplaySeconds`에 걸쳐 0→0.9 로 차오르고, 프리로드 완료 + 시간 충족 시 `ClientTravel` 개시(바 0.9). 이후 타이머 creep 0.9→0.995, 도착 시 1.0. **접속 전 바가 1.0 에 닿지 않음을 확인.**

- [ ] **Step 6: `StartConnectWatch`/`HandleConnectFailed` 전방 선언 추가(빈 구현은 Task 17)**

Task 17에서 채울 두 메서드를 헤더 protected 블록에 선언한다. Task 16 단계에서는 컴파일을 위해 최소 스텁을 cpp에 둔다.

헤더(`HandlePostLoadMap` 선언 아래):

```cpp
	//# 트래블 완료 시점 — 로딩 UI 를 내린다
	void HandlePostLoadMap(UWorld* LoadedWorld);

	//# 접속 감시 시작(타이머) / 접속 실패 처리
	void StartConnectWatch();
	void StopConnectWatch();
	void HandleConnectFailed(const FString& Reason);
```

cpp(임시 스텁 — Task 17에서 교체):

```cpp
void USpyLoadingSubsystem::StartConnectWatch()
{
}

void USpyLoadingSubsystem::StopConnectWatch()
{
}

void USpyLoadingSubsystem::HandleConnectFailed(const FString& Reason)
{
	UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] 접속 실패: %s"), *Reason);
	bConnecting = false;
}
```

- [ ] **Step 7: `HandlePostLoadMap` 도착 판정 강화**

기존 `HandlePostLoadMap` 을 아래로 교체한다. 로딩맵 자신/중간 월드에 대한 콜백은 무시하고, 실제 게임플레이 월드 도착에만 UI를 닫는다.

```cpp
void USpyLoadingSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	//# 아직 전환을 시작하지 않았으면 로딩맵 자신의 로드다 — 무시
	if (bTransitionStarted == false)
	{
		return;
	}

	//# 로드된 월드가 로딩맵이면(접속 중 재브라우즈 등) 도착이 아니다 — 무시
	if (LoadedWorld != nullptr)
	{
		const FName LoadedName = FName(*UWorld::RemovePIEPrefix(LoadedWorld->GetName()));
		if (IsLoadingMapName(LoadedName, LoadingMapName))
		{
			return;
		}
	}

	//# 도착 — 접속 감시 종료, 100% 반영, UI 종료
	StopConnectWatch();
	bConnecting = false;

	if (DisplayedProgress < 1.f)
	{
		DisplayedProgress = 1.f;
		OnProgressChanged.Broadcast(DisplayedProgress);
	}

	if (USpyUIManager* UIManager = USpyUIManager::Get(GetGameInstance()))
	{
		UIManager->ClosePersistentSpyUI(ESpyUIType::Loading);
	}

	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}

	UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 도착 — 로딩 UI 종료"));
}
```

- [ ] **Step 8: cpp include 확인**

`SpyLoadingSubsystem.cpp` 상단에 아래가 있는지 확인하고 없으면 추가한다.

```cpp
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
```

- [ ] **Step 9: 빌드 + 회귀 확인 (사람 수행)**

빌드 후 Automation `SkillProject.Manager.Loading` 재실행.
Expected: **24건 PASS**(순수 함수 불변). 컴파일 성공.

- [ ] **Step 10: Stage + 커밋 메시지(안)**

```bash
git add SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.h \
        SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.cpp
```
```
[Fix] SpyLoadingSubsystem — 클라 OpenLevel 제거, 서버 ClientTravel 접속으로 전환
```

---

## Task 17: 접속 감시 — 진행률·타임아웃·실패·재시도

**Files:**
- Modify: `SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.h`
- Modify: `SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.cpp`

**Interfaces:**
- Consumes: `ConnectPhaseDisplayed`·`HasConnectTimedOut`(Task 15), `GEngine->OnNetworkFailure`·`OnTravelFailure`, `UGameInstance::GetTimerManager`
- Produces:
  - `FOnLoadingConnectFailed OnConnectionFailed`(멀티캐스트, `const FString&`)
  - `void USpyLoadingSubsystem::RetryConnect()`

- [ ] **Step 1: 헤더에 델리게이트·멤버·메서드 선언**

`SpyLoadingSubsystem.h`의 진행률 델리게이트 아래에 접속 실패 델리게이트를 추가한다.

```cpp
//# 표시용 진행률(0~1) 변경 알림 — 위젯이 구독한다
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLoadingProgressChanged, float);

//# 접속 실패 알림(사유 문자열) — 위젯이 에러 UI 를 띄운다
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLoadingConnectFailed, const FString&);
```

public 델리게이트 블록(`OnProgressChanged` 아래)에 추가한다.

```cpp
	FOnLoadingProgressChanged OnProgressChanged;
	FOnLoadingConnectFailed OnConnectionFailed;
```

public 메서드(`StartLoading` 근처)에 재시도 선언을 추가한다.

```cpp
	//# 접속 실패 후 재시도 — 위젯 버튼이 호출
	void RetryConnect();
```

멤버 블록에 타이머·델리게이트 핸들을 추가한다.

```cpp
	double ConnectStartTime = 0.0;

	//# 접속 감시 타이머(GameInstance 타이머 — 트래블 넘어 생존)
	FTimerHandle ConnectWatchTimer;

	//# 엔진 네트워크/트래블 실패 구독 핸들
	FDelegateHandle NetworkFailureHandle;
	FDelegateHandle TravelFailureHandle;

	//# 접속 실패 상태(재시도 대기) — 타임아웃/실패 콜백 중복 방지
	bool bConnectFailed = false;
```

- [ ] **Step 2: cpp include 추가**

```cpp
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
```

- [ ] **Step 3: `StartConnectWatch`/`StopConnectWatch` 구현(스텁 교체)**

Task 16의 스텁을 아래로 교체한다. GameInstance 타이머로 0.05초마다 접속 단계 진행률·타임아웃을 갱신하고, 엔진 실패 델리게이트를 구독한다.

```cpp
void USpyLoadingSubsystem::StartConnectWatch()
{
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance == nullptr)
	{
		return;
	}

	bConnectFailed = false;

	//# 엔진 실패 델리게이트 구독(중복 방지)
	if (GEngine != nullptr)
	{
		if (NetworkFailureHandle.IsValid() == false)
		{
			NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &USpyLoadingSubsystem::HandleNetworkFailure);
		}
		if (TravelFailureHandle.IsValid() == false)
		{
			TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &USpyLoadingSubsystem::HandleTravelFailure);
		}
	}

	//# GameInstance 타이머 — 월드가 바뀌어도 살아남는다
	GameInstance->GetTimerManager().SetTimer(
		ConnectWatchTimer, this, &USpyLoadingSubsystem::TickConnectWatch, 0.05f, true);
}

void USpyLoadingSubsystem::StopConnectWatch()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		GameInstance->GetTimerManager().ClearTimer(ConnectWatchTimer);
	}

	if (GEngine != nullptr)
	{
		if (NetworkFailureHandle.IsValid())
		{
			GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
			NetworkFailureHandle.Reset();
		}
		if (TravelFailureHandle.IsValid())
		{
			GEngine->OnTravelFailure().Remove(TravelFailureHandle);
			TravelFailureHandle.Reset();
		}
	}
}
```

- [ ] **Step 4: `TickConnectWatch`(타이머 콜백) 구현**

접속 단계 바를 기어오르게 하고, 타임아웃을 감시한다.

```cpp
void USpyLoadingSubsystem::TickConnectWatch()
{
	if (bConnecting == false || bConnectFailed)
	{
		return;
	}

	const float ConnectElapsed = (float)(FPlatformTime::Seconds() - ConnectStartTime);

	//# 접속 단계 표시 진행률 — 프리로드 지점에서 도착 전까지 서서히(1.0 미만) 차오른다
	const float PreloadWeight = LoadingConfig ? LoadingConfig->AssetPhaseWeight : 0.9f;
	const float NewDisplayed = ConnectPhaseDisplayed(PreloadWeight, ConnectElapsed, ConnectTimeoutSeconds);
	if (NewDisplayed > DisplayedProgress)
	{
		DisplayedProgress = NewDisplayed;
		OnProgressChanged.Broadcast(DisplayedProgress);
	}

	//# 타임아웃 — 도착 없이 상한 시간 초과
	if (HasConnectTimedOut(ConnectElapsed, ConnectTimeoutSeconds))
	{
		HandleConnectFailed(TEXT("Connect timed out"));
	}
}
```

- [ ] **Step 5: 실패 콜백 3종 구현(`HandleConnectFailed` 스텁 교체 + 엔진 델리게이트 어댑터)**

Task 16의 `HandleConnectFailed` 스텁을 아래로 교체하고, 엔진 델리게이트 시그니처에 맞는 어댑터 2개를 추가한다.

```cpp
void USpyLoadingSubsystem::HandleConnectFailed(const FString& Reason)
{
	//# 중복 실패 콜백 무시(타임아웃 + 네트워크 실패가 겹칠 수 있다)
	if (bConnectFailed)
	{
		return;
	}

	bConnectFailed = true;
	bConnecting = false;

	StopConnectWatch();

	UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] 접속 실패: %s"), *Reason);
	OnConnectionFailed.Broadcast(Reason);
}

void USpyLoadingSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	if (bConnecting == false)
	{
		return;
	}

	HandleConnectFailed(FString::Printf(TEXT("NetworkFailure: %s"), *ErrorString));
}

void USpyLoadingSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	if (bConnecting == false)
	{
		return;
	}

	HandleConnectFailed(FString::Printf(TEXT("TravelFailure: %s"), *ErrorString));
}
```

- [ ] **Step 6: `RetryConnect` 구현**

접속 재개시. 실패 상태를 풀고 다시 `ClientTravel`.

```cpp
void USpyLoadingSubsystem::RetryConnect()
{
	if (ShouldConnectToServer(ServerAddress) == false)
	{
		UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] RetryConnect — ServerAddress 가 없습니다"));
		return;
	}

	APlayerController* PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr;
	if (PC == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] RetryConnect — PlayerController 가 없습니다"));
		return;
	}

	//# 진행률을 프리로드 지점으로 되돌린다 — 실패 시 0.995 에 동결됐으므로,
	//# 리셋하지 않으면 단조 가드(NewDisplayed > DisplayedProgress)가 creep 을 막아 바가 100% 로 남는다.
	const float PreloadWeight = LoadingConfig ? LoadingConfig->AssetPhaseWeight : 0.9f;
	DisplayedProgress = PreloadWeight;
	OnProgressChanged.Broadcast(DisplayedProgress);

	bConnectFailed = false;
	bConnecting = true;
	ConnectStartTime = FPlatformTime::Seconds();
	StartConnectWatch();

	UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 접속 재시도: %s"), *ServerAddress);
	PC->ClientTravel(ServerAddress, ETravelType::TRAVEL_Absolute);
}
```

- [ ] **Step 7: 헤더에 콜백 3종 선언**

`SpyLoadingSubsystem.h` protected 블록에 추가한다.

```cpp
	//# 접속 감시 타이머 콜백
	void TickConnectWatch();

	//# 엔진 네트워크/트래블 실패 어댑터
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);
```

`ENetworkFailure`/`ETravelFailure` 를 위해 헤더 상단에 include 를 추가한다.

```cpp
#include "Engine/EngineBaseTypes.h"
```

- [ ] **Step 8: `Deinitialize` 에서 접속 감시 정리**

기존 `Deinitialize` 를 아래로 교체한다.

```cpp
void USpyLoadingSubsystem::Deinitialize()
{
	StopConnectWatch();

	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}

	bLoading = false;
	bConnecting = false;
	OnProgressChanged.Clear();
	OnConnectionFailed.Clear();
	LoadingConfig = nullptr;

	Super::Deinitialize();
}
```

- [ ] **Step 9: 빌드 + 회귀 (사람 수행)**

빌드 후 Automation `SkillProject.Manager.Loading`.
Expected: **24건 PASS**. 컴파일 성공.

- [ ] **Step 10: Stage + 커밋 메시지(안)**

```bash
git add SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.h \
        SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.cpp
```
```
[Feature] SpyLoadingSubsystem — 접속 진행률·타임아웃·실패/재시도 처리
```

---

## Task 18: 위젯 — 에러 상태 + 재시도

**Files:**
- Modify: `SkillProject/Source/SkillProject/UI/SpyLoadingWidget.h`
- Modify: `SkillProject/Source/SkillProject/UI/SpyLoadingWidget.cpp`

**Interfaces:**
- Consumes: `USpyLoadingSubsystem::OnConnectionFailed`·`RetryConnect`(Task 17)
- Produces: `WBP_Loading`이 바인딩할 `ErrorText`(TextBlock)·`RetryButton`(Button)

- [ ] **Step 1: 헤더에 에러 위젯·핸들러 추가**

`SpyLoadingWidget.h` 를 아래로 교체한다.

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SKUserWidget.h"

#include "SpyLoadingWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UButton;

//# 로딩 화면 뷰 — 서브시스템 진행률을 구독해 바/퍼센트만 갱신한다. 접속 실패 시 에러 UI 노출.
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

	//# 접속 실패 — 에러 메시지 + 재시도 버튼 노출
	void HandleConnectionFailed(const FString& Reason);

	//# 재시도 버튼 클릭
	UFUNCTION()
	void OnRetryClicked();

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UProgressBar> LoadingBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UTextBlock> PercentText;

	//# 접속 실패 메시지 — 평소 숨김(BindWidgetOptional 로 안전)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = true))
	TObjectPtr<UTextBlock> ErrorText;

	//# 재시도 버튼 — 평소 숨김
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = true))
	TObjectPtr<UButton> RetryButton;

	FDelegateHandle ProgressChangedHandle;
	FDelegateHandle ConnectFailedHandle;
};
```

**주의:** `ErrorText`·`RetryButton` 은 `BindWidgetOptional` 이다 — 위젯 BP에 아직 없어도 컴파일이 깨지지 않게 하여 Task 19(에셋 배치)와 순서 의존을 끊는다.

- [ ] **Step 2: cpp 구현 교체**

`SpyLoadingWidget.cpp` 를 아래로 교체한다.

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SpyLoadingWidget.h"

#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Manager/SpyLoadingSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyLoadingWidget)

void USpyLoadingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	//# 에러 UI 는 평소 숨김
	if (IsValid(ErrorText))
	{
		ErrorText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (IsValid(RetryButton))
	{
		RetryButton->SetVisibility(ESlateVisibility::Collapsed);
		RetryButton->OnClicked.AddDynamic(this, &USpyLoadingWidget::OnRetryClicked);
	}

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
	ConnectFailedHandle = LoadingSubsystem->OnConnectionFailed.AddUObject(this, &USpyLoadingWidget::HandleConnectionFailed);

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
			LoadingSubsystem->OnConnectionFailed.Remove(ConnectFailedHandle);
		}
	}

	ProgressChangedHandle.Reset();
	ConnectFailedHandle.Reset();

	if (IsValid(RetryButton))
	{
		RetryButton->OnClicked.RemoveDynamic(this, &USpyLoadingWidget::OnRetryClicked);
	}

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
		PercentText->SetText(FText::FromString(FString::Printf(TEXT("%03d%%"), Percent)));
	}
}

void USpyLoadingWidget::HandleConnectionFailed(const FString& Reason)
{
	//# 실패 시 바·퍼센트를 숨긴다 — "실패했는데 100%" 모순 제거(사용자 결정).
	//# 타임아웃 시 DisplayedProgress 가 0.995(텍스트 100%)에 동결되므로 반드시 감춘다.
	if (IsValid(LoadingBar))
	{
		LoadingBar->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (IsValid(PercentText))
	{
		PercentText->SetVisibility(ESlateVisibility::Collapsed);
	}

	//# 에러 메시지 + 재시도 버튼 노출. 문구는 기획서 §9-3 확정값.
	if (IsValid(ErrorText))
	{
		ErrorText->SetText(FText::FromString(TEXT("서버에 연결하지 못했습니다")));
		ErrorText->SetVisibility(ESlateVisibility::Visible);
	}
	if (IsValid(RetryButton))
	{
		RetryButton->SetVisibility(ESlateVisibility::Visible);
	}
}

void USpyLoadingWidget::OnRetryClicked()
{
	//# 에러 UI 숨기고 바·퍼센트 복원 후 재접속
	if (IsValid(ErrorText))
	{
		ErrorText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (IsValid(RetryButton))
	{
		RetryButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (IsValid(LoadingBar))
	{
		LoadingBar->SetVisibility(ESlateVisibility::Visible);
	}
	if (IsValid(PercentText))
	{
		PercentText->SetVisibility(ESlateVisibility::Visible);
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USpyLoadingSubsystem* LoadingSubsystem = GameInstance->GetSubsystem<USpyLoadingSubsystem>())
		{
			LoadingSubsystem->RetryConnect();
		}
	}
}
```

- [ ] **Step 3: 빌드 검증 (사람 수행)**

빌드.
Expected: 성공. `BindWidgetOptional` 이라 위젯 BP에 아직 `ErrorText`/`RetryButton` 이 없어도 컴파일·기존 동작에 영향 없음.

- [ ] **Step 4: Stage + 커밋 메시지(안)**

```bash
git add SkillProject/Source/SkillProject/UI/SpyLoadingWidget.h \
        SkillProject/Source/SkillProject/UI/SpyLoadingWidget.cpp
```
```
[Feature] SpyLoadingWidget — 접속 실패 에러 메시지·재시도 버튼
```

---

## Task 19: 에셋 — DA 서버 주소 + WBP 에러 위젯 (MCP/에디터)

**TDD 대상 아님.** 에셋·값 작업. 검증은 속성 확인. `save_asset` 은 `only_if_is_dirty=False` 로 강제 저장하고 `.uasset` 타임스탬프로 확인한다(도구가 성공을 반환해도 디스크에 안 쓰일 수 있음).

**Files:**
- Modify(에셋): `/Game/Spy/Data/DA_SpyLoadingConfig` — `ServerAddress`, `ConnectTimeoutSeconds`
- Modify(에셋): `/Game/Spy/UI/WBP_Loading` — `ErrorText`(TextBlock) + `RetryButton`(Button, 자식 TextBlock "재시도")

- [ ] **Step 1: `DA_SpyLoadingConfig` 값 설정 (MCP `execute_python`)**

개발 초기값으로 로컬 데디서버 주소를 넣는다. game-designer가 최종 확정.

```python
import unreal
cfg = unreal.EditorAssetLibrary.load_asset("/Game/Spy/Data/DA_SpyLoadingConfig")
cfg.set_editor_property("ServerAddress", "127.0.0.1:7777")
cfg.set_editor_property("ConnectTimeoutSeconds", 15.0)
unreal.EditorAssetLibrary.save_asset("/Game/Spy/Data/DA_SpyLoadingConfig", only_if_is_dirty=False)
print("ServerAddress =", cfg.get_editor_property("ServerAddress"))
print("ConnectTimeoutSeconds =", cfg.get_editor_property("ConnectTimeoutSeconds"))
```

Expected: `ServerAddress = 127.0.0.1:7777`, `ConnectTimeoutSeconds = 15.0`. `.uasset` 타임스탬프 갱신 확인.

- [ ] **Step 2: `WBP_Loading` 에 에러 위젯 배치 (에디터 수동)**

`WBP_Loading`을 열고 루트 CanvasPanel에 아래를 추가한다. `BindWidgetOptional` 이라 이름이 정확해야 바인딩된다.

| 위젯 | 변수명 | 배치 |
|---|---|---|
| `TextBlock` | **`ErrorText`** | 화면 중앙, 바 위. 초기 텍스트 임의(런타임에 "서버 접속 실패"로 대체) |
| `Button` | **`RetryButton`** | `ErrorText` 아래. 자식으로 `TextBlock` "재시도" |

배치 후 저장 + 컴파일(디자이너에서 1회). 프로그래밍 방식 위젯 트리 편집으로 에디터가 멈춘 이력이 있으니 **손으로 배치**한다.

Expected: 컴파일 시 BindWidget 경고 없음. `ErrorText`/`RetryButton` 이 위젯 변수로 잡힌다.

- [ ] **Step 3: 에셋 검증 (사람 수행)**

- `DA_SpyLoadingConfig` 의 두 값 확인.
- `WBP_Loading` 컴파일 — 에러 없음, 두 위젯 존재.

- [ ] **Step 4: Stage + 커밋 메시지(안)**

```bash
git add SkillProject/Content/Spy/Data/DA_SpyLoadingConfig.uasset \
        SkillProject/Content/Spy/UI/WBP_Loading.uasset
```
```
[Chore] DA_SpyLoadingConfig — 서버 주소·타임아웃 값 + 로딩 위젯 에러 UI
```

---

## Task 20: 통합 검증 (수동, 데디서버)

**Files:** 없음. 문제 발견 시 해당 Task 로 복귀.

**⚠ PIE "2인"은 데디서버의 실제 실행이 아니므로 사용하지 않는다.** 데디서버를 별도 프로세스로 띄우고 클라를 실행한다.

- [ ] **Step 0: 검증 경로 확정 — 데디서버 (사람 수행, 필수 선행)**

**확인된 사실:** 이 프로젝트에는 패키지 서버 타깃(`SkillProjectServer.Target.cs`)이 **없다**(현재 타깃은 `SkillProject.Target.cs`·`SkillProjectEditor.Target.cs` 뿐). 따라서 독립 `SkillProjectServer.exe` 빌드는 지금 불가능하다.

- **이 기능의 정식 검증은 에디터 내 데디서버로 한다** — Play 설정 Net Mode `Play As Client` + **Run Dedicated Server** 체크 + Number of Players 1(클라 1). 이 모드는 별도 서버 빌드 없이 **진짜 데디케이티드 서버 월드**(헤드리스, `IsRunningDedicatedServer` true)를 띄우고 클라가 접속한다. "리슨서버 2인"과 달리 서버/클라가 분리돼 이 기능의 코드 경로(`ShouldCreateSubsystem` 서버 차단, 클라 `ClientTravel` 접속)를 정확히 탄다.
- 패키지 서버 실행 파일(`SkillProjectServer.exe`)이 필요하면 **서버 타깃 생성은 이 플랜 범위 밖의 별도 작업**이다. Step 1~4의 로그 확인만으로 이 기능 검증은 충분하다.
- 아래 Step 1~4는 "에디터 데디서버 + 클라" 기준으로 읽는다.

- [ ] **Step 1: 데디서버 실행 (사람 수행)**

에디터 또는 커맨드라인으로 데디서버를 DevMap으로 띄운다. 예(에디터): Play 설정 Net Mode `Play As Client` + Run Dedicated Server, 또는 빌드 후:
```
SkillProjectServer.exe /Game/Spy/Maps/DevMap -log -port=7777
```
Expected: 서버가 DevMap 상주. 서버 로그에 `SpyLoadingSubsystem` 로그 없음(`ShouldCreateSubsystem` false).

- [ ] **Step 2: 클라이언트 자동 접속 (사람 수행)**

클라를 실행(부팅 → LoadingMap → 자동 접속).
Expected:
- 로딩바가 즉시 뜨고 프리로드로 ~90%까지 참
- `# [SpyLoadingSubsystem] 서버 접속 개시: 127.0.0.1:7777` 로그
- 접속 단계에서 바가 90%→99% 부근까지 **계속 움직임**(멈추지 않음)
- 서버 합류·DevMap 로드 → `# [SpyLoadingSubsystem] 도착 — 로딩 UI 종료`
- **검은 화면·바 100% 고정 없음.** 로딩 UI가 걷히고 게임플레이 진입.

- [ ] **Step 3: 접속 실패 → 재시도 (사람 수행)**

서버를 끈 상태로 클라만 실행.
Expected:
- 접속 개시 후 타임아웃(또는 즉시 네트워크 실패) → `# [SpyLoadingSubsystem] 접속 실패: ...` 로그
- 로딩 화면에 **"서버 접속 실패" + 재시도 버튼** 노출
- 서버를 켠 뒤 재시도 버튼 클릭 → `접속 재시도` 로그 → 정상 접속·진입

**검증 포인트 ① (불확실 — 반드시 확인):** `RetryConnect` 는 `GetFirstLocalPlayerController()` 가 실패 후에도 유효하다고 가정한다. 그러나 네트워크 실패 후 클라의 월드/PC 상태는 불확실하다(실패 시 `?closed` 폴백 브라우즈가 일어날 수 있음). 재시도 클릭 시 `RetryConnect` 로그에 `PlayerController 가 없습니다` 가 찍히면 이 가정이 깨진 것이다 — 이 경우 Task 17로 돌아가 재접속을 `GEngine->Browse` 또는 실패 후 유효한 PC 재획득 방식으로 바꾼다.

**검증 포인트 ② (code-review MINOR — 함께 관찰):** `HandlePostLoadMap` 의 도착 필터는 "로딩맵 정확 일치"만 배제한다. 접속 **실패 후** 엔진이 로딩맵이 아닌 중간/폴백 월드(transition/entry 등)를 로드하며 `PostLoadMapWithWorld` 를 쏘면, 이름이 달라 필터를 통과해 "도착"으로 오판 → **로딩 UI가 조기에 걷힐 수 있다.** 실패 순간 로딩 화면(에러 메시지)이 유지되는지, 아니면 잠깐 사라졌다 나타나는지 관찰한다. 조기 종료가 관찰되면 Task 16으로 돌아가 도착 판정을 "게임플레이 맵 정확 일치"(로딩맵 배제가 아니라 대상 맵 일치)로 강화한다.

- [ ] **Step 4: 오프라인 폴백 (사람 수행)**

`DA_SpyLoadingConfig` 의 `ServerAddress` 를 임시로 비우고 단일 클라(Standalone, 1인)로 실행.
Expected: `# [SpyLoadingSubsystem] 오프라인 전환(Standalone)` → `OpenLevel(DevMap)` → 정상 진입. 검증 후 `ServerAddress` 복구.

- [ ] **Step 5: 회귀 — Automation 전체 (사람 수행)**

`SkillProject.Manager.Loading` 24건 실행.
Expected: 전건 PASS.

---

## Task 커버리지 — 스펙 대조

| 스펙 항목 | 담당 Task |
|---|---|
| §5-1 전환 `OpenLevel`→`ClientTravel` | Task 16 Step 5 |
| §5-1 오프라인 폴백 Standalone 가드 + 클라 OpenLevel 금지 | Task 16 Step 5 |
| §5-2 접속 단계 진행률(시간 페이싱, 1.0 미도달) | Task 15(`ConnectPhaseDisplayed`) + Task 17 Step 4 |
| §5-2 `GetMapPercent` 불변(오프라인만 기존 경로) | 변경 없음 — 접속 모드는 타이머가 진행률 담당 |
| §5-2 도착 판정 맵 이름 필터 | Task 15(`IsLoadingMapName`) + Task 16 Step 7 |
| §5-3 실패 델리게이트·타임아웃 | Task 17 Step 3~5 |
| §5-3 재시도 | Task 17 Step 6 |
| §5-4 위젯 에러 상태 | Task 18 |
| §5-5 Config 필드 | Task 14 |
| §7 예외 표 각 행 | Task 16 Step 5(폴백/금지/PC없음), Task 17(실패), Task 15/16(도착) |
| §8 Automation 순수 함수 4종 | Task 15 |
| §8 수동 데디서버 검증 | Task 20 |
| §9 범위 밖 | 어떤 Task 도 구현하지 않음 |

---

## Task 21: 데모 "접속" 버튼 — 자동 전환을 수동 버튼으로 (설계 피벗)

**배경:** 데디서버 접속 로딩은 세션 인프라(메뉴·서버 타깃·클라 standalone 부팅)가 없어 PIE로 검증 불가하고 매 구성에서 깨진다(PIE 데디서버는 LoadingMap을 서버로 띄우고 `IsRunningDedicatedServer()`가 PIE에서 false라 서버에서 서브시스템이 돌며 PC-null·뷰포트 없음). **사용자 결정: 로딩과 접속을 분리하고, 자동 전환 대신 "접속" 버튼을 눌러 `OpenLevel(DevMap)`으로 넘어가는 데모 흐름으로 전환한다.**

이 Task 는 Task 14~19 산출물 위에서 **자동 전환 발화를 버튼 발화로 교체**한다. 전환 로직 자체(`TransitionToGameplayMap`의 offline `OpenLevel`/connect `ClientTravel` 분기)는 그대로 두므로, `ServerAddress`를 비우면 데모(standalone `OpenLevel(DevMap)`), 나중에 채우면 접속 — **전방 호환**된다. 네트워킹 코드·24개 테스트는 유지(휴면).

**Files:**
- Modify: `SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.h/.cpp`
- Modify: `SkillProject/Source/SkillProject/UI/SpyLoadingWidget.h/.cpp`
- Modify(에셋): `/Game/Spy/Data/DA_SpyLoadingConfig` — `ServerAddress` 비움 (MCP, 메인)
- Modify(에셋): `/Game/Spy/UI/WBP_Loading` — `EnterButton` 추가 (MCP/에디터, 메인)

**Interfaces:**
- Consumes: 기존 `TransitionToGameplayMap`(변경 없음), `ShouldTransition`
- Produces:
  - `FOnLoadingReadyToEnter OnReadyToEnter` — 준비 완료 알림(파라미터 없음)
  - `void USpyLoadingSubsystem::EnterGameplay()` — 버튼이 호출, 전환 개시
  - `USpyLoadingWidget::EnterButton`(`BindWidgetOptional`)

- [ ] **Step 1: 헤더에 준비 델리게이트·상태·메서드 추가**

`SpyLoadingSubsystem.h`의 진행률 델리게이트 아래에 추가한다.

```cpp
//# 접속 실패 알림(사유 문자열) — 위젯이 에러 UI 를 띄운다
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLoadingConnectFailed, const FString&);

//# 로딩 완료(전환 준비) 알림 — 위젯이 "접속" 버튼을 띄운다. 자동 전환하지 않는다(데모).
DECLARE_MULTICAST_DELEGATE(FOnLoadingReadyToEnter);
```

public 델리게이트 블록에 추가한다.

```cpp
	FOnLoadingConnectFailed OnConnectionFailed;
	FOnLoadingReadyToEnter OnReadyToEnter;
```

public 메서드(`RetryConnect` 근처)에 추가한다.

```cpp
	//# "접속" 버튼이 호출 — 로딩 완료 후 게임플레이 맵으로 전환 개시
	void EnterGameplay();
```

멤버 블록에 상태 플래그를 추가한다.

```cpp
	//# 로딩 완료(버튼 대기) 여부 — 자동 전환 대신 버튼을 기다린다
	bool bReadyToEnter = false;
```

- [ ] **Step 2: `Tick` 을 자동 전환 → 준비 브로드캐스트로 교체**

`SpyLoadingSubsystem.cpp` `Tick`의 전환 트리거 블록:

```cpp
	if (bTransitionStarted == false && bReadyToTransition)
	{
		TransitionToGameplayMap();
	}
```

을 아래로 교체한다.

```cpp
	//# 준비 완료 — 자동 전환하지 않고 "접속" 버튼을 띄운다(데모). 버튼이 EnterGameplay() 를 호출한다.
	if (bReadyToEnter == false && bReadyToTransition)
	{
		bReadyToEnter = true;

		//# 로딩 단계 틱 종료 — 이후는 버튼 입력 대기
		bLoading = false;

		//# 바를 100% 로 확정
		if (DisplayedProgress < 1.f)
		{
			DisplayedProgress = 1.f;
			OnProgressChanged.Broadcast(DisplayedProgress);
		}

		OnReadyToEnter.Broadcast();
		UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 로딩 완료 — 접속 버튼 대기"));
	}
```

- [ ] **Step 3: `EnterGameplay` 구현**

`TransitionToGameplayMap` 구현 위에 추가한다. 준비 완료 상태에서만, 한 번만 전환한다.

```cpp
void USpyLoadingSubsystem::EnterGameplay()
{
	//# 로딩이 끝나 버튼이 떠 있을 때만, 중복 없이 전환한다
	if (bReadyToEnter == false || bTransitionStarted)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 접속 버튼 입력 — 전환 개시"));
	TransitionToGameplayMap();
}
```

`TransitionToGameplayMap`은 변경하지 않는다 — `ServerAddress` 비어 있고 `NM_Standalone`이면 `OpenLevel(DevMap)`(데모), 채워져 있으면 `ClientTravel`(전방 호환).

- [ ] **Step 4: `Deinitialize`에 상태 리셋 추가**

`Deinitialize`의 `bConnecting = false;` 근처에 추가한다.

```cpp
	bLoading = false;
	bConnecting = false;
	bReadyToEnter = false;
	OnProgressChanged.Clear();
	OnConnectionFailed.Clear();
	OnReadyToEnter.Clear();
```

- [ ] **Step 5: 위젯에 `EnterButton` 추가**

`SpyLoadingWidget.h`에 전방 선언(`class UButton;`은 이미 있음) 확인 후, 멤버·핸들러를 추가한다.

```cpp
	//# 접속 실패 — 에러 메시지 + 재시도 버튼 노출
	void HandleConnectionFailed(const FString& Reason);

	//# 로딩 완료 — "접속" 버튼 노출
	void HandleReadyToEnter();

	//# 재시도 버튼 클릭
	UFUNCTION()
	void OnRetryClicked();

	//# 접속 버튼 클릭
	UFUNCTION()
	void OnEnterClicked();
```

멤버에 버튼·핸들 추가:

```cpp
	//# 로딩 완료 후 게임 진입 버튼 — 평소 숨김
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = true))
	TObjectPtr<UButton> EnterButton;

	FDelegateHandle ProgressChangedHandle;
	FDelegateHandle ConnectFailedHandle;
	FDelegateHandle ReadyToEnterHandle;
```

- [ ] **Step 6: 위젯 cpp — 구독·핸들러·클릭**

`NativeConstruct`에 `EnterButton` 초기 숨김·구독·클릭 바인딩을 추가한다(기존 코드 유지, 아래 추가).

```cpp
	if (IsValid(EnterButton))
	{
		EnterButton->SetVisibility(ESlateVisibility::Collapsed);
		EnterButton->OnClicked.AddDynamic(this, &USpyLoadingWidget::OnEnterClicked);
	}
```

그리고 서브시스템 구독 블록에 추가:

```cpp
	ReadyToEnterHandle = LoadingSubsystem->OnReadyToEnter.AddUObject(this, &USpyLoadingWidget::HandleReadyToEnter);
```

`NativeDestruct`에 해제 추가:

```cpp
			LoadingSubsystem->OnReadyToEnter.Remove(ReadyToEnterHandle);
```
```cpp
	ReadyToEnterHandle.Reset();

	if (IsValid(EnterButton))
	{
		EnterButton->OnClicked.RemoveDynamic(this, &USpyLoadingWidget::OnEnterClicked);
	}
```

핸들러 2개 추가:

```cpp
void USpyLoadingWidget::HandleReadyToEnter()
{
	//# 로딩 완료 — "접속" 버튼 노출
	if (IsValid(EnterButton))
	{
		EnterButton->SetVisibility(ESlateVisibility::Visible);
	}
}

void USpyLoadingWidget::OnEnterClicked()
{
	//# 버튼 숨기고 전환 개시
	if (IsValid(EnterButton))
	{
		EnterButton->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USpyLoadingSubsystem* LoadingSubsystem = GameInstance->GetSubsystem<USpyLoadingSubsystem>())
		{
			LoadingSubsystem->EnterGameplay();
		}
	}
}
```

- [ ] **Step 7: 빌드 + 회귀 (사람 수행)**

에디터 종료 → VS 빌드 → Automation `SkillProject.Manager.Loading`.
Expected: **24건 PASS**(순수 함수 불변). 컴파일 성공.

- [ ] **Step 8: Stage + 커밋 메시지(안)**

```bash
git add SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.h \
        SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.cpp \
        SkillProject/Source/SkillProject/UI/SpyLoadingWidget.h \
        SkillProject/Source/SkillProject/UI/SpyLoadingWidget.cpp
```
```
[Feature] SpyLoadingSubsystem — 자동 전환을 "접속" 버튼 입력으로 전환(데모)
```

---

## Task 22: 데모 에셋 — ServerAddress 비움 + EnterButton (MCP/에디터, 메인)

- [ ] **Step 1: `DA_SpyLoadingConfig` ServerAddress 비움** — 데모=오프라인(`OpenLevel(DevMap)`). MCP `set_editor_property("ServerAddress", "")` + `save_asset(only_if_is_dirty=False)`.
- [ ] **Step 2: `WBP_Loading`에 `EnterButton`(Button, 자식 TextBlock "접속") 추가** — 중앙(0.5,0.5), 크기 220×52, RetryButton 과 같은 스타일(Normal `#1A1F24`/Hovered `#5AC8D8`/Pressed `#48A0AD`, 라벨 Roboto 20). z-order 맨 뒤(맨 위). `save_asset(only_if_is_dirty=False)`.
- [ ] **Step 3: `WBP_Loading` 에디터에서 컴파일 1회** (사람) — BindWidget 검증.

## Task 23: 데모 검증 (수동, standalone)

- [ ] **Step 1 (사람):** Play 설정 **Net Mode: Play Standalone, Number of Players 1, Run Dedicated Server 해제** → 실행.
- Expected: LoadingMap → 로딩바 0→100% (프리로드+페이싱) → "접속" 버튼 등장 → 클릭 → `# [SpyLoadingSubsystem] 접속 버튼 입력 — 전환 개시` + `오프라인 전환(Standalone)` → DevMap 진입, 캐릭터 스폰. **검은 화면·PC-null 없음.**

---

## Task 24: 데디서버 바이패스 — 서버는 로딩 건너뛰고 DevMap 서버 트래블

**배경:** 버튼 피벗(Task 21)으로 자동 전환이 사라지면서, 데디케이티드 서버가 LoadingMap 에 갇힌다 — 헤드리스라 뷰포트·버튼이 없어 전환을 못 한다(로그: `뷰포트가 없어 Persistent UI 를 열지 않습니다` → `로딩 완료 — 접속 버튼 대기` 이후 정지). 실제 `-server` 실행은 `ServerDefaultMap=DevMap` 이라 LoadingMap 을 안 거치지만, PIE "Run Dedicated Server" 는 현재 맵(LoadingMap)을 서버에 띄워 이 문제가 난다. **데디서버는 로딩 화면이 무의미하므로 로딩맵에 떨어지면 게임플레이 맵으로 서버 트래블한다.**

**Files:**
- Modify: `SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.cpp` (`StartLoading`)

**Interfaces:**
- Consumes: `ApplyConfig`(→ `MapPackageName`), `UWorld::GetNetMode`/`ServerTravel`
- Produces: 없음(동작 변경만)

- [ ] **Step 1: `StartLoading` 에 데디서버 가드 추가**

`StartLoading` 의 `ApplyConfig` 성공 체크 **직후**(에셋 프리로드 시작 전)에 추가한다. `IsRunningDedicatedServer()` 는 PIE 에서 false 이므로 **월드 넷모드**로 판정한다.

```cpp
	const USpyLoadingConfig* Config = USpyAssetManager::GetAssetByName<USpyLoadingConfig>(SpyAssetNames::LoadingConfig);
	if (ApplyConfig(Config) == false)
	{
		//# Config 이상 — 임의 맵 이름으로 fallback 하지 않는다 (하드코딩 금지)
		return;
	}

	//# 데디케이티드 서버는 헤드리스라 로딩 화면이 없다. 로딩맵에 떨어졌으면 게임플레이 맵으로 서버 트래블한다.
	//# (IsRunningDedicatedServer() 는 PIE 에서 false 이므로 월드 넷모드로 판정)
	if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		if (World->GetNetMode() == NM_DedicatedServer)
		{
			UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 데디서버 — 로딩 생략, 게임플레이 맵 서버 트래블: %s"), *MapPackageName.ToString());
			World->ServerTravel(MapPackageName.ToString());
			return;
		}
	}
```

**주의:** `ServerTravel` 을 `BeginPlay` 흐름(StartLoading 호출부)에서 바로 부르는 게 불안정하면(월드 초기화 중 트래블 거부), `World->GetTimerManager().SetTimerForNextTick(...)` 으로 다음 틱에 `ServerTravel` 하도록 지연한다. Task 24 검증에서 서버가 실제로 DevMap 으로 넘어가는지 로그로 확인하고, 안 넘어가면 지연 방식으로 바꾼다.

- [ ] **Step 2: 빌드 (사람)**

에디터 종료 → VS 빌드. 신규 `.cpp` 없어 프로젝트 재생성 불필요, 헤더 변경 없어 Live Coding 도 가능(단 안전하게 전체 빌드 권장).

- [ ] **Step 3: 검증 (사람)**

(a) **Play Standalone 1인** — 기존 데모 흐름(로딩바 → 접속 버튼 → DevMap) 그대로 동작하는지(회귀 없음).
(b) **PIE "Run Dedicated Server"** — 서버 로그에 `데디서버 — 로딩 생략, 게임플레이 맵 서버 트래블: /Game/Spy/Maps/DevMap` → 서버가 DevMap 으로 넘어가고 **LoadingMap 에 갇히지 않는지.** `로딩 완료 — 접속 버튼 대기` 가 서버에서 안 뜨는지.

- [ ] **Step 4: Stage + 커밋 메시지(안)**

```bash
git add SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.cpp
```
```
[Fix] SpyLoadingSubsystem — 데디서버는 로딩 생략하고 DevMap 서버 트래블
```

---

## Task 25: 데모 바 99/1 재배치 — 버튼은 99%, 마지막 1%는 버튼 뒤

**사용자 결정:** 바를 **99% 에셋/맵 로딩 + 1% 로컬 마무리**로 나눈다. 에셋 로딩이 바를 99%까지 채우고, "접속" 버튼이 **99% 지점**에 뜨고, 누르면 마지막 1%(로컬 DevMap 오픈)가 채워지며 진입. 서버 없이 `Play Standalone` 으로 검증. `AssetPhaseWeight=0.99`(DA 반영 완료), `ServerAddress` 비움(오프라인).

**문제:** 현재 오프라인 모드는 `HandleAssetPhaseComplete` 에서 로컬 `LoadPackageAsync`(phase 2)를 **자동** 실행해 바가 버튼 전에 100%까지 찬다. phase 2 를 버튼 뒤로 미뤄야 한다.

**Files:**
- Modify: `SkillProject/Source/SkillProject/Data/SpyLoadingConfig.h` — C++ 기본값 `AssetPhaseWeight = 0.99f`
- Modify: `SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.cpp`

- [ ] **Step 1: C++ 기본값 0.9 → 0.99**

`SpyLoadingConfig.h` 의 `AssetPhaseWeight = 0.9f` → `= 0.99f` (DA 확정값과 일치, 리뷰어 오탐 방지).

- [ ] **Step 2: `HandleAssetPhaseComplete` — 오프라인도 자동 phase 2 생략**

현재 오프라인 분기가 `LoadPackageAsync`(phase 2)를 자동 실행한다. 이를 제거해 **양 모드 모두 자동 phase 2 를 돌리지 않게** 한다. 함수는 `bAssetPhaseComplete = true;` 만 하고 반환한다.

```cpp
void USpyLoadingSubsystem::HandleAssetPhaseComplete()
{
	//# 자동 phase 2 를 돌리지 않는다. 오프라인 맵 로드는 "접속" 버튼(EnterGameplay) 뒤로 미룬다.
	//# 접속 모드도 로컬 스트리밍을 하지 않고 ClientTravel 로 서버에서 받는다.
	bAssetPhaseComplete = true;
}
```

- [ ] **Step 3: `Tick` 준비 조건 통일 — 에셋 완료 + 최소 시간**

`Tick` 의 `bReadyToTransition` 계산에서 `ShouldConnectToServer` 분기를 없애고, 양 모드 모두 **에셋 완료 + 최소 표시 시간**으로 통일한다. (오프라인도 phase 2 를 안 돌리므로 `Raw` 가 `AssetPhaseWeight`(0.99)를 못 넘어 기존 `ShouldTransition(Raw>=1.0)` 이 영원히 거짓이기 때문.)

```cpp
	//# 준비 조건 — 양 모드 모두 에셋 프리로드 완료 + 최소 표시 시간(바는 AssetPhaseWeight=0.99 에서 멈춰 버튼을 기다린다)
	const bool bReadyToTransition = bAssetPhaseComplete && ElapsedSeconds >= LoadingConfig->MinDisplaySeconds;
```

- [ ] **Step 4: 준비 시 바를 100% 로 강제하지 않는다**

Task 21 의 준비 블록에서 `DisplayedProgress = 1.f` 강제를 제거한다. 바가 프리로드 지점(0.99)에 머물러야 버튼이 99%에서 뜬다.

```cpp
	if (bReadyToEnter == false && bReadyToTransition)
	{
		bReadyToEnter = true;
		bLoading = false;

		//# 바를 100% 로 강제하지 않는다 — 마지막 1% 는 버튼 입력 후 전환에서 채운다
		OnReadyToEnter.Broadcast();
		UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 로딩 완료(99%) — 접속 버튼 대기"));
	}
```

- [ ] **Step 5: 검증 흐름**

`EnterGameplay` → `TransitionToGameplayMap` 는 그대로. 오프라인이면 `TransitionToGameplayMap` 이 `OpenLevel` 직전에 `DisplayedProgress = 1.f` 로 강제(기존 코드) → 마지막 1% 가 버튼 입력 시 채워지고 DevMap 진입. 접속 모드(dormant)는 `ClientTravel` + `ConnectPhaseDisplayed` 가 0.99→1.0 을 채운다(변경 없음).

- [ ] **Step 6: 빌드 + Automation (사람)**

에디터 종료 → VS 빌드 → `SkillProject.Manager.Loading` 24건. Expected: **24건 PASS**(순수 함수·`ConnectPhaseDisplayed` 등 불변; `CombineDesignWeightPlateau` 는 상수 인자를 쓰므로 `AssetPhaseWeight` 기본값 변경과 무관).

- [ ] **Step 7: Stage + 커밋 메시지(안)**

```bash
git add SkillProject/Source/SkillProject/Data/SpyLoadingConfig.h \
        SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.cpp
```
```
[Feature] SpyLoadingSubsystem — 데모 바 99/1, 맵 로드를 접속 버튼 뒤로 이동
```

## Task 26: 데모 최종 검증 (사람, Play Standalone)

- **Play Standalone, 1인, 데디서버 해제** → LoadingMap → 바 0→99%(에셋) → "접속" 버튼 등장 → 클릭 → 마지막 1% + DevMap 진입.
- 로그: `로딩 완료(99%) — 접속 버튼 대기` → 클릭 → `접속 버튼 입력 — 전환 개시` → `오프라인 전환(Standalone): DevMap` → `New Opening UI: MainHUD` → `Close Persistent UI: Loading`.
- **참고:** 에셋을 프리로드하므로 마지막 1%(로컬 맵 오픈)는 사실상 즉시다 — 99%에서 클릭하면 바로 진입한다. 긴 "두 번째 로딩바"는 프리로드 때문에 나타나지 않는다(의도된 동작 — 프리로드가 맵을 미리 올려서 전환이 빠름).

---

## Task 27: 데모 2단계 로딩바 — 에셋 바 → 접속 버튼 → 맵 바 (서버 접속 제거)

**사용자 최종 결정:** 서버 접속을 완전히 뺀다. 로딩바 **두 개**: ① 에셋 로딩바(0→100%) → "접속" 버튼 → ② 맵 로딩바(0→100%) → DevMap. 전부 로컬, `Play Standalone` 검증. `ServerAddress` 비움 유지, `ClientTravel`/접속 코드는 휴면(제거하지 않음). `AssetPhaseWeight` 는 이제 미사용(각 단계가 독립 0→100% 바) — 값은 두되 dormant.

**핵심:** 지금은 EnterGameplay 가 바로 `TransitionToGameplayMap`(OpenLevel) 한다. 이를 **버튼 → phase 2(맵 로딩바) 시작 → 맵 로드 완료 시 OpenLevel** 로 바꾼다. 버튼 누르면 바가 0으로 리셋되고 맵 바가 다시 찬다.

**Files:**
- Modify: `SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.h` (`bMapPhase` 멤버)
- Modify: `SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.cpp` (Tick 2단계, EnterGameplay)

- [ ] **Step 1: 헤더에 phase 2 플래그**

`SpyLoadingSubsystem.h` 멤버 블록(`bReadyToEnter` 근처)에 추가:

```cpp
	//# 맵 로딩 단계(phase 2) 활성 — "접속" 버튼 입력 후 DevMap 로드 바를 그린다
	bool bMapPhase = false;
```

- [ ] **Step 2: `Tick` 을 2단계로 재구성**

기존 `Tick` 전체를 아래로 교체한다. phase 1(에셋)과 phase 2(맵)를 분리하고, 각각 `ClampDisplayed` 로 시간 페이싱한다.

```cpp
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

	//# ── phase 2: 맵 로딩바 (접속 버튼 입력 후) ──
	if (bMapPhase)
	{
		const float MapRatio = FMath::Max(GetMapPercent(), 0.f) / 100.f;
		const float NewDisplayed = ClampDisplayed(MapRatio, ElapsedSeconds, LoadingConfig->MinDisplaySeconds);
		if (NewDisplayed > DisplayedProgress)
		{
			DisplayedProgress = NewDisplayed;
			OnProgressChanged.Broadcast(DisplayedProgress);
		}

		//# 맵 로드 완료 + 최소 표시 시간 → 전환(OpenLevel)
		if (bMapLoadComplete && ElapsedSeconds >= LoadingConfig->MinDisplaySeconds && bTransitionStarted == false)
		{
			TransitionToGameplayMap();
		}
		return;
	}

	//# ── phase 1: 에셋 로딩바 ──
	const float AssetRatio = (AssetTotalCount > 0) ? ((float)AssetLoadedCount / (float)AssetTotalCount) : 1.f;
	const float NewDisplayed = ClampDisplayed(AssetRatio, ElapsedSeconds, LoadingConfig->MinDisplaySeconds);
	if (NewDisplayed > DisplayedProgress)
	{
		DisplayedProgress = NewDisplayed;
		OnProgressChanged.Broadcast(DisplayedProgress);
	}

	//# 에셋 완료 + 최소 표시 시간 → "접속" 버튼 대기(자동 전환 안 함)
	if (bReadyToEnter == false && bAssetPhaseComplete && ElapsedSeconds >= LoadingConfig->MinDisplaySeconds)
	{
		bReadyToEnter = true;
		bLoading = false;

		//# phase 1 바를 100% 로 확정
		if (DisplayedProgress < 1.f)
		{
			DisplayedProgress = 1.f;
			OnProgressChanged.Broadcast(DisplayedProgress);
		}

		OnReadyToEnter.Broadcast();
		UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 에셋 로딩 완료 — 접속 버튼 대기"));
	}
}
```

- [ ] **Step 3: `EnterGameplay` 을 phase 2 시작으로 교체**

기존 `EnterGameplay` 를 아래로 교체한다. 즉시 전환하지 않고 맵 로딩바(phase 2)를 시작한다 — 바를 0 으로 리셋하고 `LoadPackageAsync(DevMap)` 를 돌린다.

```cpp
void USpyLoadingSubsystem::EnterGameplay()
{
	//# 에셋 로딩이 끝나 버튼이 떠 있을 때만, 중복 없이
	if (bReadyToEnter == false || bMapPhase)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 접속 버튼 입력 — 맵 로딩 시작"));

	//# phase 2 진입 — 바를 0 으로 리셋해 맵 로딩바가 새로 차오르게 한다
	bReadyToEnter = false;
	bMapPhase = true;
	ElapsedSeconds = 0.f;
	DisplayedProgress = 0.f;
	OnProgressChanged.Broadcast(DisplayedProgress);

	//# 맵 패키지 비동기 로드 시작
	bMapLoadComplete = false;
	const int32 RequestId = LoadPackageAsync(
		MapPackageName.ToString(),
		FLoadPackageAsyncDelegate::CreateUObject(this, &USpyLoadingSubsystem::HandleMapPackageLoaded));
	if (RequestId == INDEX_NONE)
	{
		UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] 맵 패키지 로드 요청 실패: %s"), *MapPackageName.ToString());
		bMapLoadComplete = true;
	}

	//# 틱 재개 — phase 2 바 구동
	bLoading = true;
}
```

- [ ] **Step 4: `Deinitialize` 에 `bMapPhase` 리셋**

`Deinitialize` 의 `bReadyToEnter = false;` 근처에 추가:

```cpp
	bReadyToEnter = false;
	bMapPhase = false;
```

- [ ] **Step 5: 확인 — 무변경 유지**

`TransitionToGameplayMap`(offline OpenLevel + 100% 강제), `HandlePostLoadMap`(도착 시 UI 종료), `HandleMapPackageLoaded`(bMapLoadComplete), `GetMapPercent`, 순수 함수 4종, 위젯, 데디서버 가드(Task 24), 기존 24 테스트 — 전부 무변경. `CombineProgress`/`AssetPhaseWeight`/`ConnectPhaseDisplayed`/`ClientTravel` 은 이제 Tick 에서 미사용이지만 **제거하지 않는다**(휴면, 테스트 유지). `HandleAssetPhaseComplete`(Task 25: bAssetPhaseComplete=true 만) 그대로.

- [ ] **Step 6: 빌드 + Automation (사람)**

에디터 종료 → VS 빌드 → `SkillProject.Manager.Loading` 24건. Expected: **24건 PASS**(순수 함수 무변경 — Tick 이 CombineProgress 대신 asset/map 비율을 직접 쓰지만 함수 자체와 테스트는 그대로).

- [ ] **Step 7: Stage + 커밋 메시지(안)**

```bash
git add SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.h \
        SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.cpp
```
```
[Feature] SpyLoadingSubsystem — 2단계 로딩바(에셋→버튼→맵), 서버 접속 제거
```

## Task 28: 데모 검증 (사람, Play Standalone)

- **Play Standalone, 1인, 데디서버 해제** → LoadingMap → **에셋 로딩바 0→100%** → "접속" 버튼 → 클릭 → **맵 로딩바 0→100%** → DevMap 진입.
- 로그: `에셋 로딩 완료 — 접속 버튼 대기` → (클릭) `접속 버튼 입력 — 맵 로딩 시작` → `오프라인 전환(Standalone): DevMap` → `New Opening UI: MainHUD` → `Close Persistent UI: Loading`.
- **참고:** 맵 콘텐츠가 에셋 단계에서 이미 로드됐으면 맵 바는 `MinDisplaySeconds` 시간에 맞춰 채워진다(실측 로드가 빨라도 최소 표시 시간만큼 보인다).

---

## Task 29: 접속 버튼 대기 중 로딩바·퍼센트 숨김

**사용자 요청:** phase 1 완료 후 "접속" 버튼이 떠 있는 동안(버튼 대기 상태)에는 `LoadingBar`·`PercentText` 를 숨기고 버튼만 보이게. 버튼 클릭 → phase 2(맵 로딩바) 시작 시 다시 보이게.

**Files:**
- Modify: `SkillProject/Source/SkillProject/UI/SpyLoadingWidget.cpp` (`HandleReadyToEnter`, `OnEnterClicked`)

- [ ] **Step 1: `HandleReadyToEnter` — 버튼 노출 + 바·퍼센트 숨김**

```cpp
void USpyLoadingWidget::HandleReadyToEnter()
{
	//# 로딩 완료 — 바·퍼센트를 숨기고 "접속" 버튼만 보인다
	if (IsValid(LoadingBar))
	{
		LoadingBar->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (IsValid(PercentText))
	{
		PercentText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (IsValid(EnterButton))
	{
		EnterButton->SetVisibility(ESlateVisibility::Visible);
	}
}
```

- [ ] **Step 2: `OnEnterClicked` — 버튼 숨김 + 바·퍼센트 복원 후 phase 2**

기존 `OnEnterClicked` 에서 버튼 숨김 뒤, `EnterGameplay()` 호출 **전에** 바·퍼센트를 다시 보이게 한다(맵 로딩바가 다시 차오르도록).

```cpp
void USpyLoadingWidget::OnEnterClicked()
{
	//# 버튼 숨기고 바·퍼센트 복원 — 맵 로딩바가 다시 차오른다
	if (IsValid(EnterButton))
	{
		EnterButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (IsValid(LoadingBar))
	{
		LoadingBar->SetVisibility(ESlateVisibility::Visible);
	}
	if (IsValid(PercentText))
	{
		PercentText->SetVisibility(ESlateVisibility::Visible);
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USpyLoadingSubsystem* LoadingSubsystem = GameInstance->GetSubsystem<USpyLoadingSubsystem>())
		{
			LoadingSubsystem->EnterGameplay();
		}
	}
}
```

- [ ] **Step 3: 빌드 + 검증 (사람)**

Play Standalone → 에셋바 0→100% → (바·퍼센트 사라지고) **접속 버튼만** → 클릭 → (바·퍼센트 다시 뜨고) 맵바 0→100% → DevMap.

- [ ] **Step 4: Stage + 커밋 메시지(안)**

```bash
git add SkillProject/Source/SkillProject/UI/SpyLoadingWidget.cpp
```
```
[Feature] SpyLoadingWidget — 접속 버튼 대기 중 로딩바·퍼센트 숨김
```
