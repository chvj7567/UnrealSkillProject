# OSS 세션 브라우저 (방 목록) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 로딩 완료 후 방(세션) 목록을 띄우고, 방을 만들거나 골라 들어가 인게임 맵에서 함께 플레이한다.

**Architecture:** 세션 로직을 프로젝트 비의존 플러그인 `SKOnline`(UI 없음, 게임 모듈 역참조 없음)으로 분리한다. 플러그인은 조인 결과로 **접속 문자열**까지만 돌려주고, 트래블·UI 는 게임 모듈이 맡는다. 게임 모듈은 이미 구현된 로딩→접속 파이프라인(접속 감시·타임아웃·재시도·persistent 로딩 UI)을 그대로 재사용하고, **주소가 어디서 오는가**만 바꾼다. 호스트는 리슨 서버(`?listen`)로 자기 월드를 연다.

**Tech Stack:** Unreal Engine 5.7 / C++ / OnlineSubsystem (Null → 추후 Steam) / UMG / Unreal Automation

## Global Constraints

- **spec**: `docs/superpowers/specs/2026-07-28-oss-session-browser-design.md` — 이 플랜은 그 스펙의 §4 아키텍처·§5 흐름·§6 에러 처리를 구현한다. 구조가 충돌하면 스펙이 우선한다.
- **코딩 룰**: `.claude/rules/cpp-style.md` (**2026-07-28 개정 — 반드시 현재 버전을 읽을 것**). 이 플랜의 코드 샘플은 개정판 기준으로 작성됐다:
  - §4 한 줄 주석은 `//#`, **한 블록 2줄 이내**
  - §5 **가드 절(즉시 return/continue)은 중괄호 없이 개행+들여쓰기.** 가드가 아닌 분기는 한 줄이어도 중괄호 필수
  - §6 **`auto` 금지** — 람다·이터레이터 등 타입을 적을 수 없는 경우만 예외
  - §7 `!` 단항 부정 금지(`bFlag == false` / `Ptr == nullptr` / `IsValid(X) == false`), `!=` 는 허용
  - §2 UObject 포인터는 `TObjectPtr<>` / §3 include 순서 = 자기 자신 → UE 헤더 → 프로젝트 헤더 → `.generated.h`
  - §8 런타임 액터·컴포넌트 탐색 금지(초기화 1회 캐싱) / §9-2 위젯 자식은 `protected` + `BindWidget`, 의도 API 만 노출 / §11 공용 enum 은 `Util/DefineEnum.h`(**플러그인 enum 은 각 플러그인 헤더에** — SKOnline 의 `ESKSessionOp` 등은 `DefineEnum.h` 에 넣지 않는다)
- **의존 방향**: `SkillProject → SKOnline → OnlineSubsystem/OnlineSubsystemUtils`. **SKOnline 은 게임 모듈·SKGAS·SKAssetCore·SKUICore 를 include 하지 않는다.**
- **커밋 금지**: `.claude/rules/git-conventions.md` + 사용자 규칙 — **`git commit` 을 실행하지 않는다.** 각 Task 마지막은 `git add` 까지만 하고 커밋 메시지(안) `[Tag] ClassName — 요약` 을 제시한다.
- **테스트 경로**: 게임 모듈 = `SkillProject/Source/SkillProject/<도메인>/Tests/`, 등록명 `SkillProject.<도메인>.<기능>.<케이스>`. **SKOnline 플러그인 테스트는 예외로 `SkillProject/Plugins/SKOnline/Source/SKOnline/Private/Tests/` 에 둔다** — 플러그인이 다른 프로젝트로 복사돼도 테스트가 따라가야 하기 때문. 등록명 `SKOnline.Session.<기능>.<케이스>`.
- **테스트 형식**: 파일 전체를 `#if WITH_DEV_AUTOMATION_TESTS` 로 감싼다. `IMPLEMENT_SIMPLE_AUTOMATION_TEST` + `EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter`. 테스트 함수·설명 문자열은 영어.
- **빌드/실행은 사용자 몫**: 이 저장소에는 recompile/test-run 커맨드가 없다. 각 Task 의 "테스트 실행"은 **사용자가 에디터/VS 에서 빌드 후 Automation 창에서 수행**한다. 구현자는 실행을 시도하지 말고 실행 대상 테스트명을 제시한다.
- **UI 목업 게이트**: Task 9(WBP 에셋 생성)는 **목업 승인 전 착수 금지** (`.claude/project.md` "UI 작업 — 목업 승인 게이트"). WidgetBlueprint 에 `compile_blueprint()` 호출 금지(에디터 데드락) — `.claude/rules/ui-workflow.md` §2-3.
- **수치·문구 미확정**: `MaxPlayers`·`MaxSearchResults`·`SearchTimeoutSeconds`·방 이름 포맷·UI 문구는 `game-designer` 가 확정한다(스펙 §8). 이 플랜은 C++ 기본값을 두고, 기획 확정값이 오면 그 값으로 덮는다. **기본값 위치는 `USKOnlineSettings` 한 곳뿐**이라 덮어쓰기가 1파일에서 끝난다.

---

### Task 1: SKOnline 플러그인 스캐폴딩 + 표시용 타입

**Files:**
- Create: `SkillProject/Plugins/SKOnline/SKOnline.uplugin`
- Create: `SkillProject/Plugins/SKOnline/Source/SKOnline/SKOnline.Build.cs`
- Create: `SkillProject/Plugins/SKOnline/Source/SKOnline/Private/SKOnlineModule.cpp`
- Create: `SkillProject/Plugins/SKOnline/Source/SKOnline/Public/SKOnlineTypes.h`
- Create: `SkillProject/Plugins/SKOnline/Source/SKOnline/Private/SKOnlineTypes.cpp`
- Test: `SkillProject/Plugins/SKOnline/Source/SKOnline/Private/Tests/SKSessionInfoTests.cpp`

**Interfaces:**
- Consumes: 없음 (첫 Task)
- Produces: `FSKSessionInfo` (필드 `RoomName`/`HostName`/`CurrentPlayers`/`MaxPlayers`/`PingMs`/`SearchResultIndex`), 정적 팩토리 `FSKSessionInfo::Make(const FString& InRoomName, const FString& InHostName, int32 InMaxConnections, int32 InOpenConnections, int32 InPingMs, int32 InIndex)`. enum `ESKSessionOp { None, Hosting, Finding, Joining, Destroying }`, `ESKSessionError { None, NoOnlineSubsystem, Busy, CreateFailed, FindFailed, JoinFailed, InvalidIndex, ResolveFailed, DestroyFailed }`. 정적 가드 `USKSessionOpRules::CanStartOp(ESKSessionOp CurrentOp, ESKSessionOp RequestedOp)` 는 Task 3.

- [ ] **Step 1: `.uplugin` 작성**

`SkillProject/Plugins/SKOnline/SKOnline.uplugin`:

```json
{
	"FileVersion": 3,
	"Version": 1,
	"VersionName": "1.0",
	"FriendlyName": "SK Online",
	"Description": "Project-agnostic online session logic (host/find/join) over OnlineSubsystem. No UI, no game dependencies.",
	"Category": "Gameplay",
	"CreatedBy": "",
	"CreatedByURL": "",
	"EnabledByDefault": true,
	"CanContainContent": false,
	"IsBetaVersion": false,
	"Installed": false,
	"Modules": [
		{
			"Name": "SKOnline",
			"Type": "Runtime",
			"LoadingPhase": "Default"
		}
	],
	"Plugins": [
		{
			"Name": "OnlineSubsystem",
			"Enabled": true
		},
		{
			"Name": "OnlineSubsystemUtils",
			"Enabled": true
		},
		{
			"Name": "OnlineSubsystemNull",
			"Enabled": true
		}
	]
}
```

- [ ] **Step 2: `Build.cs` 작성**

`SkillProject/Plugins/SKOnline/Source/SKOnline/SKOnline.Build.cs`:

```csharp
// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SKOnline : ModuleRules
{
	public SKOnline(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"DeveloperSettings",
				"OnlineSubsystem",
				"OnlineSubsystemUtils",
			});
	}
}
```

- [ ] **Step 3: 모듈 구현 파일 작성**

`SkillProject/Plugins/SKOnline/Source/SKOnline/Private/SKOnlineModule.cpp`:

```cpp
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, SKOnline);
```

- [ ] **Step 4: 실패하는 테스트를 먼저 작성**

`SkillProject/Plugins/SKOnline/Source/SKOnline/Private/Tests/SKSessionInfoTests.cpp`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "SKOnlineTypes.h"

//# 인원 계산 — 현재 인원 = 최대 − 남은 자리
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSKSessionInfoPlayerCountTest,
	"SKOnline.Session.SessionInfo.PlayerCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSKSessionInfoPlayerCountTest::RunTest(const FString& Parameters)
{
	const FSKSessionInfo Full = FSKSessionInfo::Make(TEXT("Room"), TEXT("Host"), 4, 0, 30, 0);
	TestEqual(TEXT("Full room reports max players"), Full.CurrentPlayers, 4);
	TestEqual(TEXT("Max players preserved"), Full.MaxPlayers, 4);

	const FSKSessionInfo Empty = FSKSessionInfo::Make(TEXT("Room"), TEXT("Host"), 4, 4, 30, 0);
	TestEqual(TEXT("Empty room reports zero"), Empty.CurrentPlayers, 0);

	const FSKSessionInfo Partial = FSKSessionInfo::Make(TEXT("Room"), TEXT("Host"), 4, 1, 30, 0);
	TestEqual(TEXT("Partial room counts occupied slots"), Partial.CurrentPlayers, 3);

	return true;
}

//# 방어 — 음수/역전 입력에도 표시 값이 깨지지 않는다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSKSessionInfoClampTest,
	"SKOnline.Session.SessionInfo.Clamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSKSessionInfoClampTest::RunTest(const FString& Parameters)
{
	//# 남은 자리가 최대보다 크면(비정상 응답) 현재 인원이 음수가 되면 안 된다
	const FSKSessionInfo Weird = FSKSessionInfo::Make(TEXT("Room"), TEXT("Host"), 4, 9, 30, 0);
	TestEqual(TEXT("Current players never negative"), Weird.CurrentPlayers, 0);

	const FSKSessionInfo Negative = FSKSessionInfo::Make(TEXT("Room"), TEXT("Host"), -3, 0, -5, 0);
	TestEqual(TEXT("Max players never negative"), Negative.MaxPlayers, 0);
	TestEqual(TEXT("Ping never negative"), Negative.PingMs, 0);

	return true;
}

//# 방 이름이 비면 호스트명으로 대체한다 (목록에 빈 줄이 뜨지 않게)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSKSessionInfoRoomNameFallbackTest,
	"SKOnline.Session.SessionInfo.RoomNameFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSKSessionInfoRoomNameFallbackTest::RunTest(const FString& Parameters)
{
	const FSKSessionInfo NoName = FSKSessionInfo::Make(TEXT(""), TEXT("Tae"), 4, 3, 12, 2);
	TestEqual(TEXT("Empty room name falls back to host name"), NoName.RoomName, FString(TEXT("Tae")));
	TestEqual(TEXT("Host name preserved"), NoName.HostName, FString(TEXT("Tae")));
	TestEqual(TEXT("Search index preserved"), NoName.SearchResultIndex, 2);

	const FSKSessionInfo Named = FSKSessionInfo::Make(TEXT("Alpha"), TEXT("Tae"), 4, 3, 12, 2);
	TestEqual(TEXT("Explicit room name kept"), Named.RoomName, FString(TEXT("Alpha")));

	return true;
}

#endif
```

- [ ] **Step 5: 테스트가 실패하는지 확인**

사용자에게 빌드를 요청한다. 이 시점에는 `SKOnlineTypes.h` 가 없어 **컴파일 에러**가 나는 것이 정상이다 (기대 실패). 사용자에게 "Task 1 Step 5 — 아직 타입이 없어 컴파일 실패가 정상입니다" 라고 알리고 Step 6 으로 진행한다.

- [ ] **Step 6: 타입 헤더 작성**

`SkillProject/Plugins/SKOnline/Source/SKOnline/Public/SKOnlineTypes.h`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "SKOnlineTypes.generated.h"

//# 진행 중인 세션 작업 — 동시에 하나만 허용한다(중복 입력 가드)
UENUM(BlueprintType)
enum class ESKSessionOp : uint8
{
	None,
	Hosting,
	Finding,
	Joining,
	Destroying
};

//# 세션 작업 실패 사유 — 사용자 문구는 게임 모듈이 정한다(플러그인은 사유 코드만)
UENUM(BlueprintType)
enum class ESKSessionError : uint8
{
	None,
	NoOnlineSubsystem,
	Busy,
	CreateFailed,
	FindFailed,
	JoinFailed,
	InvalidIndex,
	ResolveFailed,
	DestroyFailed
};

//# 방 목록 표시용 struct — UI 가 OSS 타입(FOnlineSessionSearchResult)에 직접 물리지 않게 하는 차단막
USTRUCT(BlueprintType)
struct SKONLINE_API FSKSessionInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "SKOnline")
	FString RoomName;

	UPROPERTY(BlueprintReadOnly, Category = "SKOnline")
	FString HostName;

	UPROPERTY(BlueprintReadOnly, Category = "SKOnline")
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "SKOnline")
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "SKOnline")
	int32 PingMs = 0;

	//# 조인 시 원본 검색 결과를 되찾는 인덱스
	UPROPERTY(BlueprintReadOnly, Category = "SKOnline")
	int32 SearchResultIndex = INDEX_NONE;

	//# 순수 변환 — OSS 타입에 의존하지 않아 자동화 테스트가 가능하다
	static FSKSessionInfo Make(
		const FString& InRoomName,
		const FString& InHostName,
		int32 InMaxConnections,
		int32 InOpenConnections,
		int32 InPingMs,
		int32 InIndex);
};
```

- [ ] **Step 7: 최소 구현 작성**

`SkillProject/Plugins/SKOnline/Source/SKOnline/Private/SKOnlineTypes.cpp`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "SKOnlineTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKOnlineTypes)

FSKSessionInfo FSKSessionInfo::Make(
	const FString& InRoomName,
	const FString& InHostName,
	int32 InMaxConnections,
	int32 InOpenConnections,
	int32 InPingMs,
	int32 InIndex)
{
	FSKSessionInfo Info;

	//# 방 이름이 비면 목록에 빈 줄이 뜨므로 호스트명으로 대체한다
	Info.RoomName = InRoomName.IsEmpty() ? InHostName : InRoomName;
	Info.HostName = InHostName;

	//# 백엔드가 비정상 값을 줘도 표시가 깨지지 않게 바닥을 잡는다
	Info.MaxPlayers = FMath::Max(InMaxConnections, 0);
	Info.CurrentPlayers = FMath::Clamp(Info.MaxPlayers - InOpenConnections, 0, Info.MaxPlayers);
	Info.PingMs = FMath::Max(InPingMs, 0);
	Info.SearchResultIndex = InIndex;

	return Info;
}
```

- [ ] **Step 8: 프로젝트 파일 재생성 + 빌드 + 테스트 실행 (사용자)**

사용자에게 요청한다:
1. `SkillProject/SkillProject.uproject` 우클릭 → Generate Visual Studio project files (신규 플러그인 모듈 인식)
2. 빌드
3. Automation 창에서 `SKOnline.Session.SessionInfo.*` 3건 실행

기대: 3건 모두 PASS.

- [ ] **Step 9: 스테이징 + 커밋 메시지(안)**

```bash
git add SkillProject/Plugins/SKOnline/
```

커밋 메시지(안): `[Feature] SKOnline — 세션 플러그인 스캐폴딩 + FSKSessionInfo`

---

### Task 2: `USKOnlineSettings` — 백엔드 이음매

**Files:**
- Create: `SkillProject/Plugins/SKOnline/Source/SKOnline/Public/SKOnlineSettings.h`
- Create: `SkillProject/Plugins/SKOnline/Source/SKOnline/Private/SKOnlineSettings.cpp`
- Test: `SkillProject/Plugins/SKOnline/Source/SKOnline/Private/Tests/SKOnlineSettingsTests.cpp`

**Interfaces:**
- Consumes: Task 1 의 모듈·Build.cs (`DeveloperSettings` 의존 포함)
- Produces: `USKOnlineSettings` — `GetDefault<USKOnlineSettings>()` 로 접근. 필드: `bIsLanMatch`(bool), `bUsesPresence`(bool), `bUseLobbiesIfAvailable`(bool), `bShouldAdvertise`(bool), `bAllowJoinInProgress`(bool), `MaxPlayers`(int32), `MaxSearchResults`(int32), `DefaultRoomNameFormat`(FString). 상수 `SKOnlineKeys::RoomName`(FName, 세션 커스텀 세팅 키).

- [ ] **Step 1: 실패하는 테스트를 먼저 작성**

`SkillProject/Plugins/SKOnline/Source/SKOnline/Private/Tests/SKOnlineSettingsTests.cpp`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "SKOnlineSettings.h"

//# 기본 프로필 = OSS Null(LAN). Steam 전환은 ini 로만 이뤄져야 한다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSKOnlineSettingsNullDefaultsTest,
	"SKOnline.Session.Settings.NullDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSKOnlineSettingsNullDefaultsTest::RunTest(const FString& Parameters)
{
	const USKOnlineSettings* Settings = GetDefault<USKOnlineSettings>();
	if (Settings == nullptr)
	{
		AddError(TEXT("USKOnlineSettings CDO is null"));
		return false;
	}

	//# Null 백엔드는 LAN 브로드캐스트로만 방을 찾는다
	TestTrue(TEXT("LAN match on by default"), Settings->bIsLanMatch);

	//# presence/lobby 는 Steam 전용 — Null 에서 켜면 검색이 깨진다
	TestFalse(TEXT("Presence off by default"), Settings->bUsesPresence);
	TestFalse(TEXT("Lobbies off by default"), Settings->bUseLobbiesIfAvailable);

	//# 광고를 끄면 방이 목록에 뜨지 않는다
	TestTrue(TEXT("Advertise on by default"), Settings->bShouldAdvertise);

	return true;
}

//# 인원/검색 수는 1 이상이어야 세션이 성립한다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSKOnlineSettingsPositiveCountsTest,
	"SKOnline.Session.Settings.PositiveCounts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSKOnlineSettingsPositiveCountsTest::RunTest(const FString& Parameters)
{
	const USKOnlineSettings* Settings = GetDefault<USKOnlineSettings>();
	if (Settings == nullptr)
	{
		AddError(TEXT("USKOnlineSettings CDO is null"));
		return false;
	}

	TestTrue(TEXT("MaxPlayers is at least two"), Settings->MaxPlayers >= 2);
	TestTrue(TEXT("MaxSearchResults is positive"), Settings->MaxSearchResults > 0);
	TestFalse(TEXT("Room name format is not empty"), Settings->DefaultRoomNameFormat.IsEmpty());

	return true;
}

#endif
```

- [ ] **Step 2: 테스트가 실패하는지 확인**

`SKOnlineSettings.h` 가 없어 컴파일 실패가 기대값이다. Step 3 으로 진행.

- [ ] **Step 3: 설정 헤더 작성**

`SkillProject/Plugins/SKOnline/Source/SKOnline/Public/SKOnlineSettings.h`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "SKOnlineSettings.generated.h"

namespace SKOnlineKeys
{
	//# 세션 커스텀 세팅 키 — 방 이름을 광고에 실어 목록에 표시한다.
	//# Steam AppID 480 은 전 세계 공용이라 이 키로 남의 테스트 방을 걸러낼 수 있다.
	inline const FName RoomName = TEXT("SK_ROOMNAME");
}

//# 세션 백엔드 프로필 — Null↔Steam 전환을 ini 한 곳에서 끝내기 위한 이음매.
//# Config DataAsset 이 아니라 UDeveloperSettings 인 이유는 spec §4-4 참고
//# (SKAssetCore 의존 회피 + DefaultPlatformService·NetDriver 와 같은 파일에서 전환).
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "SK Online"))
class SKONLINE_API USKOnlineSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	//# 프로젝트 설정 > 플러그인 > SK Online 으로 노출
	virtual FName GetCategoryName() const override;

public:
	//# LAN 브로드캐스트 검색 — OSS Null 은 true 여야 방이 보인다. Steam 은 false
	UPROPERTY(config, EditAnywhere, Category = "Backend")
	bool bIsLanMatch = true;

	//# Steam 전용 — presence 기반 검색. Null 에서 켜면 검색이 깨진다
	UPROPERTY(config, EditAnywhere, Category = "Backend")
	bool bUsesPresence = false;

	//# Steam 전용 — 로비 기반 검색
	UPROPERTY(config, EditAnywhere, Category = "Backend")
	bool bUseLobbiesIfAvailable = false;

	//# 세션을 목록에 광고할지. 끄면 방이 뜨지 않는다
	UPROPERTY(config, EditAnywhere, Category = "Session")
	bool bShouldAdvertise = true;

	//# 게임 시작 후 난입 허용
	UPROPERTY(config, EditAnywhere, Category = "Session")
	bool bAllowJoinInProgress = true;

	//# 방 최대 인원(호스트 포함)
	UPROPERTY(config, EditAnywhere, Category = "Session", meta = (ClampMin = "2"))
	int32 MaxPlayers = 4;

	//# 한 번 검색에서 받아올 최대 방 수
	UPROPERTY(config, EditAnywhere, Category = "Session", meta = (ClampMin = "1"))
	int32 MaxSearchResults = 20;

	//# 방 이름 자동 생성 포맷. {0} 에 호스트명이 들어간다 (옵션 입력 화면이 없으므로 필수)
	UPROPERTY(config, EditAnywhere, Category = "Session")
	FString DefaultRoomNameFormat = TEXT("{0}의 방");
};
```

- [ ] **Step 4: 구현 파일 작성**

`SkillProject/Plugins/SKOnline/Source/SKOnline/Private/SKOnlineSettings.cpp`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "SKOnlineSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKOnlineSettings)

FName USKOnlineSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}
```

- [ ] **Step 5: 빌드 + 테스트 실행 (사용자)**

Automation 창에서 `SKOnline.Session.Settings.*` 2건 실행. 기대: 전부 PASS.

- [ ] **Step 6: 스테이징 + 커밋 메시지(안)**

```bash
git add SkillProject/Plugins/SKOnline/Source/SKOnline/Public/SKOnlineSettings.h SkillProject/Plugins/SKOnline/Source/SKOnline/Private/SKOnlineSettings.cpp SkillProject/Plugins/SKOnline/Source/SKOnline/Private/Tests/SKOnlineSettingsTests.cpp
```

커밋 메시지(안): `[Feature] USKOnlineSettings — Null/Steam 세션 프로필 이음매`

---

### Task 3: 세션 작업 중복 가드 (순수 규칙)

**Files:**
- Create: `SkillProject/Plugins/SKOnline/Source/SKOnline/Public/SKSessionOpRules.h`
- Create: `SkillProject/Plugins/SKOnline/Source/SKOnline/Private/SKSessionOpRules.cpp`
- Test: `SkillProject/Plugins/SKOnline/Source/SKOnline/Private/Tests/SKSessionOpRulesTests.cpp`

**Interfaces:**
- Consumes: Task 1 의 `ESKSessionOp`
- Produces: `USKSessionOpRules::CanStartOp(ESKSessionOp CurrentOp, ESKSessionOp RequestedOp)` → `bool`. Task 4 의 서브시스템이 모든 명령 진입부에서 호출한다.

- [ ] **Step 1: 실패하는 테스트를 먼저 작성**

`SkillProject/Plugins/SKOnline/Source/SKOnline/Private/Tests/SKSessionOpRulesTests.cpp`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "SKSessionOpRules.h"

//# 유휴 상태에서는 어떤 작업이든 시작할 수 있다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSKSessionOpIdleAllowsAnyTest,
	"SKOnline.Session.OpRules.IdleAllowsAny",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSKSessionOpIdleAllowsAnyTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Idle allows hosting"), USKSessionOpRules::CanStartOp(ESKSessionOp::None, ESKSessionOp::Hosting));
	TestTrue(TEXT("Idle allows finding"), USKSessionOpRules::CanStartOp(ESKSessionOp::None, ESKSessionOp::Finding));
	TestTrue(TEXT("Idle allows joining"), USKSessionOpRules::CanStartOp(ESKSessionOp::None, ESKSessionOp::Joining));
	TestTrue(TEXT("Idle allows destroying"), USKSessionOpRules::CanStartOp(ESKSessionOp::None, ESKSessionOp::Destroying));

	return true;
}

//# 작업이 진행 중이면 새 작업을 막는다 — 더블클릭·연타 가드
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSKSessionOpBusyBlocksTest,
	"SKOnline.Session.OpRules.BusyBlocks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSKSessionOpBusyBlocksTest::RunTest(const FString& Parameters)
{
	//# 같은 작업 연타
	TestFalse(TEXT("Finding blocks finding"), USKSessionOpRules::CanStartOp(ESKSessionOp::Finding, ESKSessionOp::Finding));
	TestFalse(TEXT("Joining blocks joining"), USKSessionOpRules::CanStartOp(ESKSessionOp::Joining, ESKSessionOp::Joining));

	//# 다른 작업 끼어들기
	TestFalse(TEXT("Finding blocks hosting"), USKSessionOpRules::CanStartOp(ESKSessionOp::Finding, ESKSessionOp::Hosting));
	TestFalse(TEXT("Hosting blocks joining"), USKSessionOpRules::CanStartOp(ESKSessionOp::Hosting, ESKSessionOp::Joining));
	TestFalse(TEXT("Joining blocks finding"), USKSessionOpRules::CanStartOp(ESKSessionOp::Joining, ESKSessionOp::Finding));
	TestFalse(TEXT("Destroying blocks hosting"), USKSessionOpRules::CanStartOp(ESKSessionOp::Destroying, ESKSessionOp::Hosting));

	return true;
}

//# None 을 요청하는 것은 의미가 없다 — 항상 거부해 상태를 우회로 비우지 못하게 한다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSKSessionOpRequestNoneRejectedTest,
	"SKOnline.Session.OpRules.RequestNoneRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSKSessionOpRequestNoneRejectedTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("Requesting None from idle is rejected"), USKSessionOpRules::CanStartOp(ESKSessionOp::None, ESKSessionOp::None));
	TestFalse(TEXT("Requesting None while busy is rejected"), USKSessionOpRules::CanStartOp(ESKSessionOp::Finding, ESKSessionOp::None));

	return true;
}

#endif
```

- [ ] **Step 2: 테스트가 실패하는지 확인**

`SKSessionOpRules.h` 부재로 컴파일 실패가 기대값이다.

- [ ] **Step 3: 헤더 작성**

`SkillProject/Plugins/SKOnline/Source/SKOnline/Public/SKSessionOpRules.h`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SKOnlineTypes.h"

#include "SKSessionOpRules.generated.h"

//# 세션 작업 진입 규칙 — 엔진 상태에 의존하지 않는 순수 판정이라 단위 테스트가 가능하다
UCLASS()
class SKONLINE_API USKSessionOpRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	//# 현재 작업이 없을 때만 새 작업을 시작한다. RequestedOp 가 None 이면 항상 거부
	UFUNCTION(BlueprintCallable, Category = "SKOnline")
	static bool CanStartOp(ESKSessionOp CurrentOp, ESKSessionOp RequestedOp);
};
```

- [ ] **Step 4: 최소 구현 작성**

`SkillProject/Plugins/SKOnline/Source/SKOnline/Private/SKSessionOpRules.cpp`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "SKSessionOpRules.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKSessionOpRules)

bool USKSessionOpRules::CanStartOp(ESKSessionOp CurrentOp, ESKSessionOp RequestedOp)
{
	//# 아무 작업도 요청하지 않은 것은 유효한 명령이 아니다
	if (RequestedOp == ESKSessionOp::None)
		return false;

	//# 세션 작업은 비동기 콜백으로 끝나므로 동시에 하나만 허용한다
	return CurrentOp == ESKSessionOp::None;
}
```

- [ ] **Step 5: 빌드 + 테스트 실행 (사용자)**

Automation 창에서 `SKOnline.Session.OpRules.*` 3건 실행. 기대: 전부 PASS.

- [ ] **Step 6: 스테이징 + 커밋 메시지(안)**

```bash
git add SkillProject/Plugins/SKOnline/Source/SKOnline/Public/SKSessionOpRules.h SkillProject/Plugins/SKOnline/Source/SKOnline/Private/SKSessionOpRules.cpp SkillProject/Plugins/SKOnline/Source/SKOnline/Private/Tests/SKSessionOpRulesTests.cpp
```

커밋 메시지(안): `[Feature] USKSessionOpRules — 세션 작업 중복 가드`

---

### Task 4: `USKOnlineSessionSubsystem` — 세션 생성/검색/조인/파괴

**Files:**
- Create: `SkillProject/Plugins/SKOnline/Source/SKOnline/Public/SKOnlineSessionSubsystem.h`
- Create: `SkillProject/Plugins/SKOnline/Source/SKOnline/Private/SKOnlineSessionSubsystem.cpp`

**Interfaces:**
- Consumes: Task 1 `FSKSessionInfo`/`ESKSessionOp`/`ESKSessionError`, Task 2 `USKOnlineSettings`·`SKOnlineKeys::RoomName`, Task 3 `USKSessionOpRules::CanStartOp`

> **⚠⚠ 델리게이트 계약 — OSS Null 은 완료 콜백을 명령 호출 안에서 동기 발화한다.**
> `CreateSession`/`JoinSession`/`DestroySession`/`FindSessions` 는 Null 백엔드에서 **반환 전에** 완료 델리게이트를 트리거한다(`OnlineSessionInterfaceNull.cpp:248`·`649`·`478`·`580`). 두 가지 귀결이 있고 **둘 다 이 Task 의 코드가 다뤄야 한다**:
> 1. **소비자는 명령을 호출하기 *전에* 구독해야 한다.** `HostSession()` 이 반환되기 전에 `OnHostReady` 가 이미 broadcast 될 수 있다. 호출 후에 구독하면 영영 못 받고, 증상이 "아무 일도 안 일어남"이라 추적이 어렵다. (Task 7 `NativeConstruct` 는 구독 → `FindSessions()` 순서라 이미 올바르다. Task 5·8 도 이 순서를 깨지 말 것.)
> 2. **명령이 `false` 를 반환하는 실패 경로에서 통지가 두 번 나갈 수 있다** — 아래 Step 3~6 이 `CurrentOp` 가드로 막는다.
>
> **⚠ `SEARCH_PRESENCE` 는 UE 5.7 에 존재하지 않는다 (구현 중 엔진 소스로 확인).**
> `D:/UE_5.7/Engine/Plugins` + `Engine/Source` **전체 grep 결과 0건**이다(헤더 하나가 아니라 엔진 전역에서 부재). 검색 키는 `OnlineBase/Source/Public/Online/OnlineSessionNames.h` 의 `SEARCH_DEDICATED_ONLY`(126) · `SEARCH_EMPTY_SERVERS_ONLY`(128) · **`SEARCH_LOBBIES`(151)** 등이고 presence 키는 없다. 그대로 쓰면 **컴파일이 깨진다.**
>
> **그리고 매크로만 지우면 안 된다** — 그러면 검색 경로를 가르는 설정이 하나도 남지 않는다. `bUsesPresence`·`bUseLobbiesIfAvailable` 는 **생성 측**(`FOnlineSessionSettings`)에만 실리고 검색 측은 아무것도 읽지 않게 되어, Steam 전환 후 `CreateSession` 은 로비 세션을 만드는데 `FindSessions` 는 서버 브라우저 경로를 타서 **목록이 항상 비어 돌아온다.** (Steam 은 `OnlineSessionInterfaceSteam.cpp:778` 에서 `QuerySettings.Get(SEARCH_LOBBIES, ...)` 로 분기한다.)
>
> **→ 아래 Step 4 는 `bUseLobbiesIfAvailable` 로 게이트한 `SEARCH_LOBBIES` 를 쓴다.** Null/LAN 은 `bIsLanQuery` 가 `FindLANSession` 으로 라우팅하므로 이 키와 무관하다.
- Produces: `USKOnlineSessionSubsystem` (UGameInstanceSubsystem). 명령 `HostSession()`, `FindSessions()`, `JoinSessionByIndex(int32 Index)`, `DestroyCurrentSession()`. 델리게이트 `OnSessionsFound`(`const TArray<FSKSessionInfo>&`), `OnHostReady`(무인자), `OnJoinReady`(`const FString& ConnectString`), `OnSessionError`(`ESKSessionOp`, `ESKSessionError`, `const FString& Detail`). 조회 `GetCurrentOp()`.

> **테스트 없음이 의도된 Task 다.** 이 클래스는 전부 `IOnlineSubsystem` 비동기 콜백이라 자동화 테스트 대상이 아니다. 테스트 가능한 로직(변환·가드·설정)은 Task 1~3 에서 이미 분리해 커버했다. 이 Task 의 검증은 Task 10 의 PIE 수동 검증이다.

- [ ] **Step 1: 헤더 작성**

`SkillProject/Plugins/SKOnline/Source/SKOnline/Public/SKOnlineSessionSubsystem.h`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "SKOnlineTypes.h"

#include "SKOnlineSessionSubsystem.generated.h"

class FOnlineSessionSearch;

//# 방 목록 갱신 — 검색 성공 시(결과 0건 포함) 브로드캐스트
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSKSessionsFound, const TArray<FSKSessionInfo>&);

//# 방 생성 완료 — 소비자가 리슨 서버로 트래블한다(플러그인은 트래블하지 않는다)
DECLARE_MULTICAST_DELEGATE(FOnSKHostReady);

//# 조인 완료 — 해석된 접속 문자열을 넘긴다. 소비자가 ClientTravel 한다
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSKJoinReady, const FString&);

//# 작업 실패 — 사용자 문구는 소비자가 정한다. Detail 은 로그용 원문
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnSKSessionError, ESKSessionOp, ESKSessionError, const FString&);

//# OnlineSubsystem 세션 파이프라인. UI·트래블을 모르고 델리게이트만 브로드캐스트한다.
UCLASS()
class SKONLINE_API USKOnlineSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

public:
	//# 설정 기본값으로 즉시 방을 만든다(옵션 입력 화면 없음)
	void HostSession();

	//# 접속 가능한 방을 검색한다
	void FindSessions();

	//# 직전 검색 결과의 Index 번째 방에 조인한다
	void JoinSessionByIndex(int32 Index);

	//# 현재 세션을 파괴한다(조인 실패 복구·정리용)
	void DestroyCurrentSession();

	ESKSessionOp GetCurrentOp() const
	{
		return CurrentOp;
	}

public:
	FOnSKSessionsFound OnSessionsFound;
	FOnSKHostReady OnHostReady;
	FOnSKJoinReady OnJoinReady;
	FOnSKSessionError OnSessionError;

protected:
	//# 세션 인터페이스 획득 — 없으면 NoOnlineSubsystem 으로 실패 통지 후 nullptr
	IOnlineSessionPtr GetSessionInterfaceChecked(ESKSessionOp Op);

	//# 작업 시작 가드 + 상태 전이. 거부되면 Busy 로 실패 통지 후 false
	bool BeginOp(ESKSessionOp RequestedOp);

	//# 작업 종료 — 상태를 None 으로 되돌린다
	void EndOp();

	//# 실패 통지 + 상태 복구를 한 번에
	void FailOp(ESKSessionOp Op, ESKSessionError Error, const FString& Detail);

	//# 호스트명 기반 방 이름 생성 (설정의 포맷 사용)
	FString BuildRoomName() const;

protected:
	//# OSS 콜백
	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	//# 등록한 델리게이트 핸들 해제
	void ClearDelegateHandles(IOnlineSessionPtr Sessions);

protected:
	//# 동시에 하나만 — USKSessionOpRules 가 판정한다
	ESKSessionOp CurrentOp = ESKSessionOp::None;

	//# 직전 검색 결과 — JoinSessionByIndex 가 인덱스로 되찾는다
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	FDelegateHandle CreateSessionCompleteHandle;
	FDelegateHandle FindSessionsCompleteHandle;
	FDelegateHandle JoinSessionCompleteHandle;
	FDelegateHandle DestroySessionCompleteHandle;
};
```

- [ ] **Step 2: 구현 — 인터페이스 획득 / 상태 전이 / 헬퍼**

`SkillProject/Plugins/SKOnline/Source/SKOnline/Private/SKOnlineSessionSubsystem.cpp` (파일 앞부분):

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "SKOnlineSessionSubsystem.h"

#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "SKOnlineSettings.h"
#include "SKSessionOpRules.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKOnlineSessionSubsystem)

IOnlineSessionPtr USKOnlineSessionSubsystem::GetSessionInterfaceChecked(ESKSessionOp Op)
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem == nullptr)
	{
		FailOp(Op, ESKSessionError::NoOnlineSubsystem, TEXT("IOnlineSubsystem::Get() returned null"));
		return nullptr;
	}

	IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface();
	if (Sessions.IsValid() == false)
	{
		FailOp(Op, ESKSessionError::NoOnlineSubsystem, TEXT("Session interface is invalid"));
		return nullptr;
	}

	return Sessions;
}

bool USKOnlineSessionSubsystem::BeginOp(ESKSessionOp RequestedOp)
{
	if (USKSessionOpRules::CanStartOp(CurrentOp, RequestedOp) == false)
	{
		//# 진행 중 작업이 있으므로 조용히 거부하되 소비자가 알 수 있게 통지한다
		OnSessionError.Broadcast(RequestedOp, ESKSessionError::Busy, TEXT("Another session operation is in progress"));
		return false;
	}

	CurrentOp = RequestedOp;
	return true;
}

void USKOnlineSessionSubsystem::EndOp()
{
	CurrentOp = ESKSessionOp::None;
}

void USKOnlineSessionSubsystem::FailOp(ESKSessionOp Op, ESKSessionError Error, const FString& Detail)
{
	UE_LOG(LogTemp, Error, TEXT("# [SKOnlineSessionSubsystem] 세션 작업 실패 (Op=%d, Error=%d): %s"),
		(int32)Op, (int32)Error, *Detail);

	EndOp();
	OnSessionError.Broadcast(Op, Error, Detail);
}

FString USKOnlineSessionSubsystem::BuildRoomName() const
{
	const USKOnlineSettings* Settings = GetDefault<USKOnlineSettings>();

	FString HostName = TEXT("Player");
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const ULocalPlayer* LocalPlayer = GameInstance->GetFirstGamePlayer())
		{
			const FString NickName = LocalPlayer->GetNickname();
			if (NickName.IsEmpty() == false)
			{
				HostName = NickName;
			}
		}
	}

	if (Settings == nullptr || Settings->DefaultRoomNameFormat.IsEmpty())
		return HostName;

	return FString::Format(*Settings->DefaultRoomNameFormat, { HostName });
}
```

- [ ] **Step 3: 구현 — `HostSession`**

같은 파일에 이어서:

```cpp
void USKOnlineSessionSubsystem::HostSession()
{
	if (BeginOp(ESKSessionOp::Hosting) == false)
		return;

	IOnlineSessionPtr Sessions = GetSessionInterfaceChecked(ESKSessionOp::Hosting);
	if (Sessions.IsValid() == false)
		return;

	//# 이전 세션이 남아 있으면 생성이 실패한다 — 먼저 지운다
	if (Sessions->GetNamedSession(NAME_GameSession) != nullptr)
	{
		Sessions->DestroySession(NAME_GameSession);
	}

	const USKOnlineSettings* Settings = GetDefault<USKOnlineSettings>();
	if (Settings == nullptr)
	{
		FailOp(ESKSessionOp::Hosting, ESKSessionError::CreateFailed, TEXT("USKOnlineSettings CDO is null"));
		return;
	}

	FOnlineSessionSettings SessionSettings;
	SessionSettings.NumPublicConnections = Settings->MaxPlayers;
	SessionSettings.NumPrivateConnections = 0;
	SessionSettings.bIsLANMatch = Settings->bIsLanMatch;
	SessionSettings.bShouldAdvertise = Settings->bShouldAdvertise;
	SessionSettings.bAllowJoinInProgress = Settings->bAllowJoinInProgress;
	SessionSettings.bAllowJoinViaPresence = Settings->bUsesPresence;
	SessionSettings.bUsesPresence = Settings->bUsesPresence;
	SessionSettings.bUseLobbiesIfAvailable = Settings->bUseLobbiesIfAvailable;
	SessionSettings.bIsDedicated = false;

	//# 방 이름을 광고에 실어 목록에 표시한다
	SessionSettings.Set(
		SKOnlineKeys::RoomName,
		BuildRoomName(),
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	CreateSessionCompleteHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &USKOnlineSessionSubsystem::HandleCreateSessionComplete));

	if (Sessions->CreateSession(0, NAME_GameSession, SessionSettings) == false)
	{
		//# Null 은 실패 콜백을 이 호출 안에서 이미 실행했을 수 있다. 그때는 EndOp 로 op 가 None 이 돼 있으므로
		//# 중복 통지하지 않는다. 아직 Hosting 이면 콜백이 오지 않은 것이라 여기서 정리한다.
		if (CurrentOp == ESKSessionOp::Hosting)
		{
			Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
			CreateSessionCompleteHandle.Reset();
			FailOp(ESKSessionOp::Hosting, ESKSessionError::CreateFailed, TEXT("CreateSession returned false"));
		}
	}
}

void USKOnlineSessionSubsystem::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	//# 잔류 핸들의 뒤늦은 발화·다른 op 문맥의 콜백을 무시한다. 핸들 정리는 등록한 쪽이 책임진다.
	if (CurrentOp != ESKSessionOp::Hosting || SessionName != NAME_GameSession)
		return;

	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		if (IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface())
		{
			Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
		}
	}
	CreateSessionCompleteHandle.Reset();

	if (bWasSuccessful == false)
	{
		FailOp(ESKSessionOp::Hosting, ESKSessionError::CreateFailed, TEXT("CreateSession completed unsuccessfully"));
		return;
	}

	EndOp();

	UE_LOG(LogTemp, Log, TEXT("# [SKOnlineSessionSubsystem] 방 생성 완료 — 리슨 서버 전환 대기"));
	OnHostReady.Broadcast();
}
```

- [ ] **Step 4: 구현 — `FindSessions`**

```cpp
void USKOnlineSessionSubsystem::FindSessions()
{
	if (BeginOp(ESKSessionOp::Finding) == false)
		return;

	IOnlineSessionPtr Sessions = GetSessionInterfaceChecked(ESKSessionOp::Finding);
	if (Sessions.IsValid() == false)
		return;

	const USKOnlineSettings* Settings = GetDefault<USKOnlineSettings>();
	if (Settings == nullptr)
	{
		FailOp(ESKSessionOp::Finding, ESKSessionError::FindFailed, TEXT("USKOnlineSettings CDO is null"));
		return;
	}

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->bIsLanQuery = Settings->bIsLanMatch;
	SessionSearch->MaxSearchResults = Settings->MaxSearchResults;

	//# 로비 검색 질의 — Steam 은 이 키로 로비 경로를 탄다. Null 은 bIsLanQuery 가 라우팅하므로 무관.
	if (Settings->bUseLobbiesIfAvailable)
	{
		SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	}

	FindSessionsCompleteHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &USKOnlineSessionSubsystem::HandleFindSessionsComplete));

	if (Sessions->FindSessions(0, SessionSearch.ToSharedRef()) == false)
	{
		//# Null 은 실패 콜백을 이 호출 안에서 이미 실행했을 수 있다 — 중복 통지 방지(Step 3 과 동일 규약)
		if (CurrentOp == ESKSessionOp::Finding)
		{
			Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
			FindSessionsCompleteHandle.Reset();
			FailOp(ESKSessionOp::Finding, ESKSessionError::FindFailed, TEXT("FindSessions returned false"));
		}
	}
}

void USKOnlineSessionSubsystem::HandleFindSessionsComplete(bool bWasSuccessful)
{
	//# 잔류 핸들의 뒤늦은 발화를 무시한다 (이 콜백에는 SessionName 인자가 없다)
	if (CurrentOp != ESKSessionOp::Finding)
		return;

	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		if (IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface())
		{
			Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		}
	}
	FindSessionsCompleteHandle.Reset();

	if (bWasSuccessful == false || SessionSearch.IsValid() == false)
	{
		FailOp(ESKSessionOp::Finding, ESKSessionError::FindFailed, TEXT("FindSessions completed unsuccessfully"));
		return;
	}

	TArray<FSKSessionInfo> Infos;
	Infos.Reserve(SessionSearch->SearchResults.Num());

	for (int32 Index = 0; Index < SessionSearch->SearchResults.Num(); ++Index)
	{
		const FOnlineSessionSearchResult& Result = SessionSearch->SearchResults[Index];
		if (Result.IsValid() == false)
			continue;

		FString RoomName;
		Result.Session.SessionSettings.Get(SKOnlineKeys::RoomName, RoomName);

		Infos.Add(FSKSessionInfo::Make(
			RoomName,
			Result.Session.OwningUserName,
			Result.Session.SessionSettings.NumPublicConnections,
			Result.Session.NumOpenPublicConnections,
			Result.PingInMs,
			Index));
	}

	EndOp();

	UE_LOG(LogTemp, Log, TEXT("# [SKOnlineSessionSubsystem] 방 검색 완료 — %d 건"), Infos.Num());
	OnSessionsFound.Broadcast(Infos);
}
```

- [ ] **Step 5: 구현 — `JoinSessionByIndex`**

```cpp
void USKOnlineSessionSubsystem::JoinSessionByIndex(int32 Index)
{
	if (BeginOp(ESKSessionOp::Joining) == false)
		return;

	if (SessionSearch.IsValid() == false || SessionSearch->SearchResults.IsValidIndex(Index) == false)
	{
		FailOp(ESKSessionOp::Joining, ESKSessionError::InvalidIndex, TEXT("Search result index is out of range"));
		return;
	}

	IOnlineSessionPtr Sessions = GetSessionInterfaceChecked(ESKSessionOp::Joining);
	if (Sessions.IsValid() == false)
		return;

	JoinSessionCompleteHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &USKOnlineSessionSubsystem::HandleJoinSessionComplete));

	if (Sessions->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[Index]) == false)
	{
		//# Null 은 실패 콜백을 이 호출 안에서 이미 실행했을 수 있다 — 중복 통지 방지(Step 3 과 동일 규약)
		if (CurrentOp == ESKSessionOp::Joining)
		{
			Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
			JoinSessionCompleteHandle.Reset();
			FailOp(ESKSessionOp::Joining, ESKSessionError::JoinFailed, TEXT("JoinSession returned false"));
		}
	}
}

void USKOnlineSessionSubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	//# 잔류 핸들의 뒤늦은 발화·다른 op 문맥의 콜백을 무시한다
	if (CurrentOp != ESKSessionOp::Joining || SessionName != NAME_GameSession)
		return;

	IOnlineSessionPtr Sessions = nullptr;
	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		Sessions = Subsystem->GetSessionInterface();
	}

	if (Sessions.IsValid())
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
	}
	JoinSessionCompleteHandle.Reset();

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		//# 실패한 세션이 남아 있으면 다음 조인이 막힌다 — 정리한다
		if (Sessions.IsValid())
		{
			Sessions->DestroySession(NAME_GameSession);
		}

		FailOp(ESKSessionOp::Joining, ESKSessionError::JoinFailed,
			FString::Printf(TEXT("JoinSession result: %d"), (int32)Result));
		return;
	}

	FString ConnectString;
	if (Sessions.IsValid() == false || Sessions->GetResolvedConnectString(NAME_GameSession, ConnectString) == false
		|| ConnectString.IsEmpty())
	{
		if (Sessions.IsValid())
		{
			Sessions->DestroySession(NAME_GameSession);
		}

		FailOp(ESKSessionOp::Joining, ESKSessionError::ResolveFailed, TEXT("GetResolvedConnectString failed"));
		return;
	}

	EndOp();

	UE_LOG(LogTemp, Log, TEXT("# [SKOnlineSessionSubsystem] 조인 완료 — 접속 문자열: %s"), *ConnectString);
	OnJoinReady.Broadcast(ConnectString);
}
```

- [ ] **Step 6: 구현 — `DestroyCurrentSession` / `Deinitialize` / 핸들 정리**

```cpp
void USKOnlineSessionSubsystem::DestroyCurrentSession()
{
	if (BeginOp(ESKSessionOp::Destroying) == false)
		return;

	IOnlineSessionPtr Sessions = GetSessionInterfaceChecked(ESKSessionOp::Destroying);
	if (Sessions.IsValid() == false)
		return;

	//# 지울 세션이 없으면 성공으로 본다
	if (Sessions->GetNamedSession(NAME_GameSession) == nullptr)
	{
		EndOp();
		return;
	}

	DestroySessionCompleteHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &USKOnlineSessionSubsystem::HandleDestroySessionComplete));

	if (Sessions->DestroySession(NAME_GameSession) == false)
	{
		//# Null 은 실패 콜백을 이 호출 안에서 이미 실행했을 수 있다 — 중복 통지 방지(Step 3 과 동일 규약)
		if (CurrentOp == ESKSessionOp::Destroying)
		{
			Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
			DestroySessionCompleteHandle.Reset();
			FailOp(ESKSessionOp::Destroying, ESKSessionError::DestroyFailed, TEXT("DestroySession returned false"));
		}
	}
}

void USKOnlineSessionSubsystem::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	//# 잔류 핸들의 뒤늦은 발화·다른 op 문맥의 콜백을 무시한다
	if (CurrentOp != ESKSessionOp::Destroying || SessionName != NAME_GameSession)
		return;

	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		if (IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface())
		{
			Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
		}
	}
	DestroySessionCompleteHandle.Reset();

	if (bWasSuccessful == false)
	{
		FailOp(ESKSessionOp::Destroying, ESKSessionError::DestroyFailed, TEXT("DestroySession completed unsuccessfully"));
		return;
	}

	EndOp();
}

void USKOnlineSessionSubsystem::ClearDelegateHandles(IOnlineSessionPtr Sessions)
{
	if (Sessions.IsValid() == false)
		return;

	if (CreateSessionCompleteHandle.IsValid())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
		CreateSessionCompleteHandle.Reset();
	}
	if (FindSessionsCompleteHandle.IsValid())
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		FindSessionsCompleteHandle.Reset();
	}
	if (JoinSessionCompleteHandle.IsValid())
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
		JoinSessionCompleteHandle.Reset();
	}
	if (DestroySessionCompleteHandle.IsValid())
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
		DestroySessionCompleteHandle.Reset();
	}
}

void USKOnlineSessionSubsystem::Deinitialize()
{
	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		ClearDelegateHandles(Subsystem->GetSessionInterface());
	}

	SessionSearch.Reset();
	CurrentOp = ESKSessionOp::None;

	OnSessionsFound.Clear();
	OnHostReady.Clear();
	OnJoinReady.Clear();
	OnSessionError.Clear();

	Super::Deinitialize();
}
```

- [ ] **Step 7: 빌드 확인 (사용자)**

빌드만 통과하면 된다. 이 Task 에는 자동화 테스트가 없다 — 기존 테스트(`SKOnline.Session.*` 8건)가 여전히 PASS 인지만 확인한다.

- [ ] **Step 8: 스테이징 + 커밋 메시지(안)**

```bash
git add SkillProject/Plugins/SKOnline/Source/SKOnline/Public/SKOnlineSessionSubsystem.h SkillProject/Plugins/SKOnline/Source/SKOnline/Private/SKOnlineSessionSubsystem.cpp
```

커밋 메시지(안): `[Feature] USKOnlineSessionSubsystem — 세션 생성/검색/조인/파괴`

---

### Task 5: 게임 모듈 배선 — 트래블 주소 결정

**Files:**
- Modify: `SkillProject/Source/SkillProject/SkillProject.Build.cs`
- Modify: `SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.h`
- Modify: `SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.cpp`
- Test: `SkillProject/Source/SkillProject/Manager/Tests/SpySessionTravelTests.cpp`

**Interfaces:**
- Consumes: Task 4 서브시스템(이 Task 에서는 아직 호출하지 않는다 — 주소를 받을 통로만 뚫는다)
- Produces: `USpyLoadingSubsystem::ResolveTravelAddress(const FString& OverrideAddress, const FString& ConfigAddress)` → `FString` (정적). `USpyLoadingSubsystem::MakeListenTravelURL(const FString& InMapPackageName)` → `FString` (정적). `USpyLoadingSubsystem::EnterGameplay(const FString& OverrideAddress = TEXT(""))`. `USpyLoadingSubsystem::HostAndEnter()`.

- [ ] **Step 1: 실패하는 테스트를 먼저 작성**

`SkillProject/Source/SkillProject/Manager/Tests/SpySessionTravelTests.cpp`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Manager/SpyLoadingSubsystem.h"

//# 주소 우선순위 — 조인이 넘긴 override 가 config 보다 우선한다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyResolveTravelAddressTest,
	"SkillProject.Manager.Session.ResolveTravelAddress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyResolveTravelAddressTest::RunTest(const FString& Parameters)
{
	//# 조인 경로 — override 가 이긴다
	TestEqual(TEXT("Override wins over config"),
		USpyLoadingSubsystem::ResolveTravelAddress(TEXT("10.0.0.5:7777"), TEXT("127.0.0.1:7777")),
		FString(TEXT("10.0.0.5:7777")));

	//# 기존 자동 접속 경로 — override 가 없으면 config 를 쓴다(회귀 방지)
	TestEqual(TEXT("Config used when no override"),
		USpyLoadingSubsystem::ResolveTravelAddress(TEXT(""), TEXT("127.0.0.1:7777")),
		FString(TEXT("127.0.0.1:7777")));

	//# 둘 다 비면 오프라인 폴백 판정으로 넘어가야 하므로 빈 문자열
	TestEqual(TEXT("Empty when both empty"),
		USpyLoadingSubsystem::ResolveTravelAddress(TEXT(""), TEXT("")),
		FString(TEXT("")));

	//# override 가 있으면 config 가 비어 있어도 접속한다
	TestEqual(TEXT("Override alone is enough"),
		USpyLoadingSubsystem::ResolveTravelAddress(TEXT("10.0.0.5:7777"), TEXT("")),
		FString(TEXT("10.0.0.5:7777")));

	return true;
}

//# 리슨 서버 URL — 맵 패키지명에 ?listen 옵션을 붙인다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMakeListenTravelURLTest,
	"SkillProject.Manager.Session.MakeListenTravelURL",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMakeListenTravelURLTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Appends listen option"),
		USpyLoadingSubsystem::MakeListenTravelURL(TEXT("/Game/Spy/Maps/DevMap")),
		FString(TEXT("/Game/Spy/Maps/DevMap?listen")));

	//# 이미 옵션이 붙어 있으면 중복해서 붙이지 않는다
	TestEqual(TEXT("Does not duplicate listen option"),
		USpyLoadingSubsystem::MakeListenTravelURL(TEXT("/Game/Spy/Maps/DevMap?listen")),
		FString(TEXT("/Game/Spy/Maps/DevMap?listen")));

	//# 빈 맵 이름은 빈 문자열 — 호출부가 트래블을 중단해야 한다
	TestEqual(TEXT("Empty map yields empty url"),
		USpyLoadingSubsystem::MakeListenTravelURL(TEXT("")),
		FString(TEXT("")));

	return true;
}

#endif
```

- [ ] **Step 2: 테스트가 실패하는지 확인**

두 정적 함수가 없어 컴파일 실패가 기대값이다.

- [ ] **Step 3: `Build.cs` 에 `SKOnline` 의존 추가**

`SkillProject/Source/SkillProject/SkillProject.Build.cs` 의 `PublicDependencyModuleNames` 배열에서 `"SKUICore",` 다음 줄에 추가한다:

```csharp
            "SKUICore",
            "SKOnline",
```

- [ ] **Step 4: 헤더에 정적 함수 + 신규 API 선언**

`SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.h` 를 아래처럼 고친다.

(a) `EnterGameplay` 선언을 교체한다 — 기존:

```cpp
	//# "접속" 버튼이 호출 — 로딩 완료 후 게임플레이 맵으로 전환 개시
	void EnterGameplay();
```

교체 후:

```cpp
	//# 게임플레이 전환 개시. OverrideAddress 가 비어 있으면 기존 config 경로(자동 접속/오프라인 폴백)를 탄다.
	//# 방 목록에서 조인하면 해석된 접속 문자열이 여기로 들어온다.
	void EnterGameplay(const FString& OverrideAddress = TEXT(""));

	//# 방 만들기 — 맵 로드 후 리슨 서버로 자기 월드를 연다
	void HostAndEnter();
```

(b) 정적 순수 함수 블록에 두 개를 추가한다 (`ConnectPhaseDisplayed` 선언 다음 줄):

```cpp
	//# 트래블 주소 결정 — override(조인) 가 config(자동 접속) 보다 우선. 둘 다 비면 빈 문자열
	static FString ResolveTravelAddress(const FString& OverrideAddress, const FString& ConfigAddress);

	//# 리슨 서버 트래블 URL — 맵 패키지명에 ?listen 을 붙인다(중복 방지)
	static FString MakeListenTravelURL(const FString& InMapPackageName);
```

(c) 상태 멤버를 추가한다 (`bMapPhase` 선언 다음 줄):

```cpp
	//# 이번 전환이 리슨 서버 호스팅인지 — true 면 ClientTravel 대신 ServerTravel(?listen)
	bool bHostingListenServer = false;

	//# 조인이 넘긴 접속 문자열. 비어 있으면 config ServerAddress 를 쓴다
	FString PendingOverrideAddress;
```

- [ ] **Step 5: 정적 함수 구현**

`SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.cpp` 의 `ConnectPhaseDisplayed` 구현 바로 아래에 추가한다:

```cpp
FString USpyLoadingSubsystem::ResolveTravelAddress(const FString& OverrideAddress, const FString& ConfigAddress)
{
	//# 방 목록에서 조인한 주소가 항상 우선한다
	if (OverrideAddress.IsEmpty() == false)
		return OverrideAddress;

	return ConfigAddress;
}

FString USpyLoadingSubsystem::MakeListenTravelURL(const FString& InMapPackageName)
{
	if (InMapPackageName.IsEmpty())
		return FString();

	//# 이미 리슨 옵션이 붙어 있으면 그대로 둔다
	if (InMapPackageName.Contains(TEXT("?listen")))
		return InMapPackageName;

	return InMapPackageName + TEXT("?listen");
}
```

- [ ] **Step 6: `EnterGameplay` 시그니처 반영 + `HostAndEnter` 구현**

`EnterGameplay` 정의를 아래로 교체한다 (기존 본문은 유지하고 첫 줄 시그니처와 override 저장만 추가):

```cpp
void USpyLoadingSubsystem::EnterGameplay(const FString& OverrideAddress)
{
	//# 에셋 로딩이 끝나 버튼이 떠 있을 때만, 중복 없이
	if (bReadyToEnter == false || bMapPhase)
		return;

	//# 조인 경로면 해석된 접속 문자열을 기억해 둔다(전환 시 config 보다 우선)
	PendingOverrideAddress = OverrideAddress;

	UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 접속 개시 — 맵 로딩 시작 (override=%s)"),
		OverrideAddress.IsEmpty() ? TEXT("<none>") : *OverrideAddress);

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

void USpyLoadingSubsystem::HostAndEnter()
{
	if (bReadyToEnter == false || bMapPhase)
		return;

	//# 리슨 서버 플래그를 세운 뒤 공용 전환 경로를 탄다
	bHostingListenServer = true;

	UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 방 만들기 — 리슨 서버 전환 개시"));
	EnterGameplay(TEXT(""));
}
```

- [ ] **Step 7: `TransitionToGameplayMap` 에 호스팅/override 분기 반영**

`TransitionToGameplayMap` 의 본문 시작 부분(`bLoading = false;` 다음)에 리슨 분기를 넣고, 접속 주소를 `ResolveTravelAddress` 로 바꾼다:

```cpp
void USpyLoadingSubsystem::TransitionToGameplayMap()
{
	bTransitionStarted = true;

	//# 로딩 단계 틱 종료 — 이후는 접속(타이머) 또는 오프라인 OpenLevel 이 담당
	bLoading = false;

	//# 리슨 서버 호스팅 — 자기 월드를 서버로 연다. 로딩맵은 NM_Standalone(권한 보유)이라 ServerTravel 이 성립한다.
	if (bHostingListenServer)
	{
		const FString ListenURL = MakeListenTravelURL(MapPackageName.ToString());
		if (ListenURL.IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] 리슨 트래블 URL 이 비어 있습니다 — 전환 중단"));
			return;
		}

		UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
		if (World == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("# [SpyLoadingSubsystem] 월드가 없습니다 — 리슨 전환 중단"));
			return;
		}

		if (DisplayedProgress < 1.f)
		{
			DisplayedProgress = 1.f;
			OnProgressChanged.Broadcast(DisplayedProgress);
		}

		UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 리슨 서버 전환: %s"), *ListenURL);
		World->ServerTravel(ListenURL);
		return;
	}

	//# 접속 모드 — override(조인) 또는 config 주소로 ClientTravel
	const FString TravelAddress = ResolveTravelAddress(PendingOverrideAddress, ServerAddress);
	if (ShouldConnectToServer(TravelAddress))
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

		UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 서버 접속 개시: %s"), *TravelAddress);
		PC->ClientTravel(TravelAddress, ETravelType::TRAVEL_Absolute);
		return;
	}

	//# 이하 오프라인 폴백 분기는 기존 코드를 그대로 둔다
	// ... (변경 없음)
}
```

> `RetryConnect` 도 `ResolveTravelAddress(PendingOverrideAddress, ServerAddress)` 를 쓰도록 고친다 — 그러지 않으면 조인 실패 후 재시도가 조인 주소가 아니라 config 주소로 간다. `RetryConnect` 안의 `ShouldConnectToServer(ServerAddress)` 판정과 `PC->ClientTravel(ServerAddress, ...)` 두 곳을 `ResolveTravelAddress` 결과로 교체한다.

- [ ] **Step 8: `Deinitialize` 에 신규 상태 초기화 추가**

`Deinitialize` 의 `bMapPhase = false;` 다음 줄에 추가:

```cpp
	bHostingListenServer = false;
	PendingOverrideAddress.Reset();
```

- [ ] **Step 9: 빌드 + 테스트 실행 (사용자)**

Automation 창에서 실행:
- `SkillProject.Manager.Session.*` 2건 (신규) → PASS 기대
- `SkillProject.Manager.Loading.*` (기존 회귀) → 전부 PASS 유지 기대

- [ ] **Step 10: 스테이징 + 커밋 메시지(안)**

```bash
git add SkillProject/Source/SkillProject/SkillProject.Build.cs SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.h SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.cpp SkillProject/Source/SkillProject/Manager/Tests/SpySessionTravelTests.cpp
```

커밋 메시지(안): `[Feature] USpyLoadingSubsystem — 조인 주소 override + 리슨 서버 전환`

---

### Task 6: 방 1행 위젯 `USpySessionRowWidget`

**Files:**
- Create: `SkillProject/Source/SkillProject/UI/SpySessionRowWidget.h`
- Create: `SkillProject/Source/SkillProject/UI/SpySessionRowWidget.cpp`

**Interfaces:**
- Consumes: Task 1 `FSKSessionInfo`
- Produces: `USpySessionRowWidget` — `SetSessionInfo(const FSKSessionInfo& InInfo)`, 델리게이트 `FOnSpySessionRowClicked OnRowClicked` (`int32 SearchResultIndex`). BindWidget 이름: `RoomNameText`(UTextBlock), `PlayersText`(UTextBlock), `PingText`(UTextBlock), `JoinButton`(UButton).

> 이 Task 에는 자동화 테스트가 없다 — 위젯 바인딩은 WBP 에셋이 있어야 성립하므로 Task 10 의 수동 검증에서 확인한다. 표시 문자열 조립은 순수 로직이라 정적 함수로 빼서 Task 7 의 테스트에서 함께 커버한다.

- [ ] **Step 1: 헤더 작성**

`SkillProject/Source/SkillProject/UI/SpySessionRowWidget.h`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SKUserWidget.h"
#include "SKOnlineTypes.h"

#include "SpySessionRowWidget.generated.h"

class UTextBlock;
class UButton;

//# 행 클릭 — 브라우저가 구독해 조인을 시작한다
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSpySessionRowClicked, int32);

//# 방 목록의 한 줄. 표시만 하고 조인은 브라우저가 수행한다.
UCLASS()
class SKILLPROJECT_API USpySessionRowWidget : public USKUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	//# 표시 내용 갱신 — 브라우저가 검색 결과마다 호출한다
	void SetSessionInfo(const FSKSessionInfo& InInfo);

	//# 인원 표시 문자열 — "2 / 4"
	static FString MakePlayersText(int32 CurrentPlayers, int32 MaxPlayers);

	//# 핑 표시 문자열 — "30 ms". 음수/미측정은 "-- ms"
	static FString MakePingText(int32 PingMs);

public:
	FOnSpySessionRowClicked OnRowClicked;

protected:
	UFUNCTION()
	void OnJoinClicked();

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UTextBlock> RoomNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UTextBlock> PlayersText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = true))
	TObjectPtr<UTextBlock> PingText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UButton> JoinButton;

	//# 이 행이 가리키는 검색 결과 인덱스
	int32 SearchResultIndex = INDEX_NONE;
};
```

- [ ] **Step 2: 구현 작성**

`SkillProject/Source/SkillProject/UI/SpySessionRowWidget.cpp`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SpySessionRowWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpySessionRowWidget)

void USpySessionRowWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(JoinButton))
	{
		JoinButton->OnClicked.AddDynamic(this, &USpySessionRowWidget::OnJoinClicked);
	}
}

void USpySessionRowWidget::NativeDestruct()
{
	if (IsValid(JoinButton))
	{
		JoinButton->OnClicked.RemoveDynamic(this, &USpySessionRowWidget::OnJoinClicked);
	}

	OnRowClicked.Clear();

	Super::NativeDestruct();
}

FString USpySessionRowWidget::MakePlayersText(int32 CurrentPlayers, int32 MaxPlayers)
{
	return FString::Printf(TEXT("%d / %d"), FMath::Max(CurrentPlayers, 0), FMath::Max(MaxPlayers, 0));
}

FString USpySessionRowWidget::MakePingText(int32 PingMs)
{
	//# 아직 측정되지 않은 핑은 0/음수로 오므로 숫자 대신 자리표시자를 쓴다
	if (PingMs <= 0)
		return TEXT("-- ms");

	return FString::Printf(TEXT("%d ms"), PingMs);
}

void USpySessionRowWidget::SetSessionInfo(const FSKSessionInfo& InInfo)
{
	SearchResultIndex = InInfo.SearchResultIndex;

	if (IsValid(RoomNameText))
	{
		RoomNameText->SetText(FText::FromString(InInfo.RoomName));
	}

	if (IsValid(PlayersText))
	{
		PlayersText->SetText(FText::FromString(MakePlayersText(InInfo.CurrentPlayers, InInfo.MaxPlayers)));
	}

	if (IsValid(PingText))
	{
		PingText->SetText(FText::FromString(MakePingText(InInfo.PingMs)));
	}

	//# 꽉 찬 방은 누를 수 없게 한다
	if (IsValid(JoinButton))
	{
		const bool bFull = (InInfo.MaxPlayers > 0) && (InInfo.CurrentPlayers >= InInfo.MaxPlayers);
		JoinButton->SetIsEnabled(bFull == false);
	}
}

void USpySessionRowWidget::OnJoinClicked()
{
	if (SearchResultIndex == INDEX_NONE)
		return;

	OnRowClicked.Broadcast(SearchResultIndex);
}
```

- [ ] **Step 3: 빌드 확인 (사용자)**

빌드 통과만 확인한다.

- [ ] **Step 4: 스테이징 + 커밋 메시지(안)**

```bash
git add SkillProject/Source/SkillProject/UI/SpySessionRowWidget.h SkillProject/Source/SkillProject/UI/SpySessionRowWidget.cpp
```

커밋 메시지(안): `[Feature] USpySessionRowWidget — 방 목록 1행 위젯`

---

### Task 7: 방 목록 위젯 `USpySessionBrowserWidget`

**Files:**
- Create: `SkillProject/Source/SkillProject/UI/SpySessionBrowserWidget.h`
- Create: `SkillProject/Source/SkillProject/UI/SpySessionBrowserWidget.cpp`
- Modify: `SkillProject/Source/SkillProject/Util/DefineEnum.h`
- Test: `SkillProject/Source/SkillProject/UI/Tests/SpySessionBrowserTests.cpp`

**Interfaces:**
- Consumes: Task 4 `USKOnlineSessionSubsystem` 델리게이트, Task 5 `USpyLoadingSubsystem::EnterGameplay/HostAndEnter`, Task 6 `USpySessionRowWidget`
- Produces: `USpySessionBrowserWidget`. BindWidget 이름: `SessionListBox`(**UPanelWidget**), `RefreshButton`(UButton), `HostButton`(UButton), `StatusText`(UTextBlock). `ESpyUIType::SessionBrowser` enum 값. 정적 `MakeStatusMessage(ESKSessionOp, ESKSessionError)` → `FString`.

> **`SessionListBox` 를 `UPanelWidget` 으로 선언하는 이유**: 구체 타입(`UVerticalBox`)으로 못박으면 Task 9 의 목업 승인 결과가 ScrollBox 로 나왔을 때 이 Task 를 다시 손봐야 한다. `UPanelWidget` + `AddChild`/`ClearChildren` 은 VerticalBox·ScrollBox 어느 쪽이든 BindWidget 을 충족하므로, C++ 이 아직 승인되지 않은 레이아웃에 묶이지 않는다.

- [ ] **Step 1: 실패하는 테스트를 먼저 작성**

`SkillProject/Source/SkillProject/UI/Tests/SpySessionBrowserTests.cpp`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UI/SpySessionBrowserWidget.h"
#include "UI/SpySessionRowWidget.h"

//# 행 표시 문자열
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpySessionRowTextTest,
	"SkillProject.UI.SessionBrowser.RowText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpySessionRowTextTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Players text"), USpySessionRowWidget::MakePlayersText(2, 4), FString(TEXT("2 / 4")));
	TestEqual(TEXT("Players text clamps negatives"), USpySessionRowWidget::MakePlayersText(-1, -4), FString(TEXT("0 / 0")));

	TestEqual(TEXT("Ping text"), USpySessionRowWidget::MakePingText(30), FString(TEXT("30 ms")));
	TestEqual(TEXT("Unmeasured ping"), USpySessionRowWidget::MakePingText(0), FString(TEXT("-- ms")));
	TestEqual(TEXT("Negative ping"), USpySessionRowWidget::MakePingText(-5), FString(TEXT("-- ms")));

	return true;
}

//# 에러 사유 → 사용자 문구. OSS 원문을 그대로 노출하지 않는다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpySessionStatusMessageTest,
	"SkillProject.UI.SessionBrowser.StatusMessage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpySessionStatusMessageTest::RunTest(const FString& Parameters)
{
	//# 온라인 기능 자체가 없는 경우
	TestEqual(TEXT("No online subsystem"),
		USpySessionBrowserWidget::MakeStatusMessage(ESKSessionOp::Finding, ESKSessionError::NoOnlineSubsystem),
		FString(TEXT("온라인 기능을 사용할 수 없습니다")));

	//# 작업별 문구가 구분된다
	TestEqual(TEXT("Create failed"),
		USpySessionBrowserWidget::MakeStatusMessage(ESKSessionOp::Hosting, ESKSessionError::CreateFailed),
		FString(TEXT("방을 만들지 못했습니다")));

	TestEqual(TEXT("Find failed"),
		USpySessionBrowserWidget::MakeStatusMessage(ESKSessionOp::Finding, ESKSessionError::FindFailed),
		FString(TEXT("방 목록을 불러오지 못했습니다")));

	TestEqual(TEXT("Join failed"),
		USpySessionBrowserWidget::MakeStatusMessage(ESKSessionOp::Joining, ESKSessionError::JoinFailed),
		FString(TEXT("방에 들어가지 못했습니다")));

	//# 중복 입력은 사용자에게 에러로 보이면 안 된다 — 빈 문자열이면 표시하지 않는다
	TestEqual(TEXT("Busy is silent"),
		USpySessionBrowserWidget::MakeStatusMessage(ESKSessionOp::Finding, ESKSessionError::Busy),
		FString(TEXT("")));

	return true;
}

#endif
```

- [ ] **Step 2: 테스트가 실패하는지 확인**

`SpySessionBrowserWidget.h` 부재로 컴파일 실패가 기대값이다.

- [ ] **Step 3: `ESpyUIType` 에 `SessionBrowser` 추가**

`SkillProject/Source/SkillProject/Util/DefineEnum.h` 의 enum 을 아래로 교체한다:

```cpp
UENUM(BlueprintType)
enum ESpyUIType : uint8
{
	None UMETA(DisplayName = "None"),
	MainHUD UMETA(DisplayName = "MainHUD"),
	HpBar UMETA(DisplayName = "HpBar"),
	Loading UMETA(DisplayName = "Loading"),
	SessionBrowser UMETA(DisplayName = "SessionBrowser"),
};
```

> 기존 값의 순서를 바꾸지 않고 **끝에만 추가**한다. UI 는 이름 기반이라 안전하지만, 값 이동은 저장된 에셋의 enum 인덱스를 깨뜨린다.

- [ ] **Step 4: 헤더 작성**

`SkillProject/Source/SkillProject/UI/SpySessionBrowserWidget.h`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SKUserWidget.h"
#include "SKOnlineTypes.h"

#include "SpySessionBrowserWidget.generated.h"

class UPanelWidget;
class UButton;
class UTextBlock;
class USpySessionRowWidget;

//# 방 목록 화면 — 검색/생성/조인을 트리거하고 결과를 행으로 그린다.
//# 세션 작업은 SKOnline 이, 트래블은 SpyLoadingSubsystem 이 맡는다.
UCLASS()
class SKILLPROJECT_API USpySessionBrowserWidget : public USKUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	//# 세션 에러 → 사용자 문구. 빈 문자열이면 표시하지 않는다(무음 처리)
	static FString MakeStatusMessage(ESKSessionOp Op, ESKSessionError Error);

protected:
	//# SKOnline 델리게이트 핸들러
	void HandleSessionsFound(const TArray<FSKSessionInfo>& Infos);
	void HandleHostReady();
	void HandleJoinReady(const FString& ConnectString);
	void HandleSessionError(ESKSessionOp Op, ESKSessionError Error, const FString& Detail);

	//# 버튼
	UFUNCTION()
	void OnRefreshClicked();

	UFUNCTION()
	void OnHostClicked();

	//# 행 클릭 → 조인
	void HandleRowClicked(int32 SearchResultIndex);

	//# 전환 개시 — 브라우저를 닫고 게임 입력 모드로 되돌린다
	void CloseForTravel();

	//# 상태 문구 표시(빈 문자열이면 숨김)
	void SetStatus(const FString& Message);

protected:
	//# 구체 패널 타입을 못박지 않는다 — VerticalBox/ScrollBox 어느 쪽이든 받는다(목업 승인 결과에 독립)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UPanelWidget> SessionListBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UButton> RefreshButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UButton> HostButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = true))
	TObjectPtr<UTextBlock> StatusText;

	//# 목록 안에 반복 생성되는 조각이라 UI 매니저가 아니라 이 프로퍼티로 참조한다
	UPROPERTY(EditDefaultsOnly, Category = "SessionBrowser")
	TSubclassOf<USpySessionRowWidget> RowWidgetClass;

	FDelegateHandle SessionsFoundHandle;
	FDelegateHandle HostReadyHandle;
	FDelegateHandle JoinReadyHandle;
	FDelegateHandle SessionErrorHandle;
};
```

- [ ] **Step 5: 구현 작성**

`SkillProject/Source/SkillProject/UI/SpySessionBrowserWidget.cpp`:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SpySessionBrowserWidget.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "SKOnlineSessionSubsystem.h"
#include "Manager/SpyLoadingSubsystem.h"
#include "UI/SpySessionRowWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpySessionBrowserWidget)

FString USpySessionBrowserWidget::MakeStatusMessage(ESKSessionOp Op, ESKSessionError Error)
{
	//# 중복 입력 가드는 사용자 잘못이 아니므로 아무것도 보이지 않는다
	if (Error == ESKSessionError::Busy || Error == ESKSessionError::None)
		return FString();

	if (Error == ESKSessionError::NoOnlineSubsystem)
		return TEXT("온라인 기능을 사용할 수 없습니다");

	switch (Op)
	{
	case ESKSessionOp::Hosting:
		return TEXT("방을 만들지 못했습니다");

	case ESKSessionOp::Finding:
		return TEXT("방 목록을 불러오지 못했습니다");

	case ESKSessionOp::Joining:
		return TEXT("방에 들어가지 못했습니다");

	default:
		return TEXT("온라인 요청을 처리하지 못했습니다");
	}
}

void USpySessionBrowserWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(RefreshButton))
	{
		RefreshButton->OnClicked.AddDynamic(this, &USpySessionBrowserWidget::OnRefreshClicked);
	}
	if (IsValid(HostButton))
	{
		HostButton->OnClicked.AddDynamic(this, &USpySessionBrowserWidget::OnHostClicked);
	}

	SetStatus(FString());

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance == nullptr)
		return;

	USKOnlineSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<USKOnlineSessionSubsystem>();
	if (SessionSubsystem == nullptr)
	{
		SetStatus(MakeStatusMessage(ESKSessionOp::Finding, ESKSessionError::NoOnlineSubsystem));
		return;
	}

	SessionsFoundHandle = SessionSubsystem->OnSessionsFound.AddUObject(this, &USpySessionBrowserWidget::HandleSessionsFound);
	HostReadyHandle = SessionSubsystem->OnHostReady.AddUObject(this, &USpySessionBrowserWidget::HandleHostReady);
	JoinReadyHandle = SessionSubsystem->OnJoinReady.AddUObject(this, &USpySessionBrowserWidget::HandleJoinReady);
	SessionErrorHandle = SessionSubsystem->OnSessionError.AddUObject(this, &USpySessionBrowserWidget::HandleSessionError);

	//# 화면이 뜨자마자 한 번 검색한다 — 사용자가 새로고침을 먼저 누르지 않아도 되게
	SessionSubsystem->FindSessions();
}

void USpySessionBrowserWidget::NativeDestruct()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USKOnlineSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<USKOnlineSessionSubsystem>())
		{
			SessionSubsystem->OnSessionsFound.Remove(SessionsFoundHandle);
			SessionSubsystem->OnHostReady.Remove(HostReadyHandle);
			SessionSubsystem->OnJoinReady.Remove(JoinReadyHandle);
			SessionSubsystem->OnSessionError.Remove(SessionErrorHandle);
		}
	}

	SessionsFoundHandle.Reset();
	HostReadyHandle.Reset();
	JoinReadyHandle.Reset();
	SessionErrorHandle.Reset();

	if (IsValid(RefreshButton))
	{
		RefreshButton->OnClicked.RemoveDynamic(this, &USpySessionBrowserWidget::OnRefreshClicked);
	}
	if (IsValid(HostButton))
	{
		HostButton->OnClicked.RemoveDynamic(this, &USpySessionBrowserWidget::OnHostClicked);
	}

	Super::NativeDestruct();
}

void USpySessionBrowserWidget::SetStatus(const FString& Message)
{
	if (IsValid(StatusText) == false)
		return;

	if (Message.IsEmpty())
	{
		StatusText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	StatusText->SetText(FText::FromString(Message));
	StatusText->SetVisibility(ESlateVisibility::Visible);
}

void USpySessionBrowserWidget::HandleSessionsFound(const TArray<FSKSessionInfo>& Infos)
{
	if (IsValid(SessionListBox) == false)
		return;

	SessionListBox->ClearChildren();

	if (Infos.Num() == 0)
	{
		//# 결과 0건은 에러가 아니다 — 안내만 띄우고 새로고침을 남겨 둔다
		SetStatus(TEXT("방이 없습니다"));
		return;
	}

	SetStatus(FString());

	if (RowWidgetClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("# [SpySessionBrowserWidget] RowWidgetClass 가 지정되지 않았습니다"));
		return;
	}

	for (const FSKSessionInfo& Info : Infos)
	{
		USpySessionRowWidget* Row = CreateWidget<USpySessionRowWidget>(this, RowWidgetClass);
		if (Row == nullptr)
			continue;

		Row->SetSessionInfo(Info);
		Row->OnRowClicked.AddUObject(this, &USpySessionBrowserWidget::HandleRowClicked);

		//# 패널 공용 API — VerticalBox/ScrollBox 어느 쪽이든 동작한다
		SessionListBox->AddChild(Row);
	}
}

void USpySessionBrowserWidget::OnRefreshClicked()
{
	SetStatus(FString());

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USKOnlineSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<USKOnlineSessionSubsystem>())
		{
			SessionSubsystem->FindSessions();
		}
	}
}

void USpySessionBrowserWidget::OnHostClicked()
{
	SetStatus(FString());

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USKOnlineSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<USKOnlineSessionSubsystem>())
		{
			SessionSubsystem->HostSession();
		}
	}
}

void USpySessionBrowserWidget::HandleRowClicked(int32 SearchResultIndex)
{
	SetStatus(FString());

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USKOnlineSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<USKOnlineSessionSubsystem>())
		{
			SessionSubsystem->JoinSessionByIndex(SearchResultIndex);
		}
	}
}

void USpySessionBrowserWidget::HandleHostReady()
{
	CloseForTravel();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USpyLoadingSubsystem* LoadingSubsystem = GameInstance->GetSubsystem<USpyLoadingSubsystem>())
		{
			LoadingSubsystem->HostAndEnter();
		}
	}
}

void USpySessionBrowserWidget::HandleJoinReady(const FString& ConnectString)
{
	CloseForTravel();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USpyLoadingSubsystem* LoadingSubsystem = GameInstance->GetSubsystem<USpyLoadingSubsystem>())
		{
			LoadingSubsystem->EnterGameplay(ConnectString);
		}
	}
}

void USpySessionBrowserWidget::HandleSessionError(ESKSessionOp Op, ESKSessionError Error, const FString& Detail)
{
	//# 원문 사유는 로그로만 남기고 화면에는 고정 한국어를 쓴다
	UE_LOG(LogTemp, Error, TEXT("# [SpySessionBrowserWidget] 세션 오류: %s"), *Detail);

	SetStatus(MakeStatusMessage(Op, Error));
}

void USpySessionBrowserWidget::CloseForTravel()
{
	//# 트래블로 월드와 함께 사라지는 데 기대지 않고 명시적으로 닫는다(전환 전 잔상·오클릭 방지)
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (APlayerController* PC = GameInstance->GetFirstLocalPlayerController())
		{
			PC->bShowMouseCursor = false;
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
		}
	}

	Close();
}
```

- [ ] **Step 6: 빌드 + 테스트 실행 (사용자)**

Automation 창에서 `SkillProject.UI.SessionBrowser.*` 2건 실행. 기대: PASS.

- [ ] **Step 7: 스테이징 + 커밋 메시지(안)**

```bash
git add SkillProject/Source/SkillProject/UI/SpySessionBrowserWidget.h SkillProject/Source/SkillProject/UI/SpySessionBrowserWidget.cpp SkillProject/Source/SkillProject/Util/DefineEnum.h SkillProject/Source/SkillProject/UI/Tests/SpySessionBrowserTests.cpp
```

커밋 메시지(안): `[Feature] USpySessionBrowserWidget — 방 목록 화면`

---

### Task 8: 로딩 위젯 분기 — 방 목록 띄우기

**Files:**
- Modify: `SkillProject/Plugins/SKUICore/Source/SKUICore/Public/SKUIManager.h`
- Modify: `SkillProject/Plugins/SKUICore/Source/SKUICore/Private/SKUIManager.cpp`
- Modify: `SkillProject/Source/SkillProject/Manager/SpyUIManager.h`
- Modify: `SkillProject/Source/SkillProject/Manager/SpyUIManager.cpp`
- Modify: `SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.h`
- Modify: `SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.cpp`
- Modify: `SkillProject/Source/SkillProject/UI/SpyLoadingWidget.cpp`
- Test: `SkillProject/Source/SkillProject/Manager/Tests/SpySessionTravelTests.cpp` (Task 5 파일에 케이스 추가)

**Interfaces:**
- Consumes: Task 7 `ESpyUIType::SessionBrowser`
- Produces: `USKUIManager::OpenUI(FName InUIName, int32 ZOrder = 0)`. `USpyUIManager::OpenSpyUI(ESpyUIType UIType, int32 ZOrder = 0)`. `USpyLoadingSubsystem::ShouldShowSessionBrowser(const FString& ConfigAddress)` → `bool` (정적). `USpyLoadingSubsystem::HasConfiguredServerAddress()` → `bool` (인스턴스, 위젯이 조회).

> **⛔ 이 Task 가 먼저 고치는 블로커 (Step 1~2)**
>
> persistent 로딩 UI 는 `OpenPersistentSpyUI(Loading)` 로 **ZOrder 100** 에 얹히는데(`SKUIManager.cpp:262` `AddViewportWidgetContent(TakeWidget(), ZOrder)`), 일반 UI 는 `AddToViewport()` 를 **인자 없이** 호출해 **ZOrder 0** 이다(`SKUIManager.cpp:110`·`122`). 로딩 위젯은 전체 화면 불투명 배경(`#08090B`, `loading-scene.md` §5-2)을 가지므로, 방 목록을 그냥 열면 **로딩 화면 뒤에 깔려 보이지 않는다.**
>
> **사용자 결정(A안)**: 호출부에서 우회하지 않고 **`OpenUI` 에 ZOrder 파라미터를 추가**해 원인을 없앤다. `OpenPersistentUI` 는 이미 ZOrder 를 받으므로 두 API 의 비대칭이 사라진다. 기본값 `0` 이라 **기존 호출부는 전부 무영향**이다.

- [ ] **Step 1: `USKUIManager::OpenUI` 에 ZOrder 파라미터 추가**

`SkillProject/Plugins/SKUICore/Source/SKUICore/Public/SKUIManager.h` 의 선언을 교체한다:

```cpp
	//# ZOrder 는 뷰포트 레이어 순서. 기본 0 — persistent UI(기본 100) 위에 띄우려면 그보다 큰 값을 넘긴다.
	UFUNCTION(BlueprintCallable)
	void OpenUI(FName InUIName, int32 ZOrder = 0);
```

`SkillProject/Plugins/SKUICore/Source/SKUICore/Private/SKUIManager.cpp` 에서:

(a) 정의 시그니처를 `void USKUIManager::OpenUI(FName InUIName, int32 ZOrder)` 로 바꾼다.

(b) 람다 캡처에 `ZOrder` 를 추가한다 — `[this, InUIName]` → `[this, InUIName, ZOrder]`.

(c) 람다 안의 `AddToViewport()` 호출 **2곳**(캐시 재사용 경로·신규 생성 경로)에 인자를 넘긴다:

```cpp
	//# 캐싱 중인 UI이면 Open
	OpenUIList.Add(FindCashingUI->Get());
	FindCashingUI->Get()->AddToViewport(ZOrder);
```

```cpp
	UserWidget->SetUIName(InUIName);
	OpenUIList.Add(UserWidget);

	UserWidget->AddToViewport(ZOrder);
```

> `OpenSubUI` 는 `UWidgetComponent` 에 붙이는 경로라 뷰포트 ZOrder 개념이 없다 — 건드리지 않는다.

- [ ] **Step 2: `USpyUIManager::OpenSpyUI` 에 ZOrder 전달**

`SkillProject/Source/SkillProject/Manager/SpyUIManager.h` 의 선언을 교체한다:

```cpp
	UFUNCTION(BlueprintCallable)
	void OpenSpyUI(ESpyUIType UIType, int32 ZOrder = 0);
```

`SkillProject/Source/SkillProject/Manager/SpyUIManager.cpp` 의 정의를 교체한다:

```cpp
void USpyUIManager::OpenSpyUI(ESpyUIType UIType, int32 ZOrder)
{
	FString EnumName = StaticEnum<ESpyUIType>()->GetNameStringByValue((int64)UIType);
	OpenUI(FName(*EnumName), ZOrder);
}
```

> 이 Task 의 뒤 스텝에서 방 목록은 `OpenSpyUI(ESpyUIType::SessionBrowser, 200)` 로 연다. **200 은 로딩 persistent UI 의 100 보다 커야 한다**는 제약에서 나온 값이며, 그 사이(101~199)를 다른 UI 가 쓸 여지를 남긴 간격이다.

- [ ] **Step 3: 실패하는 테스트를 먼저 작성**

`SkillProject/Source/SkillProject/Manager/Tests/SpySessionTravelTests.cpp` 파일 끝(`#endif` 앞)에 추가:

```cpp
//# 방 목록 표시 판정 — config 주소가 비어 있을 때만 방 목록을 띄운다(D6 이중 경로)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyShouldShowSessionBrowserTest,
	"SkillProject.Manager.Session.ShouldShowSessionBrowser",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyShouldShowSessionBrowserTest::RunTest(const FString& Parameters)
{
	//# 기본 경로 — 주소가 비었으므로 방 목록
	TestTrue(TEXT("Empty config address shows browser"),
		USpyLoadingSubsystem::ShouldShowSessionBrowser(TEXT("")));

	//# 데디서버·CI 경로 — 주소가 있으면 기존 자동 접속을 유지한다
	TestFalse(TEXT("Configured address keeps auto connect"),
		USpyLoadingSubsystem::ShouldShowSessionBrowser(TEXT("127.0.0.1:7777")));

	return true;
}
```

- [ ] **Step 4: 테스트가 실패하는지 확인**

`ShouldShowSessionBrowser` 부재로 컴파일 실패가 기대값이다.

- [ ] **Step 5: 헤더에 선언 추가**

`SpyLoadingSubsystem.h` 의 정적 함수 블록(Task 5 에서 추가한 `MakeListenTravelURL` 다음)에 추가:

```cpp
	//# 방 목록을 띄울지 — config 주소가 비어 있을 때만. 채워져 있으면 기존 자동 접속을 유지한다
	static bool ShouldShowSessionBrowser(const FString& ConfigAddress);
```

그리고 `GetDisplayedProgress` 다음에 인스턴스 조회를 추가:

```cpp
	//# 위젯이 분기에 쓰는 조회 — config 에 서버 주소가 지정돼 있는가
	bool HasConfiguredServerAddress() const
	{
		return ServerAddress.IsEmpty() == false;
	}
```

- [ ] **Step 6: 구현 추가**

`SpyLoadingSubsystem.cpp` 의 `MakeListenTravelURL` 구현 아래에 추가:

```cpp
bool USpyLoadingSubsystem::ShouldShowSessionBrowser(const FString& ConfigAddress)
{
	//# 주소가 지정돼 있으면 그쪽이 우선한다(데디서버 붙이기·CI 용 경로 보존)
	return ConfigAddress.IsEmpty();
}
```

- [ ] **Step 7: 로딩 위젯의 완료 처리를 분기시킨다**

`SkillProject/Source/SkillProject/UI/SpyLoadingWidget.cpp` 의 `HandleReadyToEnter` 를 아래로 교체한다:

```cpp
void USpyLoadingWidget::HandleReadyToEnter()
{
	//# 로딩 완료 — 바·퍼센트를 숨긴다(접속 개시 시 다시 보인다)
	if (IsValid(LoadingBar))
	{
		LoadingBar->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (IsValid(PercentText))
	{
		PercentText->SetVisibility(ESlateVisibility::Collapsed);
	}

	//# 버튼·목록을 누를 수 있도록 커서 표시 + UI 입력 모드
	//# (패키지 standalone 은 기본이 GameOnly·커서숨김이라 클릭이 UI 로 안 간다)
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance != nullptr)
	{
		if (APlayerController* PC = GameInstance->GetFirstLocalPlayerController())
		{
			PC->bShowMouseCursor = true;
			FInputModeGameAndUI InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			InputMode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(InputMode);
		}
	}

	USpyLoadingSubsystem* LoadingSubsystem = (GameInstance != nullptr)
		? GameInstance->GetSubsystem<USpyLoadingSubsystem>()
		: nullptr;

	//# config 에 서버 주소가 있으면 기존 "접속" 버튼 경로를 그대로 탄다
	if (LoadingSubsystem != nullptr && LoadingSubsystem->HasConfiguredServerAddress())
	{
		if (IsValid(EnterButton))
		{
			EnterButton->SetVisibility(ESlateVisibility::Visible);
		}
		return;
	}

	//# 주소가 없으면 방 목록을 띄운다. 접속 버튼은 쓰지 않는다.
	if (IsValid(EnterButton))
	{
		EnterButton->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (USpyUIManager* UIManager = USpyUIManager::Get(GameInstance))
	{
		UIManager->OpenSpyUI(ESpyUIType::SessionBrowser, 200);
	}
}
```

같은 파일 상단 include 에 두 줄을 추가한다 (프로젝트 헤더 구역):

```cpp
#include "Manager/SpyUIManager.h"
#include "Util/DefineEnum.h"
```

- [ ] **Step 8: 접속 개시 시 바·퍼센트를 되살린다**

브라우저 경로에서는 `OnEnterClicked` 가 호출되지 않으므로, 바 복원을 서브시스템 쪽 진행률 이벤트에 맡긴다. `SpyLoadingWidget.cpp` 의 `HandleProgressChanged` 첫 부분에 복원 로직을 넣는다:

```cpp
void USpyLoadingWidget::HandleProgressChanged(float InDisplayed)
{
	const float Clamped = FMath::Clamp(InDisplayed, 0.f, 1.f);

	//# 방 목록 경로에서는 OnEnterClicked 를 거치지 않으므로, 진행률이 다시 흐르기 시작하면
	//# (= 전환이 개시되면) 여기서 바·퍼센트를 되살린다.
	if (IsValid(LoadingBar) && LoadingBar->GetVisibility() == ESlateVisibility::Collapsed)
	{
		LoadingBar->SetVisibility(ESlateVisibility::Visible);
	}
	if (IsValid(PercentText) && PercentText->GetVisibility() == ESlateVisibility::Collapsed)
	{
		PercentText->SetVisibility(ESlateVisibility::Visible);
	}

	if (IsValid(LoadingBar))
	{
		LoadingBar->SetPercent(Clamped);
	}

	if (IsValid(PercentText))
	{
		const int32 Percent = FMath::RoundToInt(Clamped * 100.f);

		//# 기획서 §5-1 — 3자리 zero-pad + % 기호 (예: 007%)
		PercentText->SetText(FText::FromString(FString::Printf(TEXT("%03d%%"), Percent)));
	}
}
```

> **주의**: `HandleConnectionFailed` 가 실패 시 둘을 `Collapsed` 로 숨기는데, 실패 후에는 진행률 브로드캐스트가 멈추므로 이 복원 로직이 실패 화면을 되살리지 않는다. `RetryConnect` 가 진행률을 리셋 브로드캐스트할 때 복원되는 것이 의도된 동작이다(기존 `OnRetryClicked` 의 복원과 중복되지만 무해).

- [ ] **Step 9: 조인 후 접속 실패 → 세션 정리 + 방 목록 복귀**

**메우는 구멍**: 조인이 성공해 `ClientTravel` 이 개시된 뒤 네트워크 실패·타임아웃이 나면, 사용자는 로딩맵 에러 화면에 남는데 **조인된 세션은 살아 있고 방 목록으로 돌아갈 길이 없다.** 기존 "재시도" 버튼은 같은 주소로 다시 갈 뿐이라 호스트가 이미 사라진 경우 회복 수단이 아니다. 방 목록 경로에서는 재시도 대신 **세션을 정리하고 목록으로 되돌린다.**

`SkillProject/Source/SkillProject/UI/SpyLoadingWidget.cpp` 의 `HandleConnectionFailed` 를 아래로 교체한다:

```cpp
void USpyLoadingWidget::HandleConnectionFailed(const FString& Reason)
{
	//# 실패 시 바·퍼센트를 숨긴다 — "실패했는데 100%" 모순 제거(사용자 결정).
	if (IsValid(LoadingBar))
	{
		LoadingBar->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (IsValid(PercentText))
	{
		PercentText->SetVisibility(ESlateVisibility::Collapsed);
	}

	UGameInstance* GameInstance = GetGameInstance();
	USpyLoadingSubsystem* LoadingSubsystem = (GameInstance != nullptr)
		? GameInstance->GetSubsystem<USpyLoadingSubsystem>()
		: nullptr;

	//# 방 목록 경로(config 주소 없음) — 재시도 대신 세션을 정리하고 목록으로 되돌린다.
	//# 같은 주소로 재시도해 봐야 호스트가 사라진 경우 회복되지 않는다.
	if (LoadingSubsystem != nullptr && LoadingSubsystem->HasConfiguredServerAddress() == false)
	{
		if (USKOnlineSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<USKOnlineSessionSubsystem>())
		{
			SessionSubsystem->DestroyCurrentSession();
		}

		if (USpyUIManager* UIManager = USpyUIManager::Get(GameInstance))
		{
			UIManager->OpenSpyUI(ESpyUIType::SessionBrowser, 200);
		}

		//# 브라우저가 입력을 받아야 하므로 커서·UI 입력 모드를 되살린다
		if (APlayerController* PC = GameInstance->GetFirstLocalPlayerController())
		{
			PC->bShowMouseCursor = true;
			FInputModeGameAndUI InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			InputMode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(InputMode);
		}

		return;
	}

	//# 기존 config 주소 경로 — 에러 메시지 + 재시도 버튼(문구는 기획서 §9-3 확정값)
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
```

같은 파일 상단 include 에 한 줄을 추가한다:

```cpp
#include "SKOnlineSessionSubsystem.h"
```

> **전환 상태를 되돌려야 한다.** `HandleConnectionFailed` 이후 사용자가 방 목록에서 다시 조인하려면 `EnterGameplay` 의 진입 가드(`bReadyToEnter == false || bMapPhase`)를 통과해야 한다. `USpyLoadingSubsystem::HandleConnectFailed` 끝에 아래를 추가해 브라우저 경로에서 재진입이 가능하게 한다:
>
> ```cpp
> 	//# 방 목록 경로는 재시도 버튼이 아니라 목록 복귀로 회복하므로, 다시 조인할 수 있게 상태를 되돌린다
> 	if (ServerAddress.IsEmpty())
> 	{
> 		bMapPhase = false;
> 		bTransitionStarted = false;
> 		bHostingListenServer = false;
> 		PendingOverrideAddress.Reset();
> 		bReadyToEnter = true;
> 	}
> ```

- [ ] **Step 10: 빌드 + 테스트 실행 (사용자)**

Automation 창에서 실행:
- `SkillProject.Manager.Session.*` 3건 → PASS
- `SkillProject.Manager.Loading.*` (기존 회귀) → PASS 유지

- [ ] **Step 11: 스테이징 + 커밋 메시지(안)**

```bash
git add SkillProject/Plugins/SKUICore/Source/SKUICore/Public/SKUIManager.h SkillProject/Plugins/SKUICore/Source/SKUICore/Private/SKUIManager.cpp SkillProject/Source/SkillProject/Manager/SpyUIManager.h SkillProject/Source/SkillProject/Manager/SpyUIManager.cpp SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.h SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.cpp SkillProject/Source/SkillProject/UI/SpyLoadingWidget.cpp SkillProject/Source/SkillProject/Manager/Tests/SpySessionTravelTests.cpp
```

커밋 메시지(안) — 플러그인 변경이 섞이므로 **2개로 쪼갠다**:
1. `[Feature] USKUIManager — OpenUI ZOrder 파라미터 추가` (SKUICore 2파일 + SpyUIManager 2파일)
2. `[Feature] USpyLoadingWidget — 로딩 완료 시 방 목록 진입` (나머지)

---

### Task 9: 에셋 작업 — WBP 생성 + 데이터 등록

> **⛔ 목업 승인 게이트.** 이 Task 는 새 화면 배치를 만든다. `.claude/project.md` "UI 작업 — 목업 승인 게이트" 에 따라 **메인이 목업(HTML/이미지/ASCII)을 제시하고 사용자 승인을 받은 뒤에만** 착수한다. 승인 전 위젯 트리·WBP 에셋을 만지지 않는다.
>
> **⚠ WidgetBlueprint 에 `compile_blueprint()` 를 호출하지 않는다 — 에디터가 데드락된다.** 생성/편집 후 `save_asset(only_if_is_dirty=False)` 만 하고, **컴파일은 사용자가 디자이너에서 1회** 수행한다 (`.claude/rules/ui-workflow.md` §2-3). 착수 전 메모리 `project-mcp-umg-editing` 의 레시피를 읽는다.

**Files:**
- Create (에셋): `Content/Spy/UI/WBP_SessionBrowser.uasset`
- Create (에셋): `Content/Spy/UI/WBP_SessionRow.uasset`
- Modify (에셋): `Content/Spy/Data/` 의 `SpyAssetData` — `SessionBrowser` 엔트리
- Modify (에셋): `Content/Spy/Data/` 의 `DA_SpyLoadingConfig` — `ServerAddress` 를 빈 값으로

**Interfaces:**
- Consumes: Task 6 `USpySessionRowWidget` (BindWidget: `RoomNameText`, `PlayersText`, `PingText`, `JoinButton`), Task 7 `USpySessionBrowserWidget` (BindWidget: `SessionListBox`, `RefreshButton`, `HostButton`, `StatusText` + 프로퍼티 `RowWidgetClass`)
- Produces: 실행 가능한 방 목록 화면

- [x] **Step 1: 목업 제시 + 승인 대기 — 완료**

**목업 승인 완료 (2026-07-28).** 아티팩트: https://claude.ai/code/artifact/d9769442-9eb0-4d2e-939c-b5d6b7f99bf6
(소스: 스크래치패드 `session-browser-mockup.html` — 같은 경로로 재발행하면 URL 이 유지된다.)

승인 과정에서 **사용자가 레이아웃 변경을 지시**했다: 1단 중앙 목록 → **좌측 로고 / 우측 목록 2단**. 변경된 목업으로 재승인받았고, 그 결과가 아래 Step 2·3 의 값이다.

> **다음에 UI 작업을 할 때 주의** — 목업은 승인의 **입력**이지 승인 후 산출물이 아니다. 화면 배치를 글로만 승인받고 나중에 목업을 내는 순서는 게이트를 무력화한다.

- [ ] **Step 2: `WBP_SessionRow` 생성**

기존 WBP 를 복제해 만든 뒤 `USpySessionRowWidget` 로 reparent 한다(빈 WBP 는 파이썬으로 루트를 만들 수 없다). 위젯 배치를 **끝낸 뒤 마지막에** reparent 한다 — BindWidget 이 충족돼야 컴파일 에러가 없다.

**⚠ 루트 패널을 반드시 교체한다.** 복제 원본의 루트는 대개 `CanvasPanel` 인데, 기획서 §4-3-2 는 행 루트가 **`SizeBox`(`WidthOverride = 1032`, `HeightOverride = 80`)** 여야 한다고 확정했다. 루트가 `SizeBox` 가 아니면 폭·높이 검산(pitch 80, 컬럼 합 1032)이 성립하지 않아 목록 전체 레이아웃이 어긋난다. 복제 후 **원본 루트를 제거하고 `SizeBox` 를 새 루트로 세운 뒤** 그 아래에 트리를 짠다.

기획서 §4-3-2 확정 트리:

```
SizeBox_Row (1032 × 80)          ← 루트. 반드시 교체할 것
└ VerticalBox
  ├ SizeBox_Band (높이 72)        ← Auto
  │ └ Border
  │   └ HorizontalBox            ← §4-3-3 의 7슬롯
  └ Spacer (0 × 8)               ← Auto. 이 8px 이 행 간격이다
```

컬럼 폭(좌→우): `24 / 548 / 140 / 120 / 16 / 160 / 24` = **1032**. 전부 `Size = Auto` + `SizeBox` 고정폭이고 `Fill` 을 쓰지 않는다.

> **행 폭이 1144 가 아니라 1032 인 이유** — 사용자 승인으로 화면이 **좌측 로고 / 우측 목록 2단** 구성이 되면서 목록 패널이 `1152 → 1040` 으로 좁아졌다(`1040 − 스크롤바 8 = 1032`). **행 높이 80(밴드 72 + `Spacer` 8) 구조와 나머지 컬럼 폭은 변동 없다** — 바뀐 것은 `RoomNameText` 가 `660 → 548` 하나뿐이다.

필수 자식 위젯 이름(대소문자 정확히): `RoomNameText`(TextBlock), `PlayersText`(TextBlock), `PingText`(TextBlock), `JoinButton`(Button).

`save_asset(only_if_is_dirty=False)` 로 저장하고, `.uasset` 타임스탬프 재조회로 검증한다.

- [ ] **Step 3: `WBP_SessionBrowser` 생성**

같은 방식으로 만들고 `USpySessionBrowserWidget` 로 reparent 한다.

필수 자식 위젯 이름: `SessionListBox`(**`ScrollBox`** — 기획서 §4-1 확정. Task 7 이 `UPanelWidget` 으로 받으므로 BindWidget 은 충족된다), `RefreshButton`(Button), `HostButton`(Button), `StatusText`(TextBlock).

**승인된 2단 레이아웃** (기획서 §4-2, 1920×1080 캔버스):

```
│←120→│  로고 560×360  │←80→│      SessionListBox 1040×520      │←120→│
        x 120, y 300           x 760, y 220
                                        [새로고침]  [방 만들기]      ← 중심 (1160,810) / (1400,810), 각 220×52
                                          StatusText 중심 (1280, 880)
```

- **로고 영역** — `UImage` 정적 배치, x `120` / y `300` / `560 × 360`. **BindWidget 을 추가하지 않는다** — 코드가 만지지 않으므로 C++ delta 0 이다.
  - **에셋 확정: `/Game/Spy/UI/Icons/T_Logo`** (임포트 완료 — 원본 `Logo.png` 1296×832, UI 텍스처 그룹 · `UserInterface2D` 압축 · 밉맵 없음 · sRGB). 기획서가 임시안으로 정했던 `TextBlock` 락업은 **폐기**한다.
  - 비율 `1296/832 = 1.5577` vs 영역 `560/360 = 1.5556` — 차이 0.14% 라 `Image` 브러시를 영역에 그대로 채우면 된다. **컬럼 폭 재작업 없음.**
  - ~~⚠ **원본에 투명 배경이 없다**~~ → **PIE 실측 결과 문제 없음(2026-07-28, M-SB-6 종결).** 알파는 전 픽셀 불투명이고 우하단이 `#0E0E0E`(화면 배경 `#08090B` 대비 RGB 차 최대 6)지만 **사각형 경계가 육안으로 보이지 않는다.** 투명 PNG 교체 불필요 — 현재 `T_Logo` 를 그대로 쓴다.
- 세로 값(패널 top 220 / height 520, 버튼 y 810, StatusText y 880)은 **1단 시절과 동일**하다 — 바뀐 것은 가로 배치뿐이라 "7행째 40px 노출", "목록 하단 ↔ 버튼 상단 44" 검산이 그대로 유효하다.
- 버튼·상태 문구는 화면 중앙이 아니라 **목록 컬럼 중심 x 1280** 에 정렬한다(목록에 딸린 액션이기 때문).

- [ ] **Step 4: `RowWidgetClass` 지정**

`WBP_SessionBrowser` CDO 의 `row_widget_class` 를 `WBP_SessionRow` 의 generated class 로 설정한다. 클래스 참조는 CDO 직속 프로퍼티라 `set_editor_property` 가 통한다.

- [ ] **Step 5: `SpyAssetData` 에 UI 엔트리 등록**

UI 그룹에 이름 `SessionBrowser` → `WBP_SessionBrowser` 경로를 등록한다. **`SessionRow` 는 등록하지 않는다** — Step 4 의 클래스 참조로 로드되므로 UI 매니저 엔트리가 아니다.

등록 후 `get_spy_asset_data` 로 되읽어 검증한다.

- [x] **Step 6: `SpyLoadingConfig.ServerAddress` 를 빈 값으로 — 조치 불필요(이미 빈 값)**

실측 결과 `/Game/Spy/Data/Config/SpyLoadingConfig` 의 `ServerAddress` 는 **이미 `""`** 였다. `networked-loading-transition.md` §9-1 이 `127.0.0.1:7777` 을 지정했으나 에셋에는 반영되지 않은 상태였다 — 즉 **방 목록을 켜는 조건이 이미 충족돼 있다.** 되돌리지 말 것.

> **⚠ 이 에셋은 프로퍼티가 snake_case 가 아니라 C++ 원래 이름으로 접근된다** — `get_editor_property("ServerAddress")` 는 되고 `"server_address"` 는 실패한다. 다른 Config 에셋도 같을 수 있으니 MCP 로 만질 때 주의.
>
> 함께 확인된 현재 값: `ConnectTimeoutSeconds = 15.0` · `MinDisplaySeconds = 2.5` · `AssetPhaseWeight = 0.99` · `GameplayMap = /Game/Spy/Maps/DevMap`.

- [ ] **Step 7: 사용자 컴파일 + 확인**

사용자에게 요청한다:
1. 디자이너에서 `WBP_SessionRow` 컴파일 → 그 다음 `WBP_SessionBrowser` 컴파일 (순서 중요 — Row 가 먼저)
2. 컴파일 에러(BindWidget 누락)가 없는지 확인

- [ ] **Step 8: 스테이징 + 커밋 메시지(안)**

```bash
git add SkillProject/Content/Spy/UI/WBP_SessionBrowser.uasset SkillProject/Content/Spy/UI/WBP_SessionRow.uasset SkillProject/Content/Spy/Data/
```

커밋 메시지(안): `[Feature] WBP_SessionBrowser — 방 목록 위젯 + 에셋 등록`

---

### Task 10: 통합 수동 검증

**Files:**
- 없음 (검증 전용)

**Interfaces:**
- Consumes: Task 1~9 전부

> 이 Task 는 **사용자가 수행**한다. 구현자는 절차를 제시하고 결과를 받아 정리한다.

- [ ] **Step 1: 호스팅 경로 확인**

에디터 Play 설정을 **Number of Players = 2, Net Mode = Play Standalone** 으로 두고 실행한다.

0. **(M-SB-1 — 최우선 확인)** 방 목록이 로딩 화면 **위에** 보이는가. Task 8 Step 1~2 의 ZOrder 수정이 듣지 않으면 목록이 불투명 배경 뒤에 깔려 **아무것도 안 보이거나 로딩 화면만 남는다.** 이 항목이 실패하면 아래를 볼 필요가 없다
1. 두 창 모두 로딩 → 방 목록이 뜨는가
2. 창 A 에서 "방 만들기" → A 가 DevMap 으로 진입하는가
3. 로그에 `# [SKOnlineSessionSubsystem] 방 생성 완료` 와 `# [SpyLoadingSubsystem] 리슨 서버 전환: /Game/Spy/Maps/DevMap?listen` 이 찍히는가

- [ ] **Step 2: 조인 경로 확인**

1. 창 B 에서 "새로고침" → A 의 방이 목록에 뜨는가 (방 이름·인원·핑)

> **인원 표기는 `1 / 4` 여야 한다** (2026-07-30 패키지 실측으로 정정 — 이전에 "0/4 도 정상"으로 적었던 것은 **틀렸다**).
>
> Null 은 `CreateSession` 시 `NumOpenPublicConnections` 를 최대치로 시작하고 **`RegisterPlayers` 호출로 깎는다** — 엔진 주석이 `OnlineSessionInterfaceNull.cpp:202` 에서 `local player will register later` 라고 명시한다(감소는 `:912-914`).
> 그런데 호스트의 `PostLogin`(→ `AGameSession::RegisterPlayer`)은 DevMap 로딩 중에 끝나고, 우리 `CreateSession` 은 `ScheduleHostSessionAfterArrival()` 로 **그 다음 틱**에 돌아 등록 시점을 놓친다. **포트 0 을 고치려고 순서를 뒤집은 부작용이다.**
> → 조치: `HandleCreateSessionComplete` 성공 경로에서 로컬 플레이어를 `RegisterPlayer` 로 직접 등록한다. `FSKSessionInfo::Make` 의 `Current = Max - Open` 계산식은 옳다(입력값이 틀렸을 뿐).
2. 방 행의 "참가" 클릭 → 로그에 `# [SKOnlineSessionSubsystem] 조인 완료 — 접속 문자열:` 과 `# [SpyLoadingSubsystem] 서버 접속 개시:` 가 찍히는가
3. B 가 A 의 월드에 합류해 **둘이 서로 보이고 움직임이 동기화되는가** (이 항목이 이번 기능의 성공 기준)

- [ ] **Step 3: 빈 목록 / 에러 경로 확인**

1. 방 없이 "새로고침" → "방이 없습니다" 가 뜨고 새로고침 버튼이 살아 있는가
2. 버튼 연타 시 에러 문구가 튀지 않는가(Busy 무음 처리)

> **오진 주의 — 호스팅 직후의 짧은 창.** `CreateSession` 성공과 `ServerTravel(?listen)` 이 리슨 소켓을 여는 사이에는 아주 짧게 "검색은 되지만 아무도 듣지 않는 주소"인 구간이 있다. 이 순간에 조인하면 접속 실패가 뜬다. 실사용에서는 사실상 재현되지 않지만, **호스트가 방을 만들자마자 곧바로 조인했을 때 나는 1회성 접속 실패는 로직 버그가 아니다** — 새로고침 후 다시 조인하면 정상이다.

- [ ] **Step 4: 조인 후 접속 실패 복구 확인 (Task 8 Step 9)**

1. A 에서 방을 만들어 인게임 진입 → B 의 목록에서 그 방을 확인
2. **A 를 강제 종료**(호스트 소멸) → B 에서 그 방 행의 "참가" 클릭
3. 접속 실패 후 **재시도 버튼이 아니라 방 목록으로 되돌아오는가**
4. 되돌아온 목록에서 "새로고침" → 사라진 방이 목록에서 빠지는가
5. 그 상태에서 B 가 직접 "방 만들기" 로 호스팅할 수 있는가 (진입 가드가 풀렸는지 확인)

- [ ] **Step 5: 기존 경로 회귀 확인 (D6)**

`DA_SpyLoadingConfig.ServerAddress` 를 `127.0.0.1:7777` 로 되돌리고 실행한다.

1. 방 목록이 **뜨지 않고** 기존 "접속" 버튼이 뜨는가
2. 에디터 "Run Dedicated Server" 로 서버를 띄운 뒤 접속이 되는가
3. 확인 후 `ServerAddress` 를 다시 빈 값으로 되돌린다

- [ ] **Step 6: 결과 정리**

실패 항목이 있으면 어느 Step 에서 무엇이 어긋났는지(로그 포함) 정리해 보고한다. 전부 통과하면 `.claude/.active-sessions.md` 를 갱신하고 최종 커밋 메시지(안)를 제시한다.

---

## 자기 점검 결과 (플랜 작성자)

**스펙 커버리지**

| 스펙 절 | 구현 Task |
|---|---|
| §4-1 플러그인 구조 | Task 1 |
| §4-2 서브시스템 API | Task 4 |
| §4-3 `FSKSessionInfo` | Task 1 |
| §4-4 `USKOnlineSettings` | Task 2 |
| §4-5 게임 모듈 변경 | Task 5, 7, 8, 9 |
| §5 흐름 | Task 5(트래블), 7(브라우저), 8(진입) |
| §5-1 리슨 전환 | Task 5 Step 7 |
| §5-2 UI 계층·입력 모드·명시적 닫기 | Task 7(`CloseForTravel`), Task 8(입력 모드·바 복원) |
| §5-2 렌더 순서 (스펙 작성 후 발견된 블로커) | Task 8 Step 1~2 — `OpenUI` ZOrder 파라미터 추가, 브라우저 200. 검증은 Task 10 Step 1-0 |
| §6 에러 처리 (OSS 단계) | Task 4(가드·세션 정리), Task 7(`MakeStatusMessage`) |
| §6 에러 처리 (접속 단계) | Task 8 Step 9 — 조인 후 접속 실패 시 세션 파괴 + 방 목록 복귀 + 진입 가드 해제 |
| §7-1 자동화 테스트 | Task 1, 2, 3, 5, 7, 8 |
| §7-2 수동 검증 | Task 10 |
| §8 기획 확정값 | Task 2 (`USKOnlineSettings` 한 곳에 모음) |
| §10 Steam 이행 | 구현 없음 — Task 2 가 이음매만 제공 (스펙대로) |

**타입 일관성 확인**: `FSKSessionInfo::Make` 인자 순서(Task 1 정의 → Task 4 호출 → Task 1 테스트), `CanStartOp(Current, Requested)` 인자 순서(Task 3 → Task 4), `EnterGameplay(OverrideAddress)`(Task 5 → Task 7), `HostAndEnter()`(Task 5 → Task 7), BindWidget 이름(Task 6·7 정의 → Task 9 에셋) 전부 일치 확인.

**알려진 위험 (구현 중 확인 필요)**

1. **`Result.Session.SessionSettings.NumPublicConnections`** 는 검색 결과에 광고된 값이다. `bShouldAdvertise` 가 꺼진 백엔드에서는 0 이 올 수 있다 — 그 경우 `FSKSessionInfo::Make` 의 클램프가 `0 / 0` 으로 표시한다(크래시 없음).
2. **PIE 2창에서 OSS Null 세션**은 같은 프로세스가 아니라 별도 인스턴스여야 한다. Play Standalone(별도 프로세스)이 필수이며, "Play In Editor" 단일 프로세스 모드로는 검색이 되지 않는다 — Task 10 Step 1 에 명시했다.
3. **`RetryConnect` 의 override 반영**(Task 5 Step 7 각주)을 빠뜨리면 조인 실패 후 재시도가 엉뚱한 주소로 간다. code-reviewer 가 반드시 확인할 지점.
4. **런타임 `AddChild` 슬롯의 패딩은 행이 통제할 수 없는 유일한 잔여 변수다.** 기획서 §4-3-2 의 여백 내장형 행은 슬롯 *정렬* 에는 비의존이지만, `ScrollBox` 슬롯에 기본 패딩이 붙으면 그만큼 pitch 에 더해진다. **슬롯 패딩 0 을 전제로 검산돼 있다** — 엔진 기본값을 단정하지 않고 Task 10 에서 관측한다: 행이 2개 이상일 때 간격이 8px 로 보이는지. 어긋나도 구조는 성립하고 C++ 변경도 필요 없다(어긋나는 것은 "7행째 40px 노출" 디테일뿐).
5-a. **잔존 세션이 LAN 을 오염시킨다 — 실제 발생, 일부 해소.**
   PIE 에서 **호스트가 에디터 프로세스 안에서 돌면** `CreateSession` 이 에디터의 OSS 인스턴스에 세션을 만들고, **PIE 를 끝내도 그 세션이 남아** LAN 비콘(포트 14001)으로 계속 광고된다(실측: 포트 14001 소유자 = 에디터 PID). 그 결과 **아무도 호스팅하지 않았는데 검색이 1건을 반환**하고, 게다가 그 방의 **포트는 0** 이다 — 에디터는 `GetWorldForOnline` 의 `Cast<UGameEngine>(GEngine)` 이 실패해 월드를 못 찾기 때문(`OnlineSubsystemUtils.cpp:225·245`). 조인하면 20초 타임아웃.
   → **조치: `USKOnlineSessionSubsystem` 의 `Deinitialize`(종료 시 파괴) + `Initialize`(같은 프로세스의 잔존 세션 청소) 배선 추가.** "방 나가기" UI·인게임 이탈 시 파괴는 여전히 범위 밖(아래 #5-b).
   → **검증 시 주의: 에디터를 호스트로 쓰지 마라.** `Standalone Game` 두 개로 테스트한다.
   → **정리 배선의 알려진 한계**: `Initialize` 청소는 GameInstance 단위로 돈다. **단일 프로세스 PIE 로 플레이어 2명**을 띄우면 두 번째 GameInstance 의 `Initialize` 가 **첫 번째의 세션을 지운다.** 이 플랜의 검증은 `Play Standalone`(별도 프로세스)으로 못박혀 있고 단일 프로세스 모드는 애초에 LAN 검색이 성립하지 않으므로 실사용 경로에는 영향이 없다 — 다만 그 조합으로 테스트하면 방이 사라지는 것처럼 보이니 혼동하지 말 것.
5-b. **`HostSession()` 의 destroy→create 동일 프레임 — Null 은 안전, Steam 은 실제 위험** (구현 중 엔진 소스로 좁혀진 결론).
   `HostSession` 은 남은 세션이 있으면 `DestroySession` 후 곧바로 `CreateSession` 을 호출한다. **Null 은 `RemoveNamedSession` 을 동기 실행하고 그 자리에서 완료 델리게이트를 트리거**하므로(`OnlineSessionInterfaceNull.cpp`, `Result != ONLINE_IO_PENDING` 분기) 같은 프레임의 `CreateSession` 시점엔 이미 제거돼 있다. **Steam 은 async task 를 큐잉하고 `ONLINE_IO_PENDING` 을 돌려주므로** 세션이 남은 채 생성이 실행된다.
   → **Task 10(Null/LAN)에서는 재현되지 않을 가능성이 높고, Steam 전환 시 드러난다.** 지금 고치지 않는다. 고칠 경우의 형태는 "destroy 완료 델리게이트를 걸고 그 콜백에서 create 를 잇는 pending-host 상태"이며, `CurrentOp` 상태 기계를 건드리므로 별도 결정이 필요하다.
   진입 경로(세션이 남아 있게 되는 이유): ①호스팅 후 브라우저 복귀 시 소비자가 `DestroyCurrentSession()` 을 부르지 않음(**가장 유력** — "방 나가기" 배선이 이 플랜에 없다) ②조인 성공 후 클라이언트 쪽 `NAME_GameSession` 잔존 ③조인 실패 복구의 `DestroySession` 이 fire-and-forget ④`CreateSession` 실패 경로에 정리 없음.
6. **`ServerTravel`(호스팅) 실패는 감지 경로가 없다 — 미해결, 사용자 결정 대기.**
   리슨 분기는 `StartConnectWatch()` 를 부르지 않으므로 `OnTravelFailure`·`OnNetworkFailure` **구독 자체가 존재하지 않는다.** `ServerTravel` 이 실패하면 에러도 재시도도 없이 **로딩 화면에 영원히 남는다.** 도착 시 `bHostingListenServer` 를 되돌리는 처리는 넣었지만 이 경우엔 도착 자체가 없어 플래그도 걸린 채 남는다(이후 조인이 리슨 분기를 탄다).
   호스트는 **로컬에 이미 프리로드된 맵**을 여는 것이라 실패 확률이 낮아 이번 범위에서 제외했다. 해소하려면 `StartConnectWatch` 를 "실패 구독"과 "진행률 타이머"로 분리해 리슨 경로에서도 실패만 구독해야 한다 — **구조 변경이므로 별도 승인이 필요하다.**
7. **`RetryConnect` 의 override 는 실효적으로 항상 비어 있다 — 나중에 브라우저에 재시도 버튼을 달면 터진다.**
   `HandleConnectFailed` 가 브로드캐스트 **전에** `PendingOverrideAddress.Reset()` 하므로, `RetryConnect` 의 `ResolveTravelAddress` 는 언제나 config `ServerAddress` 로 귀결된다. **지금은 무해하다** — 브라우저 경로는 재시도 버튼을 띄우지 않고(목록 복귀로 대체), config 경로는 override 가 원래 없다.
   → **브라우저 경로에 재시도 버튼을 추가하는 순간 `"접속 주소가 없습니다"` 로 즉시 중단된다.** 그때는 실패한 조인 주소를 별도 필드에 보관하거나 리셋 시점을 브로드캐스트 이후로 옮겨야 한다.
8. **파괴 op 와 자동 검색이 겹치는 경로는 버그가 아니다.** 접속 실패 복구(Task 8 Step 9)에서 `DestroyCurrentSession()` 직후 브라우저가 열리면, `NativeConstruct` 의 자동 `FindSessions()` 가 `Busy` 로 거부돼 **빈 목록 + "이전 방을 정리하는 중입니다" 가 남을 수 있다.** 회복은 새로고침 1클릭(Task 10 Step 4 의 4항이 이미 그 조작을 포함)이며, 지울 세션이 없으면 `EndOp()` 가 동기 실행돼 이 경로는 아예 발생하지 않는다. **검증자가 결함으로 보고하지 않도록 Task 10 Step 4 수행 시 이 문단을 함께 읽을 것.**
