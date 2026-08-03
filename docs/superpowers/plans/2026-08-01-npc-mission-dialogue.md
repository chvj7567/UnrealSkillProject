# NPC 대화 기반 미션 수락+보고 시스템 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.
>
> **⚠ 이 프로젝트 전용 주의사항**: 이 리포는 `.claude/project.md` 표준 흐름(uses_superpowers: true)을 쓴다. 이 plan이 끝나면 generic `subagent-driven-development`/`executing-plans` 로 바로 코드를 짜지 않고, **`gameplay-programmer`(실제 .h/.cpp 구현) → `code-reviewer` → `test-engineer`** 순서로 이어간다 — 기획서·spec 승인은 이미 끝났다(사용자 승인 게이트 통과). 이 plan의 Task는 그 파이프라인이 참조할 "무엇을 어디에" 목록이다.
>
> **개정: 2026-08-01c 전면 재작성.** 이전 판(2026-08-01b)은 "보고"를 미션의 서브상태(`bObjectiveMet`/`ReportCurrentMission()`/`ReadyToReport`)로 다뤘다 — **이 모델은 전부 폐기됐다.** 새 spec(`docs/superpowers/specs/2026-08-01-npc-mission-dialogue-design.md`, 개정 c)은 "보고"를 미션 배열 자체의 독립 항목으로 바꾸고 데이터를 5테이블(`Mission`/`MissionReward`/`MissionCommunication`/`NPC`/`Dialogue`)로 정규화했다. **이미 구현된 2026-08-01b 코드는 아래 "폐기 대상 코드" 절을 반드시 먼저 읽고 되돌린 뒤 새로 짠다.**

**Goal:** NPC에게 근접 상호작용해 대화를 걸면, 그 NPC가 담당하는 Gameplay 미션을 수락/거절할 수 있고(수락해야 진행 이벤트가 카운트됨), 목표를 달성하면 배열의 다음 항목(그 NPC에게 보고하는 Dialogue 미션)이 자동 활성화되어 같은 NPC와 재대화하는 것만으로(카드 없이) 보상 지급 + 다음 NPC 게이트 해제가 일어난다.

**Architecture:** `Missions[]` 12행(짝수=Gameplay, 홀수=Dialogue)을 `USpyMissionComponent`가 `bAccepted` 단일 게이트로 순차 진행시킨다. Dialogue 타입은 배열 진입과 동시에 자동 수락된다. NPC 도메인(`ASpyNPCCharacter` + `ISpyNPCRoot`)은 `USpyNPCConfig` 허브 DataAsset이 묶은 3개 DataTable(`NPC`/`Dialogue`/`MissionCommunication`)을 `BeginPlay`에 1회 스캔해 캐싱하고, 4상태 판정 순수 함수(`ResolveNPCDialogueState`)로 대사 상태를 결정한다. 플레이어 쪽은 `USpyInteractionComponent`가 근접 감지·서버 RPC를 담당하고, 결과는 대화창/미션 카드 위젯(`USpyUserWidget` 파생)으로 표시한다.

**Tech Stack:** Unreal Engine 5.7, C++, GAS(SKGAS), Enhanced Input, `UDataTable`, `USKUIManager`/`USpyUIManager`.

## Global Constraints

- 엔진: Unreal Engine 5.7 / 언어: C++ (CLAUDE.md)
- `.h`/`.cpp` 는 한 줄이라도 손대면 반드시 `gameplay-programmer` 서브에이전트가 작성한다 — 메인/이 plan 실행 세션이 직접 코드를 쓰지 않는다 (`.claude/project.md` 메인 오케스트레이터 규칙)
- 코딩 스타일은 `.claude/rules/cpp-style.md` 전체 준수: `//#` 주석만, `!`/`auto` 금지(예외만 허용), 가드 절 중괄호 없음, `TObjectPtr<>`, 불리언/널 명시 비교(`== false`/`== nullptr`), §8 컴포넌트 탐색 지양, §11 공용 enum은 `Util/DefineEnum.h`, §12 공용 인터페이스는 도메인별 `CommonInterface.*.h`, §13 루트 파사드 + 대응 인터페이스, §14-1 DataTable row struct 정규화 절차, §15 매직 넘버 금지
- 서버 권한: 게임플레이 상태 변경(미션 수락·보고 등)은 서버에서 실행, `HasAuthority()` 체크 후 처리 (`unreal-infra.md` §2)
- 에셋 접근은 `USpyAssetManager` 경유, 하드코딩 `/Game/...` 경로 금지 (`plugin-skassetcore.md`)
- UI 열기/닫기는 `USpyUIManager::OpenSpyUI`/`CloseSpyUI` 경유, 위젯 직접 `CreateWidget`+`AddToViewport` 금지 (`plugin-skuicore.md`)
- 위젯 배치(WBP 편집)는 목업 승인 후 사용자가 디자이너에서 직접 수행 — MCP는 `compile_blueprint()` 호출 금지 (`ui-workflow.md`)
- 커밋: `git commit` 자동 실행 금지. 관련 파일 `git add` 까지만 하고 메시지(안) 제시 (`git-conventions.md`)
- 이 저장소에 전용 recompile/test-run CLI 커맨드가 없다 — 빌드·Automation 테스트 실행은 Unreal Editor(Session Frontend) 또는 Visual Studio에서 사용자가 수행한다
- 참조 spec: `docs/superpowers/specs/2026-08-01-npc-mission-dialogue-design.md` (개정 2026-08-01c, 섹션 번호는 이 문서 기준) / 참조 기획서: `docs/design/npc-mission-dialogue.md` (개정 2026-08-01c, 실제 데이터 값의 단일 진실)

---

## ⚠ 폐기 대상 코드 — Task 착수 전 먼저 확인

2026-08-01b 구현 중 작성된 아래 파일들은 새 모델과 맞지 않는다. **각 Task가 "무엇을 남기고 무엇을 되돌리는지" 명시하므로, 임의로 삭제하지 말고 해당 Task 설명을 따른다.**

| 파일 | 현재(2026-08-01b) 상태 | 처리 |
|---|---|---|
| `Data/SpyMissionConfig.h` | `FSpyMissionEntry`에 `Description` 필드만 추가된 상태(`ExperienceReward` 그대로 있음) | **`Description`은 유지.** `ExperienceReward` 삭제 + `MissionType` 추가 + `FSpyMissionRewardRow`/`MissionRewardTable`/`GetMissionReward()` 신규는 Task 1 |
| `Data/SpyNPCDialogueRow.h`/`.cpp` | 5줄 단일 NPC 로우(`FSpyNPCDialogueRow`: `NPCDisplayName`+`LockedLine`/`OfferLine`/`InProgressLine`/`ReportLine`/`CompletedLine`) + `SpyNPCDialogue::ResolveDialogueState`(4-param) | **파일명은 재사용, 내용 전면 교체** — Task 2가 `FSpyNPCRow`/`FSpyDialogueRow`/`ESpyMissionCommRole`/`FSpyMissionCommunicationRow`/`USpyNPCConfig`로 대체 |
| `System/Tests/SpyNPCDialogueTests.cpp` | 구 5상태(`Locked`/`Offer`/`InProgress`/`ReadyToReport`/`Completed`) 테스트 6개 | **전면 교체** — Task 2가 새 4상태 테스트로 대체 |
| `System/SpyMissionComponent.h`/`.cpp` | `bObjectiveMet` 필드, `ReportCurrentMission()`, `IsCurrentObjectiveMet()`, `ProcessProgress`가 목표달성/보고를 분리 | **`bObjectiveMet`/`ReportCurrentMission()`/`IsCurrentObjectiveMet()` 전부 삭제.** `ProcessProgress`는 즉시 전진+보상으로 복귀(Task 3). `AcceptCurrentMission()`의 레벨 재평가 로직(§5-5)은 **그대로 유지** — 손대지 않는다 |
| `Character/CommonInterface.Character.h` | 커밋 이력상 수정됨(정확한 diff는 Task 6에서 gameplay-programmer가 재확인) | Task 6에서 `GetInteractionHost()` 시그니처만 확인, 이미 있으면 유지 |
| `ManagerComponent/CommonInterface.Manager.h` | 커밋 이력상 수정됨 | Task 6에서 `ISpyInteractionHost` 존재 여부 확인 후 없으면 추가 |
| `NPC/CommonInterface.NPC.h`, `NPC/SpyNPCCharacter.h` | 헤더만 존재, `.cpp` 없음 — Task 4(구 모델) 착수 중 중단됨 | **전면 교체** — Task 4가 새 4상태 모델로 다시 작성 |

---

## 파일 구조 개요

```
Data/
  SpyMissionConfig.h|.cpp        # 수정 — MissionType 추가, ExperienceReward 삭제, MissionReward 관계 테이블
  SpyNPCDialogueRow.h|.cpp       # 전면 교체 — NPC/Dialogue/MissionCommunication row struct + USpyNPCConfig
NPC/
  CommonInterface.NPC.h          # 전면 교체 — ISpyNPCRoot, FSpyNPCDialogueResult
  SpyNPCCharacter.h|.cpp         # 전면 교체 — 도메인 루트 액터
ManagerComponent/
  CommonInterface.Manager.h      # 수정(또는 확인) — ISpyInteractionHost
  SpyInteractionComponent.h|.cpp # 신규 — 플레이어 측 상호작용 컴포넌트
Character/
  CommonInterface.Character.h    # 수정(또는 확인) — ISpyCharacterRoot::GetInteractionHost()
  SpyCharacter.h|.cpp            # 수정 — InteractionComponent 소유 + 접근자 구현
System/
  SpyMissionComponent.h|.cpp     # 수정 — bObjectiveMet 계열 삭제, bAccepted 게이트 복귀 + 레이스 수정(spec §5-2-1)
  Tests/SpyNPCDialogueTests.cpp  # 전면 교체 — 4상태 판정 + Dialogue 텍스트 합성 + GetMissionReward 테스트
UI/
  SpyDialogueWidget.h|.cpp       # 신규
  SpyMissionOfferWidget.h|.cpp   # 신규 — Offer 전용(보상 텍스트 없음)
Util/
  DefineEnum.h                   # 수정 — ESpyMissionType, ESpyNPCDialogueState(4상태), ESpyUIType 확장
  SpyGameplayTags.h|.cpp         # 수정 — Event_Mission_Report, Input_Native_Interact
Input/
  SpyInputComponent.h|.cpp       # 수정 — Interact 바인딩
```

---

### Task 1: `Mission` 엔티티 — `MissionType` 추가 + `ExperienceReward` 를 `MissionReward` 관계 테이블로 분리

**Files:**
- Modify: `SkillProject/Source/SkillProject/Util/DefineEnum.h`
- Modify: `SkillProject/Source/SkillProject/Data/SpyMissionConfig.h`
- Modify: `SkillProject/Source/SkillProject/Data/SpyMissionConfig.cpp`

**Interfaces:**
- Consumes: 없음
- Produces:
  - `enum class ESpyMissionType : uint8 { Gameplay, Dialogue }` (`Util/DefineEnum.h`) — Task 3(`ProcessProgress`의 자동 수락 분기), Task 4(NPC가 카드 표시 여부 판단)가 소비
  - `FSpyMissionEntry::MissionType` — 위와 동일
  - `USTRUCT() struct FSpyMissionRewardRow : public FTableRowBase { int32 MissionId; float ExperienceReward; }`
  - `float USpyMissionConfig::GetMissionReward(int32 InMissionId) const` — Task 3의 `GrantReward()`가 호출

- [ ] **Step 1: `ESpyMissionType` 추가**

`Util/DefineEnum.h`에 기존 enum들 옆에 추가 (§11 — 공용 enum 단일 파일):

```cpp
//# 미션 1개의 수락 방식을 가른다. Gameplay는 NPC Offer 카드로 수동 수락,
//# Dialogue는 배열 진입과 동시에 자동 수락된다 (docs/superpowers/specs/2026-08-01-npc-mission-dialogue-design.md §4-2)
UENUM(BlueprintType)
enum class ESpyMissionType : uint8
{
	Gameplay,
	Dialogue,
};
```

- [ ] **Step 2: `FSpyMissionEntry` 수정 — `MissionType` 추가, `ExperienceReward` 삭제**

`Data/SpyMissionConfig.h` 의 `FSpyMissionEntry`:

```cpp
//# 미션 1개의 정의
USTRUCT(BlueprintType)
struct FSpyMissionEntry
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	ESpyMissionType MissionType = ESpyMissionType::Gameplay;

	//# 이 미션이 반응할 이벤트 태그. Dialogue 타입은 전부 공용 Event_Mission_Report 를 쓴다
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	FGameplayTag MatchTag;

	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	ESpyMissionMode Mode = ESpyMissionMode::Accumulate;

	UPROPERTY(EditDefaultsOnly, Category = "Mission", meta = (ClampMin = "1"))
	int32 TargetCount = 1;

	//# HUD 상시 표시 이름. Dialogue 타입은 이 값 자체가 "시스템 메시지"다
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	FText DisplayName;

	//# 수락 카드 서술문. Gameplay 타입만 사용 — Dialogue 타입은 카드가 없으므로 빈 문자열로 둔다
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	FText Description;
};
```

**`ExperienceReward` 필드(기존에 있던 `float ExperienceReward = 0.f;`)를 삭제한다.**

- [ ] **Step 3: `FSpyMissionRewardRow` 추가 + `USpyMissionConfig`에 테이블 참조/접근자 추가**

`Data/SpyMissionConfig.h` — `FSpyMissionEntry` 아래, `FSpyMissionProgressResult` 위에 추가:

```cpp
//# Mission 의 선택적 관계(§14-1) — Dialogue 타입 미션에만 행이 존재한다.
//# 명명 규칙(§14-1-5): Mission_Reward
USTRUCT(BlueprintType)
struct FSpyMissionRewardRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	int32 MissionId = 0;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
	float ExperienceReward = 0.f;
};
```

`USpyMissionConfig` 클래스에 필드+접근자 추가:

```cpp
public:
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	TArray<FSpyMissionEntry> Missions;

	//# Dialogue 타입 미션의 보상 관계 테이블. RowStruct = FSpyMissionRewardRow
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	TObjectPtr<UDataTable> MissionRewardTable;

public:
	// ... 기존 GetMissionCount/IsValidMissionIndex/GetMission/ResolveMissionProgress 그대로 ...

	//# MissionId 로 보상을 조회한다. 행이 없으면(Gameplay 타입) 0.f — sentinel 이 아니라
	//# "관계 없음"의 정상적인 부재 결과다 (spec §4-3)
	UFUNCTION(BlueprintPure, Category = "Mission")
	float GetMissionReward(int32 InMissionId) const;
```

- [ ] **Step 4: `GetMissionReward()` 구현**

`Data/SpyMissionConfig.cpp` — 기존 함수들 아래에 추가:

```cpp
float USpyMissionConfig::GetMissionReward(int32 InMissionId) const
{
	if (MissionRewardTable == nullptr)
		return 0.f;

	TArray<FSpyMissionRewardRow*> Rows;
	MissionRewardTable->GetAllRows<FSpyMissionRewardRow>(TEXT("USpyMissionConfig::GetMissionReward"), Rows);

	for (const FSpyMissionRewardRow* Row : Rows)
	{
		if (Row != nullptr && Row->MissionId == InMissionId)
			return Row->ExperienceReward;
	}

	return 0.f;
}
```

**`ResolveMissionProgress()`는 손대지 않는다** — spec §2-3이 확인했듯 `MissionType`을 몰라도 완료 판정이 성립한다.

- [ ] **Step 5: 컴파일 확인**

에디터 Hot Reload 또는 VS 빌드. `FSpyMissionEntry.ExperienceReward`를 참조하던 코드가 있으면(Task 3에서 `GrantReward()`가 이걸 참조했었다) 이 시점엔 아직 `SpyMissionComponent.cpp`를 안 고쳤으므로 컴파일 에러가 난다 — **정상이다.** Task 3에서 해소된다.

- [ ] **Step 6: Stage**

```bash
git add SkillProject/Source/SkillProject/Util/DefineEnum.h SkillProject/Source/SkillProject/Data/SpyMissionConfig.h SkillProject/Source/SkillProject/Data/SpyMissionConfig.cpp
```
메시지(안): `[Feature] FSpyMissionEntry — MissionType 추가 + ExperienceReward를 MissionReward 관계 테이블로 분리`

---

### Task 2: NPC 대화 데이터 5테이블 중 4개(NPC/Dialogue/MissionCommunication) + 판정 순수 함수 (TDD)

**Files:**
- Modify: `SkillProject/Source/SkillProject/Util/DefineEnum.h`
- Rewrite: `SkillProject/Source/SkillProject/Data/SpyNPCDialogueRow.h` (기존 파일 — 전면 교체)
- Rewrite: `SkillProject/Source/SkillProject/Data/SpyNPCDialogueRow.cpp` (기존 파일 — 전면 교체)
- Rewrite: `SkillProject/Source/SkillProject/System/Tests/SpyNPCDialogueTests.cpp` (기존 파일 — 전면 교체)

**Interfaces:**
- Consumes: 없음 (순수 데이터/로직)
- Produces:
  - `enum class ESpyNPCDialogueState : uint8 { Default, Offer, InProgress, Report }` (`Util/DefineEnum.h`)
  - `ESpyNPCDialogueState ResolveNPCDialogueState(int32 CurrentMissionId, bool bAccepted, int32 OfferMissionId, int32 ReportMissionId)` — Task 4의 `ASpyNPCCharacter::RequestInteract`가 호출
  - `USTRUCT() struct FSpyNPCRow : public FTableRowBase { FText NPCDisplayName; int32 DefaultDialogueId; }`
  - `USTRUCT() struct FSpyDialogueRow : public FTableRowBase { int32 DialogueId; int32 DialogueIndex; FText Text; }`
  - `FText ResolveDialogueText(const UDataTable* InDialogueTable, int32 InDialogueId)` — Task 4가 소비
  - `enum class ESpyMissionCommRole : uint8 { Offer, Report }`
  - `USTRUCT() struct FSpyMissionCommunicationRow : public FTableRowBase { int32 MissionId; int32 NPCId; ESpyMissionCommRole Role; int32 OfferDialogueId; int32 InProgressDialogueId; int32 ReportDialogueId; }`
  - `UCLASS() class USpyNPCConfig : public UDataAsset { TObjectPtr<UDataTable> NPCTable; TObjectPtr<UDataTable> DialogueTable; TObjectPtr<UDataTable> MissionCommunicationTable; }` — Task 4의 `ASpyNPCCharacter`가 `EditDefaultsOnly`로 참조

- [ ] **Step 1: `ESpyNPCDialogueState` 추가**

`Util/DefineEnum.h`:

```cpp
//# NPC 상호작용 대사 상태 4종. Locked/Completed 구분을 두지 않는다 —
//# 둘 다 "현재 미션이 이 NPC와 무관함"으로 통합된다 (spec §6)
UENUM()
enum class ESpyNPCDialogueState : uint8
{
	Default,
	Offer,
	InProgress,
	Report,
};
```

- [ ] **Step 2: 실패하는 테스트 작성**

`System/Tests/SpyNPCDialogueTests.cpp` 전면 교체:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Data/SpyNPCDialogueRow.h"
#include "Engine/DataTable.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNPCDialogueOfferTest,
	"SkillProject.System.NPCDialogue.Offer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNPCDialogueOfferTest::RunTest(const FString& Parameters)
{
	//# 현재 미션이 이 NPC의 Offer 미션, 미수락
	const ESpyNPCDialogueState State = ResolveNPCDialogueState(0, false, 0, 1);

	TestTrue(TEXT("Offer when current mission is this NPC's offer mission and not accepted"), State == ESpyNPCDialogueState::Offer);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNPCDialogueInProgressTest,
	"SkillProject.System.NPCDialogue.InProgress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNPCDialogueInProgressTest::RunTest(const FString& Parameters)
{
	//# 현재 미션이 이 NPC의 Offer 미션, 수락됨
	const ESpyNPCDialogueState State = ResolveNPCDialogueState(0, true, 0, 1);

	TestTrue(TEXT("InProgress when accepted"), State == ESpyNPCDialogueState::InProgress);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNPCDialogueReportTest,
	"SkillProject.System.NPCDialogue.Report",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNPCDialogueReportTest::RunTest(const FString& Parameters)
{
	//# 현재 미션이 이 NPC의 Report 미션
	const ESpyNPCDialogueState State = ResolveNPCDialogueState(1, true, 0, 1);

	TestTrue(TEXT("Report when current mission is this NPC's report mission"), State == ESpyNPCDialogueState::Report);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNPCDialogueDefaultTest,
	"SkillProject.System.NPCDialogue.Default",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNPCDialogueDefaultTest::RunTest(const FString& Parameters)
{
	//# 현재 미션이 이 NPC의 Offer/Report 둘 다 아님 — 아직 차례 아님과 이미 끝남 양쪽 모두 여기로 온다
	TestTrue(TEXT("Default before this NPC's turn"), ResolveNPCDialogueState(2, false, 4, 5) == ESpyNPCDialogueState::Default);
	TestTrue(TEXT("Default after this NPC's turn"), ResolveNPCDialogueState(6, false, 0, 1) == ESpyNPCDialogueState::Default);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyDialogueTextResolveSingleTest,
	"SkillProject.System.NPCDialogue.DialogueText.Single",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyDialogueTextResolveSingleTest::RunTest(const FString& Parameters)
{
	UDataTable* Table = NewObject<UDataTable>();
	Table->RowStruct = FSpyDialogueRow::StaticStruct();

	FSpyDialogueRow Row;
	Row.DialogueId = 3;
	Row.DialogueIndex = 0;
	Row.Text = FText::FromString(TEXT("한 줄"));
	Table->AddRow(TEXT("Row0"), Row);

	const FText Result = ResolveDialogueText(Table, 3);

	TestTrue(TEXT("Single-line group returns that line"), Result.EqualTo(FText::FromString(TEXT("한 줄"))));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyDialogueTextResolveMultiTest,
	"SkillProject.System.NPCDialogue.DialogueText.Multi",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyDialogueTextResolveMultiTest::RunTest(const FString& Parameters)
{
	UDataTable* Table = NewObject<UDataTable>();
	Table->RowStruct = FSpyDialogueRow::StaticStruct();

	FSpyDialogueRow Row1;
	Row1.DialogueId = 7;
	Row1.DialogueIndex = 1;
	Row1.Text = FText::FromString(TEXT("두번째"));
	Table->AddRow(TEXT("Row1"), Row1);

	FSpyDialogueRow Row0;
	Row0.DialogueId = 7;
	Row0.DialogueIndex = 0;
	Row0.Text = FText::FromString(TEXT("첫번째"));
	Table->AddRow(TEXT("Row0"), Row0);

	const FText Result = ResolveDialogueText(Table, 7);

	//# DialogueIndex 오름차순으로 이어붙여야 한다 — 로우 추가 순서와 무관하게
	TestTrue(TEXT("Multi-line group joins in ascending DialogueIndex order"), Result.ToString().Contains(TEXT("첫번째")) && Result.ToString().Find(TEXT("첫번째")) < Result.ToString().Find(TEXT("두번째")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyDialogueTextResolveMissingTest,
	"SkillProject.System.NPCDialogue.DialogueText.Missing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyDialogueTextResolveMissingTest::RunTest(const FString& Parameters)
{
	UDataTable* Table = NewObject<UDataTable>();
	Table->RowStruct = FSpyDialogueRow::StaticStruct();

	const FText Result = ResolveDialogueText(Table, 99);

	TestTrue(TEXT("Missing DialogueId returns empty text"), Result.IsEmpty());

	return true;
}

#endif //# WITH_DEV_AUTOMATION_TESTS
```

- [ ] **Step 3: 테스트가 실패하는지 확인**

`SpyNPCDialogueRow.h`가 아직 새 심볼(`ResolveNPCDialogueState`/`FSpyDialogueRow`/`ResolveDialogueText`)을 갖지 않으므로 컴파일이 실패한다. 에디터/VS 빌드로 확인.

- [ ] **Step 4: 최소 구현 작성**

`Data/SpyNPCDialogueRow.h` 전면 교체:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "Util/DefineEnum.h"

#include "SpyNPCDialogueRow.generated.h"

//# NPC 1명 — 핵심 엔티티, 관계 없음 (DefaultDialogueId 는 필수 1:1 관계라 관계 테이블 없이 직접 참조, §14-1-3)
USTRUCT(BlueprintType)
struct FSpyNPCRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	//# MissionCommunication.NPCId 매칭 키 — 다른 관계 테이블과 동일하게 "전체 스캔 + 필드 비교"로 조회한다
	UPROPERTY(EditAnywhere)
	int32 NPCId = 0;

	UPROPERTY(EditAnywhere)
	FText NPCDisplayName;

	UPROPERTY(EditAnywhere)
	int32 DefaultDialogueId = 0;
};

//# 대사 한 줄(또는 여러 줄로 이어지는 한 그룹) — 핵심 엔티티, 복합 키(§14-1-4)
USTRUCT(BlueprintType)
struct FSpyDialogueRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	int32 DialogueId = 0;

	UPROPERTY(EditAnywhere)
	int32 DialogueIndex = 0;

	UPROPERTY(EditAnywhere)
	FText Text;
};

//# DialogueId 로 그룹을 모아 DialogueIndex 오름차순으로 이어붙인다. 부수효과 없음 — 테스트 대상.
//# 이번 범위 데이터는 전부 DialogueIndex = 0 한 줄뿐이라 사실상 1:1 조회와 동일하게 동작한다
SKILLPROJECT_API FText ResolveDialogueText(const UDataTable* InDialogueTable, int32 InDialogueId);

//# MissionCommunication 의 판별자 — Offer 행/Report 행에서 사용하는 필드가 다르다
UENUM(BlueprintType)
enum class ESpyMissionCommRole : uint8
{
	Offer,
	Report,
};

//# Mission 의 필수 관계 — 모든 미션 행이 정확히 1개씩 갖는다. NPC/Dialogue 로의 FK.
//# 명명 규칙(§14-1-5): Mission_Communication
USTRUCT(BlueprintType)
struct FSpyMissionCommunicationRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	int32 MissionId = 0;

	UPROPERTY(EditAnywhere)
	int32 NPCId = 0;

	UPROPERTY(EditAnywhere)
	ESpyMissionCommRole Role = ESpyMissionCommRole::Offer;

	//# Role == Offer 일 때만 사용
	UPROPERTY(EditAnywhere)
	int32 OfferDialogueId = 0;

	//# Role == Offer 일 때만 사용
	UPROPERTY(EditAnywhere)
	int32 InProgressDialogueId = 0;

	//# Role == Report 일 때만 사용
	UPROPERTY(EditAnywhere)
	int32 ReportDialogueId = 0;
};

//# NPC 도메인의 DataTable 3개를 묶는 허브. NPC 블루프린트 6종은 이것 하나만 참조한다
UCLASS()
class SKILLPROJECT_API USpyNPCConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "NPC")
	TObjectPtr<UDataTable> NPCTable; //# RowStruct = FSpyNPCRow

	UPROPERTY(EditDefaultsOnly, Category = "NPC")
	TObjectPtr<UDataTable> DialogueTable; //# RowStruct = FSpyDialogueRow

	UPROPERTY(EditDefaultsOnly, Category = "NPC")
	TObjectPtr<UDataTable> MissionCommunicationTable; //# RowStruct = FSpyMissionCommunicationRow
};

//# 부수효과 없음 — 4상태 판정. 이 NPC의 Offer/Report 대상 MissionId 를 미리 알고 있다고 가정한다
//# (ASpyNPCCharacter가 BeginPlay에 MissionCommunicationTable 스캔으로 캐싱, Task 4)
SKILLPROJECT_API ESpyNPCDialogueState ResolveNPCDialogueState(int32 CurrentMissionId, bool bAccepted, int32 OfferMissionId, int32 ReportMissionId);
```

`Data/SpyNPCDialogueRow.cpp` 전면 교체:

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/SpyNPCDialogueRow.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyNPCDialogueRow)

FText ResolveDialogueText(const UDataTable* InDialogueTable, int32 InDialogueId)
{
	if (InDialogueTable == nullptr)
		return FText::GetEmpty();

	TArray<FSpyDialogueRow*> AllRows;
	InDialogueTable->GetAllRows<FSpyDialogueRow>(TEXT("ResolveDialogueText"), AllRows);

	TArray<const FSpyDialogueRow*> Matches;
	for (const FSpyDialogueRow* Row : AllRows)
	{
		if (Row != nullptr && Row->DialogueId == InDialogueId)
			Matches.Add(Row);
	}

	if (Matches.Num() == 0)
		return FText::GetEmpty();

	Matches.Sort([](const FSpyDialogueRow& A, const FSpyDialogueRow& B) { return A.DialogueIndex < B.DialogueIndex; });

	FString Joined;
	for (int32 Index = 0; Index < Matches.Num(); ++Index)
	{
		if (Index > 0)
			Joined += TEXT(" ");

		Joined += Matches[Index]->Text.ToString();
	}

	return FText::FromString(Joined);
}

ESpyNPCDialogueState ResolveNPCDialogueState(int32 CurrentMissionId, bool bAccepted, int32 OfferMissionId, int32 ReportMissionId)
{
	if (CurrentMissionId == OfferMissionId)
		return (bAccepted ? ESpyNPCDialogueState::InProgress : ESpyNPCDialogueState::Offer);

	if (CurrentMissionId == ReportMissionId)
		return ESpyNPCDialogueState::Report;

	return ESpyNPCDialogueState::Default;
}
```

**주의**: `Matches.Sort`의 람다는 `const FSpyDialogueRow&` 참조라 §6(`auto` 금지)에 저촉되지 않는다 — 명시 타입 그대로다.

- [ ] **Step 5: 테스트 통과 확인**

에디터 Session Frontend → Automation → `SkillProject.System.NPCDialogue.*` 필터로 7개 테스트(`Offer`/`InProgress`/`Report`/`Default`/`DialogueText.Single`/`DialogueText.Multi`/`DialogueText.Missing`) 전부 통과 확인.

- [ ] **Step 6: Stage**

```bash
git add SkillProject/Source/SkillProject/Util/DefineEnum.h SkillProject/Source/SkillProject/Data/SpyNPCDialogueRow.h SkillProject/Source/SkillProject/Data/SpyNPCDialogueRow.cpp SkillProject/Source/SkillProject/System/Tests/SpyNPCDialogueTests.cpp
```
메시지(안): `[Feature] SpyNPCDialogueRow — NPC/Dialogue/MissionCommunication 5테이블 스키마 + 판정 함수 (2026-08-01b 모델 대체)`

---

### Task 3: `USpyMissionComponent` — 2026-08-01b 되돌리기 + `MissionType` 기반 자동 수락 + 레이스 수정

**⚠ 2026-08-01c 개정**: `bObjectiveMet`/`ReportCurrentMission()`/`IsCurrentObjectiveMet()`를 전부 삭제한다. `ProcessProgress`는 완료 즉시 보상+전진(2026-08-01 이전 동작)으로 되돌리되, 새로 진입한 미션이 `Dialogue` 타입이면 자동 수락한다. **`AddProgress`의 `PendingEvents` 드레인 루프에 반드시 `bAccepted` 재검증을 추가한다(spec §5-2-1) — 이건 새 기능이 아니라 design-reviewer가 실측 코드로 확인한 레이스 컨디션 수정이다.**

**Files:**
- Modify: `SkillProject/Source/SkillProject/System/SpyMissionComponent.h`
- Modify: `SkillProject/Source/SkillProject/System/SpyMissionComponent.cpp`

**Interfaces:**
- Consumes: `USpyMissionConfig::GetMissionReward(int32)`(Task 1), `ESpyMissionType`(Task 1), `SpyGameplayTags::Event_Mission_Level`(기존), `USpyCharacterAttributeSet::GetLevelAttribute()`(기존)
- Produces:
  - `bool USpyMissionComponent::AcceptCurrentMission()` — Task 6의 `Server_AcceptCurrentMission`이 호출 (시그니처 불변)
  - `bool USpyMissionComponent::IsCurrentAccepted() const` — Task 4의 `RequestInteract`가 호출 (시그니처 불변)
  - `const FSpyMissionEntry* USpyMissionComponent::GetMissionEntry(int32 InIndex) const` — Task 4가 카드 텍스트 조회에 호출 (시그니처 불변, 삭제하지 않음)

**⚠ 이 태스크는 Automation 테스트 대상이 아니다** (`MissionConfig`가 `protected`, `HasAuthority()`가 액터 컨텍스트 필요 — 기존과 동일한 이유). 검증은 코드 리뷰 + PIE다.

- [ ] **Step 1: `FSpyMissionState`에서 `bObjectiveMet` 삭제**

`SpyMissionComponent.h`:

```cpp
//# 복제 단위
USTRUCT()
struct FSpyMissionState
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 MissionIndex = 0;

	UPROPERTY()
	int32 Count = 0;

	//# 현재 미션을 수락했는가. false면 AddProgress가 진행 신호를 전부 무시한다.
	//# Dialogue 타입 미션은 인덱스 진입과 동시에 자동으로 true가 된다 (ProcessProgress)
	UPROPERTY()
	bool bAccepted = false;
};
```

- [ ] **Step 2: 헤더에서 `ReportCurrentMission()`/`IsCurrentObjectiveMet()` 삭제**

`SpyMissionComponent.h`에서 다음 선언을 제거한다:

```cpp
	//# 삭제 대상
	bool ReportCurrentMission();

	UFUNCTION(BlueprintPure)
	bool IsCurrentObjectiveMet() const { return MissionState.bObjectiveMet; }
```

`AcceptCurrentMission()`/`IsCurrentAccepted()`/`GetMissionEntry()` 선언은 그대로 둔다(변경 없음).

- [ ] **Step 3: `AddProgress()` 재작성 — 게이트 단순화 + 드레인 레이스 수정**

`SpyMissionComponent.cpp`의 `AddProgress()` 전체를 아래로 교체한다:

```cpp
void USpyMissionComponent::AddProgress(FGameplayTag InEventTag, int32 InAmount)
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr || Owner->HasAuthority() == false)
		return;

	if (MissionConfig == nullptr)
	{
		if (bWarnedMissingConfig == false)
		{
			bWarnedMissingConfig = true;

			UE_LOG(LogTemp, Warning, TEXT("# [SpyMissionComponent] MissionConfig가 지정되지 않아 미션이 진행되지 않습니다: %s"), *GetNameSafe(Owner));
		}

		return;
	}

	//# 처리 중에 들어온 이벤트는 버리지 않고 큐에 쌓는다.
	//# (보상 XP → 레벨업 → 레벨 신호 → AddProgress 경로가 실재한다)
	if (bProcessingProgress)
	{
		FSpyMissionPendingEvent Pending;
		Pending.EventTag = InEventTag;
		Pending.Amount = InAmount;
		PendingEvents.Add(Pending);

		return;
	}

	TGuardValue<bool> ReentryGuard(bProcessingProgress, true);

	//# NPC 대화로 수락하기 전에는 어떤 진행 신호도 반영하지 않는다.
	if (MissionState.bAccepted)
		ProcessProgress(InEventTag, InAmount);

	//# 드레인도 매 반복 "지금" 상태로 재검증한다 — 큐잉 당시엔 수락 상태였어도
	//# ProcessProgress가 그 사이 미션을 전진시켜 bAccepted가 false로 바뀌었을 수 있다
	//# (spec §5-2-1 — 보상 GE가 유발한 재진입 레벨업이 아직 미수락인 다음 미션을 완료시키는 경합 수정)
	while (PendingEvents.Num() > 0)
	{
		const FSpyMissionPendingEvent Next = PendingEvents[0];
		PendingEvents.RemoveAt(0);

		if (MissionState.bAccepted)
			ProcessProgress(Next.EventTag, Next.Amount);
	}
}
```

- [ ] **Step 4: `ProcessProgress()` — 즉시 전진+보상 복귀 + 자동 수락**

`SpyMissionComponent.cpp`의 `ProcessProgress()` 전체를 아래로 교체한다:

```cpp
void USpyMissionComponent::ProcessProgress(FGameplayTag InEventTag, int32 InAmount)
{
	if (MissionConfig == nullptr)
		return;

	const FSpyMissionProgressResult Result = MissionConfig->ResolveMissionProgress(
		MissionState.MissionIndex, MissionState.Count, InEventTag, InAmount);

	const bool bChanged = (Result.MissionIndex != MissionState.MissionIndex) || (Result.Count != MissionState.Count);

	if (Result.bCompletedNow)
	{
		const int32 CompletedIndex = MissionState.MissionIndex;

		GrantReward(CompletedIndex); //# Gameplay 타입은 MissionReward 행이 없어 조용히 no-op
		OnMissionCompleted.Broadcast(this, CompletedIndex);
	}

	if (bChanged == false)
		return;

	MissionState.MissionIndex = Result.MissionIndex;
	MissionState.Count = Result.Count;

	//# 새로 진입한 미션이 Dialogue 타입이면 자동 수락 — NPC Offer 절차가 없다
	const FSpyMissionEntry* NewEntry = GetMissionEntry(MissionState.MissionIndex);
	MissionState.bAccepted = (NewEntry != nullptr && NewEntry->MissionType == ESpyMissionType::Dialogue);

	OnMissionProgressChanged.Broadcast(this, MissionState.MissionIndex, MissionState.Count, GetTargetCount());

	if (IsAllCompleted())
		OnAllMissionsCompleted.Broadcast(this);
}
```

- [ ] **Step 5: `ReportCurrentMission()` 함수 정의 삭제**

`SpyMissionComponent.cpp`에서 `ReportCurrentMission()` 함수 전체(2026-08-01b가 추가한 것)를 삭제한다.

- [ ] **Step 6: `AcceptCurrentMission()` — 변경 없음, 그대로 유지**

기존 구현을 그대로 둔다(레벨 재평가 포함) — 이 함수는 2026-08-01b에서도 바뀌지 않았고 이번에도 바뀌지 않는다:

```cpp
bool USpyMissionComponent::AcceptCurrentMission()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr || Owner->HasAuthority() == false)
		return false;

	if (IsAllCompleted())
		return false;

	if (MissionState.bAccepted)
		return true; //# 멱등

	MissionState.bAccepted = true;

	const FSpyMissionEntry* CurrentEntry = GetMissionEntry(MissionState.MissionIndex);
	if (CurrentEntry != nullptr && CurrentEntry->MatchTag == SpyGameplayTags::Event_Mission_Level && AbilitySystemComponent != nullptr)
	{
		const float CurrentLevel = AbilitySystemComponent->GetNumericAttribute(USpyCharacterAttributeSet::GetLevelAttribute());
		AddProgress(SpyGameplayTags::Event_Mission_Level, FMath::RoundToInt(CurrentLevel));
	}

	return true;
}
```

- [ ] **Step 7: `GrantReward()` — `MissionRewardTable` 조회로 전환 + Dialogue 타입 보상 누락 경고**

`SpyMissionComponent.cpp`의 `GrantReward()` 전체를 아래로 교체한다:

```cpp
void USpyMissionComponent::GrantReward(int32 InCompletedIndex)
{
	if (MissionConfig == nullptr)
		return;

	if (AbilitySystemComponent == nullptr)
	{
		if (bWarnedMissingAbilitySystem == false)
		{
			bWarnedMissingAbilitySystem = true;

			UE_LOG(LogTemp, Warning, TEXT("# [SpyMissionComponent] ASC가 연결되지 않아 미션 보상을 지급하지 못했습니다 (InitializeByAbilitySystem 확인): %s"), *GetNameSafe(GetOwner()));
		}

		return;
	}

	const FSpyMissionEntry* Entry = MissionConfig->GetMission(InCompletedIndex);
	if (Entry == nullptr)
		return;

	const float Reward = MissionConfig->GetMissionReward(InCompletedIndex);

	if (Reward <= 0.f)
	{
		//# Gameplay 타입은 보상이 없는 게 정상이다. Dialogue 타입인데 0이면
		//# MissionReward 행을 빠뜨린 에디터 데이터 실수일 수밖에 없다 — 경고로 구분한다
		if (Entry->MissionType == ESpyMissionType::Dialogue)
		{
			UE_LOG(LogTemp, Warning, TEXT("# [SpyMissionComponent] Dialogue 미션 %d 의 MissionReward 행이 없습니다 (데이터 누락 의심): %s"), InCompletedIndex, *GetNameSafe(GetOwner()));
		}

		return;
	}

	FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
	ContextHandle.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
		USpyGE_ExperienceGain::StaticClass(), 1.f, ContextHandle);
	if (SpecHandle.IsValid() == false)
		return;

	SpecHandle.Data->SetSetByCallerMagnitude(SpyGameplayTags::Data_Experience_Gain, Reward);
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}
```

- [ ] **Step 8: `GetMissionEntry()` — 변경 없음**

기존 구현 그대로 둔다:

```cpp
const FSpyMissionEntry* USpyMissionComponent::GetMissionEntry(int32 InIndex) const
{
	return (MissionConfig != nullptr ? MissionConfig->GetMission(InIndex) : nullptr);
}
```

- [ ] **Step 9: 컴파일 확인**

Task 1의 `ExperienceReward` 삭제로 깨졌던 `GrantReward()`가 이제 `GetMissionReward()`로 대체돼 컴파일이 통과해야 한다.

- [ ] **Step 10: PIE로 확인**

(Task 4/6 완료 후) 1인 PIE — 처치 미션(Gameplay) 완료 시 보상 없이 즉시 다음(Dialogue) 미션으로 전진 + 자동 수락되는지. 레이븐에게 재대화(보고) 시 그 자리에서 보상 지급 + 팰컨(Gameplay) 미션으로 전진하지만 **아직 미수락 상태**인지(§5-2-1 검증 핵심). 레벨 미션(팰컨) 수락 전에 이미 레벨 3을 넘긴 상태를 의도적으로 만들어(레이븐 보고 직전 초과 킬 등) **수락 즉시** 완료되는지(§5-5 재평가), **그 전(레이븐 보고 시점)에는 완료되지 않는지**(§5-2-1 레이스 수정 검증).

- [ ] **Step 11: Stage**

```bash
git add SkillProject/Source/SkillProject/System/SpyMissionComponent.h SkillProject/Source/SkillProject/System/SpyMissionComponent.cpp
```
메시지(안): `[Fix] USpyMissionComponent — 2026-08-01b 보고 서브상태 모델 되돌리기 + PendingEvents bAccepted 레이스 수정`

---

### Task 4: NPC 도메인 — `ISpyNPCRoot` + `ASpyNPCCharacter` (4상태 판정, 보고는 카드 없이 즉시 완료)

**Files:**
- Rewrite: `SkillProject/Source/SkillProject/NPC/CommonInterface.NPC.h` (기존 파일 — 전면 교체)
- Rewrite: `SkillProject/Source/SkillProject/NPC/SpyNPCCharacter.h` (기존 파일 — 전면 교체)
- Create: `SkillProject/Source/SkillProject/NPC/SpyNPCCharacter.cpp`

**Interfaces:**
- Consumes: `ResolveNPCDialogueState`/`ResolveDialogueText`(Task 2), `USpyNPCConfig`/`FSpyNPCRow`/`FSpyDialogueRow`/`FSpyMissionCommunicationRow`/`ESpyMissionCommRole`(Task 2), `USpyMissionComponent::GetMissionIndex`/`IsCurrentAccepted`/`GetMissionEntry`(기존+Task 3)
- Produces:
  - `ISpyNPCRoot::RequestInteract(APlayerController* Requester) -> FSpyNPCDialogueResult`
  - `ISpyNPCRoot::GetNPCId() -> int32`
  - `FSpyNPCDialogueResult { ESpyNPCDialogueState State; FText NPCName; FText Line; bool bShowMissionCard; FText MissionTitle; FText MissionDescription; }` — Task 6의 `Client_ReceiveDialogueResult` RPC 파라미터

- [ ] **Step 1: `NPC/CommonInterface.NPC.h` 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Util/DefineEnum.h"

#include "CommonInterface.NPC.generated.h"

//# 서버가 상호작용 판정 결과로 클라이언트에 돌려주는 값.
USTRUCT()
struct FSpyNPCDialogueResult
{
	GENERATED_BODY()

public:
	UPROPERTY()
	ESpyNPCDialogueState State = ESpyNPCDialogueState::Default;

	UPROPERTY()
	FText NPCName;

	UPROPERTY()
	FText Line;

	//# Offer 상태일 때만 true — 미션 수락 카드를 띄운다. Report 는 카드가 없다
	UPROPERTY()
	bool bShowMissionCard = false;

	//# Offer 상태일 때만 채운다. 보상 텍스트는 없다 — Gameplay 타입은 보상이 없다
	UPROPERTY()
	FText MissionTitle;

	UPROPERTY()
	FText MissionDescription;
};

//# NPC 도메인의 유일한 외부 진입점 (cpp-style §13).
//# 하위 컴포넌트(상호작용 SphereComponent)가 1개뿐이라 §13 예외 대상이지만,
//# 소비자(플레이어 상호작용 컴포넌트)를 위해 인터페이스는 미리 뺀다.
UINTERFACE(MinimalAPI)
class USpyNPCRoot : public UInterface
{
	GENERATED_BODY()
};

class ISpyNPCRoot
{
	GENERATED_BODY()

public:
	//# 서버 권한에서만 유효한 결과를 반환한다. 상태가 Report 면 이 호출 안에서
	//# AddProgress(Event_Mission_Report, 1) 까지 함께 처리한다.
	virtual FSpyNPCDialogueResult RequestInteract(APlayerController* Requester) = 0;

	virtual int32 GetNPCId() const = 0;
};
```

- [ ] **Step 2: `NPC/SpyNPCCharacter.h` 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularCharacter.h"
#include "NPC/CommonInterface.NPC.h"

#include "SpyNPCCharacter.generated.h"

class USphereComponent;
class USpyNPCConfig;

//# NPC 도메인 루트. NPCId 하나로 3개 DataTable(USpyNPCConfig 경유)을 BeginPlay에 1회 스캔해
//# 자신의 Default/Offer/InProgress/Report 대사와 담당 MissionId(Offer/Report) 를 캐싱한다.
UCLASS()
class SKILLPROJECT_API ASpyNPCCharacter : public AModularCharacter, public ISpyNPCRoot
{
	GENERATED_BODY()

public:
	ASpyNPCCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//# ISpyNPCRoot
	virtual FSpyNPCDialogueResult RequestInteract(APlayerController* Requester) override;
	virtual int32 GetNPCId() const override { return NPCId; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	void CacheNPCData();

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> InteractionSphere;

	//# NPC 테이블 행 식별자이자 MissionCommunication.NPCId 매칭 키
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue")
	int32 NPCId = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Dialogue")
	TObjectPtr<USpyNPCConfig> NPCConfig;

	//# BeginPlay 1회 캐싱 (§8 — 매 프레임/매 상호작용 조회 금지)
	bool bDataCached = false;
	FText CachedNPCDisplayName;
	FText CachedDefaultLine;
	int32 CachedOfferMissionId = INDEX_NONE;
	FText CachedOfferLine;
	FText CachedInProgressLine;
	int32 CachedReportMissionId = INDEX_NONE;
	FText CachedReportLine;
};
```

- [ ] **Step 3: `NPC/SpyNPCCharacter.cpp` 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "NPC/SpyNPCCharacter.h"
#include "Components/SphereComponent.h"
#include "Character/CommonInterface.Character.h"
#include "ManagerComponent/CommonInterface.Manager.h"
#include "System/SpyMissionComponent.h"
#include "Data/SpyMissionConfig.h"
#include "Data/SpyNPCDialogueRow.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

ASpyNPCCharacter::ASpyNPCCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(GetRootComponent());
	InteractionSphere->SetSphereRadius(300.f);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void ASpyNPCCharacter::BeginPlay()
{
	Super::BeginPlay();

	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ASpyNPCCharacter::OnInteractionSphereBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ASpyNPCCharacter::OnInteractionSphereEndOverlap);

	CacheNPCData();
}

void ASpyNPCCharacter::CacheNPCData()
{
	if (NPCConfig == nullptr)
		return;

	if (NPCConfig->NPCTable != nullptr)
	{
		TArray<FSpyNPCRow*> NPCRows;
		NPCConfig->NPCTable->GetAllRows<FSpyNPCRow>(TEXT("ASpyNPCCharacter::CacheNPCData"), NPCRows);

		for (const FSpyNPCRow* Row : NPCRows)
		{
			if (Row == nullptr || Row->NPCId != NPCId)
				continue;

			CachedNPCDisplayName = Row->NPCDisplayName;
			CachedDefaultLine = ResolveDialogueText(NPCConfig->DialogueTable, Row->DefaultDialogueId);

			break;
		}
	}

	if (NPCConfig->MissionCommunicationTable == nullptr)
		return;

	TArray<FSpyMissionCommunicationRow*> CommRows;
	NPCConfig->MissionCommunicationTable->GetAllRows<FSpyMissionCommunicationRow>(TEXT("ASpyNPCCharacter::CacheNPCData"), CommRows);

	for (const FSpyMissionCommunicationRow* Row : CommRows)
	{
		if (Row == nullptr || Row->NPCId != NPCId)
			continue;

		if (Row->Role == ESpyMissionCommRole::Offer)
		{
			CachedOfferMissionId = Row->MissionId;
			CachedOfferLine = ResolveDialogueText(NPCConfig->DialogueTable, Row->OfferDialogueId);
			CachedInProgressLine = ResolveDialogueText(NPCConfig->DialogueTable, Row->InProgressDialogueId);
		}
		else
		{
			CachedReportMissionId = Row->MissionId;
			CachedReportLine = ResolveDialogueText(NPCConfig->DialogueTable, Row->ReportDialogueId);
		}
	}

	bDataCached = true;
}

void ASpyNPCCharacter::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APawn* OtherPawn = Cast<APawn>(OtherActor);
	if (OtherPawn == nullptr || OtherPawn->IsLocallyControlled() == false)
		return;

	TScriptInterface<ISpyCharacterRoot> CharRoot(OtherActor);
	if (CharRoot.GetObject() == nullptr)
		return;

	TScriptInterface<ISpyInteractionHost> Host = CharRoot->GetInteractionHost();
	if (Host.GetObject() == nullptr)
		return;

	Host->NotifyNPCRangeChanged(this, true);
}

void ASpyNPCCharacter::OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	const APawn* OtherPawn = Cast<APawn>(OtherActor);
	if (OtherPawn == nullptr || OtherPawn->IsLocallyControlled() == false)
		return;

	TScriptInterface<ISpyCharacterRoot> CharRoot(OtherActor);
	if (CharRoot.GetObject() == nullptr)
		return;

	TScriptInterface<ISpyInteractionHost> Host = CharRoot->GetInteractionHost();
	if (Host.GetObject() == nullptr)
		return;

	Host->NotifyNPCRangeChanged(this, false);
}

FSpyNPCDialogueResult ASpyNPCCharacter::RequestInteract(APlayerController* Requester)
{
	FSpyNPCDialogueResult Result;

	if (Requester == nullptr)
		return Result;

	Result.NPCName = CachedNPCDisplayName;

	APawn* RequesterPawn = Requester->GetPawn();
	if (RequesterPawn == nullptr)
		return Result;

	APlayerState* RequesterPS = RequesterPawn->GetPlayerState();
	if (RequesterPS == nullptr)
		return Result;

	USpyMissionComponent* MissionComp = USpyMissionComponent::FindMissionComponent(RequesterPS);
	if (MissionComp == nullptr)
		return Result;

	Result.State = ResolveNPCDialogueState(
		MissionComp->GetMissionIndex(), MissionComp->IsCurrentAccepted(), CachedOfferMissionId, CachedReportMissionId);

	switch (Result.State)
	{
	case ESpyNPCDialogueState::Offer:
		Result.Line = CachedOfferLine;
		Result.bShowMissionCard = true;

		if (const FSpyMissionEntry* Entry = MissionComp->GetMissionEntry(CachedOfferMissionId))
		{
			Result.MissionTitle = Entry->DisplayName;
			Result.MissionDescription = Entry->Description;
		}
		break;

	case ESpyNPCDialogueState::InProgress:
		Result.Line = CachedInProgressLine;
		break;

	case ESpyNPCDialogueState::Report:
		Result.Line = CachedReportLine;

		//# 카드 없이 이 자리에서 완료 처리까지 끝낸다 — 서버 재검증(거리)은 호출부(Task 6)가 이미 마쳤다
		MissionComp->AddProgress(SpyGameplayTags::Event_Mission_Report, 1);
		break;

	case ESpyNPCDialogueState::Default:
	default:
		Result.Line = CachedDefaultLine;
		break;
	}

	return Result;
}
```

- [ ] **Step 4: 컴파일 확인**

Task 6(`ISpyInteractionHost`)가 아직 없으면 이 시점엔 컴파일 오류가 난다 — 정상. Task 6까지 끝난 뒤 재확인한다.

- [ ] **Step 5: Stage**

```bash
git add SkillProject/Source/SkillProject/NPC/
```
메시지(안): `[Feature] ASpyNPCCharacter — NPC 도메인 루트 4상태 재구현 (2026-08-01b ReadyToReport 모델 대체)`

---

### Task 5: UI 위젯 — `SpyDialogueWidget` / `SpyMissionOfferWidget` (Offer 전용, 보상 텍스트 없음)

**Files:**
- Modify: `SkillProject/Source/SkillProject/Util/DefineEnum.h`
- Create: `SkillProject/Source/SkillProject/UI/SpyDialogueWidget.h`
- Create: `SkillProject/Source/SkillProject/UI/SpyDialogueWidget.cpp`
- Create: `SkillProject/Source/SkillProject/UI/SpyMissionOfferWidget.h`
- Create: `SkillProject/Source/SkillProject/UI/SpyMissionOfferWidget.cpp`

**Interfaces:**
- Consumes: 없음
- Produces:
  - `USpyDialogueWidget::ShowLine(FText Name, FText Line)`
  - `USpyMissionOfferWidget::ShowMission(FText Title, FText Description)` — **2026-08-01b와 달리 `RewardText` 인자 없음** (Gameplay 타입 미션은 보상이 없다)
  - `USpyMissionOfferWidget::FOnMissionCardChoice OnAcceptClicked` / `OnDeclineClicked`
  - `ESpyUIType::Dialogue`, `ESpyUIType::MissionOffer`

- [ ] **Step 1: `ESpyUIType`에 두 항목 추가**

`Util/DefineEnum.h`의 기존 `ESpyUIType`:

```cpp
UENUM(BlueprintType)
enum ESpyUIType : uint8
{
	None UMETA(DisplayName = "None"),
	MainHUD UMETA(DisplayName = "MainHUD"),
	HpBar UMETA(DisplayName = "HpBar"),
	Loading UMETA(DisplayName = "Loading"),
	SessionBrowser UMETA(DisplayName = "SessionBrowser"),
	Dialogue UMETA(DisplayName = "Dialogue"),
	MissionOffer UMETA(DisplayName = "MissionOffer"),
};
```

(기존 값 목록은 gameplay-programmer가 실제 파일을 열어 확인 — 위는 `Dialogue`/`MissionOffer` 두 항목만 새로 추가하는 것이 목적이다.)

- [ ] **Step 2: `SpyDialogueWidget.h` 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SpyUserWidget.h"

#include "SpyDialogueWidget.generated.h"

class UTextBlock;

//# NPC 대사 한 줄만 표시하는 대화창. 내부 텍스트 위젯은 캡슐화하고
//# 외부는 ShowLine 하나만 호출한다 (cpp-style §9-2)
UCLASS()
class SKILLPROJECT_API USpyDialogueWidget : public USpyUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void ShowLine(FText InNPCName, FText InLine);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_NPCName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Line;
};
```

- [ ] **Step 3: `SpyDialogueWidget.cpp` 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SpyDialogueWidget.h"
#include "Components/TextBlock.h"

void USpyDialogueWidget::ShowLine(FText InNPCName, FText InLine)
{
	if (Txt_NPCName != nullptr)
		Txt_NPCName->SetText(InNPCName);

	if (Txt_Line != nullptr)
		Txt_Line->SetText(InLine);
}
```

- [ ] **Step 4: `SpyMissionOfferWidget.h` 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SpyUserWidget.h"

#include "SpyMissionOfferWidget.generated.h"

class UTextBlock;
class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMissionCardChoice);

//# 미션 수락/거절 카드. Offer 전용 — Report 는 카드가 없으므로 겸용하지 않는다.
UCLASS()
class SKILLPROJECT_API USpyMissionOfferWidget : public USpyUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void ShowMission(FText InTitle, FText InDescription);

	UPROPERTY(BlueprintAssignable)
	FOnMissionCardChoice OnAcceptClicked;

	UPROPERTY(BlueprintAssignable)
	FOnMissionCardChoice OnDeclineClicked;

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleAcceptButtonClicked();

	UFUNCTION()
	void HandleDeclineButtonClicked();

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Title;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Description;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Accept;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Decline;
};
```

- [ ] **Step 5: `SpyMissionOfferWidget.cpp` 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SpyMissionOfferWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void USpyMissionOfferWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Accept != nullptr)
		Btn_Accept->OnClicked.AddDynamic(this, &USpyMissionOfferWidget::HandleAcceptButtonClicked);

	if (Btn_Decline != nullptr)
		Btn_Decline->OnClicked.AddDynamic(this, &USpyMissionOfferWidget::HandleDeclineButtonClicked);
}

void USpyMissionOfferWidget::ShowMission(FText InTitle, FText InDescription)
{
	if (Txt_Title != nullptr)
		Txt_Title->SetText(InTitle);

	if (Txt_Description != nullptr)
		Txt_Description->SetText(InDescription);
}

void USpyMissionOfferWidget::HandleAcceptButtonClicked()
{
	OnAcceptClicked.Broadcast();
}

void USpyMissionOfferWidget::HandleDeclineButtonClicked()
{
	OnDeclineClicked.Broadcast();
}
```

- [ ] **Step 6: 컴파일 확인 + Stage**

```bash
git add SkillProject/Source/SkillProject/Util/DefineEnum.h SkillProject/Source/SkillProject/UI/SpyDialogueWidget.h SkillProject/Source/SkillProject/UI/SpyDialogueWidget.cpp SkillProject/Source/SkillProject/UI/SpyMissionOfferWidget.h SkillProject/Source/SkillProject/UI/SpyMissionOfferWidget.cpp
```
메시지(안): `[Feature] SpyDialogueWidget/SpyMissionOfferWidget — NPC 대화·미션 카드 위젯 (Offer 전용, 보상 텍스트 없음)`

---

### Task 6: 플레이어 상호작용 컴포넌트 + Character 루트 배선 (`Server_ReportCurrentMission` RPC 없음)

**Files:**
- Modify(또는 확인): `SkillProject/Source/SkillProject/ManagerComponent/CommonInterface.Manager.h`
- Create: `SkillProject/Source/SkillProject/ManagerComponent/SpyInteractionComponent.h`
- Create: `SkillProject/Source/SkillProject/ManagerComponent/SpyInteractionComponent.cpp`
- Modify(또는 확인): `SkillProject/Source/SkillProject/Character/CommonInterface.Character.h`
- Modify: `SkillProject/Source/SkillProject/Character/SpyCharacter.h`
- Modify: `SkillProject/Source/SkillProject/Character/SpyCharacter.cpp`

**Interfaces:**
- Consumes: `ISpyNPCRoot`(Task 4), `FSpyNPCDialogueResult`(Task 4, `bShowMissionCard`만 — `bShowReportCard` 없음), `ESpyUIType::Dialogue`/`MissionOffer`(Task 5), `USpyDialogueWidget`/`USpyMissionOfferWidget`(Task 5), `USpyMissionComponent::AcceptCurrentMission`(Task 3)
- Produces:
  - `ISpyInteractionHost::NotifyNPCRangeChanged(AActor* NPCActor, bool bInRange)`
  - `USpyInteractionComponent::TryInteract()` — Task 7의 입력 바인딩이 호출
  - `ISpyCharacterRoot::GetInteractionHost() -> TScriptInterface<ISpyInteractionHost>`

- [ ] **Step 1: `ISpyInteractionHost` 존재 확인/추가**

`ManagerComponent/CommonInterface.Manager.h`를 먼저 열어 `ISpyInteractionHost`가 이미 있는지 확인한다(git status상 이 파일이 이미 한 번 수정됐다). 없으면 끝에 추가:

```cpp
//# 상호작용 호스트 — USpyInteractionComponent 가 구현한다.
UINTERFACE(MinimalAPI)
class USpyInteractionHost : public UInterface
{
	GENERATED_BODY()
};

class ISpyInteractionHost
{
	GENERATED_BODY()

public:
	virtual void NotifyNPCRangeChanged(AActor* NPCActor, bool bInRange) = 0;
};
```

- [ ] **Step 2: `ISpyCharacterRoot`에 접근자 존재 확인/추가**

`Character/CommonInterface.Character.h`를 먼저 열어 `GetInteractionHost()`가 이미 선언돼 있는지 확인한다. 없으면 추가:

```cpp
	virtual TScriptInterface<ISpyInteractionHost> GetInteractionHost() const = 0;
```

- [ ] **Step 3: `SpyInteractionComponent.h` 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ManagerComponent/CommonInterface.Manager.h"
#include "NPC/CommonInterface.NPC.h"

#include "SpyInteractionComponent.generated.h"

//# 플레이어 측 상호작용 컴포넌트. 근접 NPC 하나를 추적하고,
//# 입력이 들어오면 서버에 상호작용/수락 요청을 보낸다.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SKILLPROJECT_API USpyInteractionComponent : public UActorComponent, public ISpyInteractionHost
{
	GENERATED_BODY()

public:
	USpyInteractionComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//# ISpyInteractionHost
	virtual void NotifyNPCRangeChanged(AActor* NPCActor, bool bInRange) override;

	UFUNCTION(BlueprintCallable)
	void TryInteract();

protected:
	UFUNCTION(Server, Reliable)
	void Server_RequestInteract(AActor* TargetNPC);

	UFUNCTION(Server, Reliable)
	void Server_AcceptCurrentMission();

	UFUNCTION(Client, Reliable)
	void Client_ReceiveDialogueResult(FSpyNPCDialogueResult Result);

	//# MissionOfferWidget의 [수락] 버튼 — Offer 카드는 이 상태에서만 뜨므로 분기가 필요 없다
	UFUNCTION()
	void HandleMissionCardConfirmed();

	UFUNCTION()
	void HandleMissionCardDismissed();

protected:
	UPROPERTY(Transient)
	TObjectPtr<AActor> NearbyNPC;
};
```

**`Server_ReportCurrentMission` RPC는 만들지 않는다** — 보고는 `Server_RequestInteract` 안에서 `ASpyNPCCharacter::RequestInteract`가 이미 완료 처리를 끝낸다(Task 4). `LastDialogueState` 필드도 필요 없다 — 카드가 뜨는 상태가 `Offer` 하나뿐이라 `HandleMissionCardConfirmed`가 분기할 이유가 없다.

- [ ] **Step 4: `SpyInteractionComponent.cpp` 작성**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "ManagerComponent/SpyInteractionComponent.h"
#include "Manager/SpyUIManager.h"
#include "Util/DefineEnum.h"
#include "UI/SpyDialogueWidget.h"
#include "UI/SpyMissionOfferWidget.h"
#include "System/SpyMissionComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"

USpyInteractionComponent::USpyInteractionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void USpyInteractionComponent::NotifyNPCRangeChanged(AActor* NPCActor, bool bInRange)
{
	if (bInRange)
	{
		NearbyNPC = NPCActor;
		return;
	}

	if (NearbyNPC == NPCActor)
		NearbyNPC = nullptr;

	//# 프롬프트 표시/숨김은 HUD 쪽 델리게이트 구독으로 처리한다(이 컴포넌트는 상태만 들고 있음).
	//# HUD 프롬프트 배선은 WBP 목업 승인 후 사용자가 디자이너에서 연결한다.
}

void USpyInteractionComponent::TryInteract()
{
	if (NearbyNPC == nullptr)
		return;

	Server_RequestInteract(NearbyNPC);
}

void USpyInteractionComponent::Server_RequestInteract_Implementation(AActor* TargetNPC)
{
	if (TargetNPC == nullptr)
		return;

	AActor* Owner = GetOwner();
	if (Owner == nullptr || Owner->HasAuthority() == false)
		return;

	const float DistanceCm = FVector::Dist(Owner->GetActorLocation(), TargetNPC->GetActorLocation());
	static constexpr float MaxInteractDistanceCm = 300.f; //# NPC SphereComponent 트리거 반경과 동일 (기획서 §5-1)
	if (DistanceCm > MaxInteractDistanceCm)
		return;

	TScriptInterface<ISpyNPCRoot> NPCRoot(TargetNPC);
	if (NPCRoot.GetObject() == nullptr)
		return;

	APlayerController* PC = Cast<APlayerController>(Owner->GetInstigatorController());
	if (PC == nullptr)
		return;

	const FSpyNPCDialogueResult Result = NPCRoot->RequestInteract(PC);

	Client_ReceiveDialogueResult(Result);
}

void USpyInteractionComponent::Server_AcceptCurrentMission_Implementation()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr || Owner->HasAuthority() == false)
		return;

	APlayerController* PC = Cast<APlayerController>(Owner->GetInstigatorController());
	if (PC == nullptr)
		return;

	APawn* Pawn = PC->GetPawn();
	if (Pawn == nullptr)
		return;

	APlayerState* PS = Pawn->GetPlayerState();
	if (PS == nullptr)
		return;

	USpyMissionComponent* MissionComp = USpyMissionComponent::FindMissionComponent(PS);
	if (MissionComp == nullptr)
		return;

	MissionComp->AcceptCurrentMission();
}

void USpyInteractionComponent::Client_ReceiveDialogueResult_Implementation(FSpyNPCDialogueResult Result)
{
	USpyUIManager* UIManager = Cast<USpyUIManager>(USKUIManager::Get(this));
	if (UIManager == nullptr)
		return;

	UIManager->OpenSpyUI(ESpyUIType::Dialogue);

	if (Result.bShowMissionCard)
	{
		UIManager->OpenSpyUI(ESpyUIType::MissionOffer);
	}

	//# 위젯 인스턴스에 ShowLine/ShowMission으로 실제 텍스트를 채우는 배선은 §알려진 갭 참조
}

void USpyInteractionComponent::HandleMissionCardConfirmed()
{
	//# 카드가 뜨는 상태는 Offer 하나뿐이라 분기가 필요 없다
	Server_AcceptCurrentMission();
}

void USpyInteractionComponent::HandleMissionCardDismissed()
{
	if (USpyUIManager* UIManager = Cast<USpyUIManager>(USKUIManager::Get(this)))
	{
		UIManager->CloseSpyUI(ESpyUIType::MissionOffer);
	}
}
```

**⚠ 알려진 갭(gameplay-programmer 구현 시 확정 필요, 2026-08-01b와 동일한 미해결 사항)**: `Client_ReceiveDialogueResult_Implementation`이 `OpenSpyUI`로 위젯을 연 뒤 그 인스턴스에 `ShowLine`/`ShowMission`을 호출하는 연결점이 `USKUIManager`/`USpyUIManager`의 기존 API만으로는 없다(`OpenSpyUI`가 위젯 포인터를 돌려주지 않는다). 데이터 자체는 Task 4가 이미 `FSpyNPCDialogueResult`에 채워 넣으므로 이 갭은 순수하게 "그 값을 위젯에 전달하는 배선"의 문제다. (a) `USpyUIManager`에 "방금 연 위젯 반환" 오버로드 추가, (b) 위젯이 열릴 때 `USpyInteractionComponent`가 들고 있는 마지막 결과를 직접 구독/폴링 — 둘 중 gameplay-programmer가 확정한다.

- [ ] **Step 5: `ASpyCharacter`에 컴포넌트 소유 + 접근자 구현 추가**

`Character/SpyCharacter.h`:

```cpp
	//# ISpyCharacterRoot
	virtual TScriptInterface<ISpyInteractionHost> GetInteractionHost() const override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<class USpyInteractionComponent> InteractionComponent;
```

`Character/SpyCharacter.cpp` — 생성자에서 `CreateDefaultSubobject<USpyInteractionComponent>(TEXT("InteractionComponent"))` (다른 컴포넌트와 동일 패턴), 접근자 구현:

```cpp
TScriptInterface<ISpyInteractionHost> ASpyCharacter::GetInteractionHost() const
{
	TScriptInterface<ISpyInteractionHost> Result;
	Result.SetObject(InteractionComponent);
	Result.SetInterface(Cast<ISpyInteractionHost>(InteractionComponent));

	return Result;
}
```

(기존 `GetParkourHost`/`GetGrappleHost` 구현이 이미 있으면 정확히 같은 패턴을 그대로 복사한다.)

- [ ] **Step 6: 컴파일 확인**

이 Task가 끝나면 Task 4(`ASpyNPCCharacter`)도 함께 컴파일돼야 한다.

- [ ] **Step 7: Stage**

```bash
git add SkillProject/Source/SkillProject/ManagerComponent/CommonInterface.Manager.h SkillProject/Source/SkillProject/ManagerComponent/SpyInteractionComponent.h SkillProject/Source/SkillProject/ManagerComponent/SpyInteractionComponent.cpp SkillProject/Source/SkillProject/Character/CommonInterface.Character.h SkillProject/Source/SkillProject/Character/SpyCharacter.h SkillProject/Source/SkillProject/Character/SpyCharacter.cpp
```
메시지(안): `[Feature] USpyInteractionComponent — NPC 상호작용/미션 수락 RPC 배선 (보고 RPC 없음, 대화 안에서 완료)`

---

### Task 7: 입력 바인딩 + `Event_Mission_Report` 태그

**Files:**
- Modify: `SkillProject/Source/SkillProject/Util/SpyGameplayTags.h`
- Modify: `SkillProject/Source/SkillProject/Util/SpyGameplayTags.cpp`
- Modify: `SkillProject/Source/SkillProject/Input/SpyInputComponent.h`
- Modify: `SkillProject/Source/SkillProject/Input/SpyInputComponent.cpp`

**Interfaces:**
- Consumes: `USpyInteractionComponent::TryInteract()`(Task 6)
- Produces: `SpyGameplayTags::Event_Mission_Report`(Task 3/4가 이미 참조하므로 이 Task 전에 gameplay-programmer가 임시로 선언해 뒀을 수 있다 — 여기서 정식 등록으로 정리) / 없음 (입력은 최종 소비 지점)

- [ ] **Step 1: `Event_Mission_Report` + `Input_Native_Interact` 태그 추가**

`SpyGameplayTags.h`의 기존 `Event_Mission_*` 그룹과 `Input_Native_*` 그룹에 각각 추가:

```cpp
	SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Mission_Report);
```

```cpp
	SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Native_Interact);
```

`SpyGameplayTags.cpp`:

```cpp
	UE_DEFINE_GAMEPLAY_TAG(Event_Mission_Report, "Event.Mission.Report");
```

```cpp
	UE_DEFINE_GAMEPLAY_TAG(Input_Native_Interact, "Input.Native.Interact");
```

(정확한 삽입 위치는 gameplay-programmer가 기존 `Event_Mission_Kill`/`Event_Mission_Level`/`Event_Mission_Combo`, `Input_Native_Move`/`Input_Native_Look`/`Input_Native_CursorToggle` 옆에 같은 그룹으로 맞춘다.)

- [ ] **Step 2: `USpyInputComponent`에 `Interact` 바인딩 추가**

`SpyInputComponent.h`의 `protected:` 함수 목록에 추가:

```cpp
	void Interact(const FInputActionValue& InValue);
```

`SpyInputComponent.cpp`의 기존 `BindNativeAction` 호출 블록에 한 줄 추가:

```cpp
					SpyIC->BindNativeAction(InputConfig, SpyGameplayTags::Input_Native_Interact, ETriggerEvent::Started, this, &ThisClass::Interact);
```

`SpyInputComponent.cpp`에 새 함수 추가:

```cpp
void USpyInputComponent::Interact(const FInputActionValue& InValue)
{
	APawn* Pawn = GetPawn<APawn>();
	if (Pawn == nullptr)
		return;

	ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(Pawn);
	if (SpyCharacter == nullptr)
		return;

	TScriptInterface<ISpyInteractionHost> Host = SpyCharacter->GetInteractionHost();
	if (Host.GetObject() == nullptr)
		return;

	USpyInteractionComponent* InteractionComp = Cast<USpyInteractionComponent>(Host.GetObject());
	if (InteractionComp == nullptr)
		return;

	InteractionComp->TryInteract();
}
```

`SpyInputComponent.cpp` 상단 include에 추가:

```cpp
#include "ManagerComponent/SpyInteractionComponent.h"
```

- [ ] **Step 3: 컴파일 확인**

전체 프로젝트 빌드. 이 Task가 마지막 조각이므로 여기까지 오면 Task 1~7이 전부 컴파일돼야 한다.

- [ ] **Step 4: Stage**

```bash
git add SkillProject/Source/SkillProject/Util/SpyGameplayTags.h SkillProject/Source/SkillProject/Util/SpyGameplayTags.cpp SkillProject/Source/SkillProject/Input/SpyInputComponent.h SkillProject/Source/SkillProject/Input/SpyInputComponent.cpp
```
메시지(안): `[Feature] USpyInputComponent — NPC 상호작용 입력(Interact) 바인딩 + Event_Mission_Report 태그 등록`

---

## 에셋 작업 (코드 밖, 목업 승인 이후 사용자/MCP)

이 plan의 코드 Task가 전부 끝난 뒤에만 착수한다.

- [ ] `DT_SpyNPC`(6행)/`DT_SpyDialogue`(24행)/`DT_SpyMissionCommunication`(12행)/`DT_SpyMissionReward`(6행) DataTable 에셋 생성 — 실제 값은 `docs/design/npc-mission-dialogue.md` §3
- [ ] `DA_SpyNPCConfig` 생성 — 3개 DataTable(NPC/Dialogue/MissionCommunication) 연결
- [ ] `DA_SpyMissionConfig`의 `Missions[]` 12행 재입력 + `MissionRewardTable`에 `DT_SpyMissionReward` 연결
- [ ] `IA_Interact` InputAction 생성 + 기존 IMC에 `F` 키 매핑 + `USpyInputConfig`의 `NativeInputActions`에 `Input.Native.Interact` 태그로 등록
- [ ] NPC 블루프린트 6종 배치 — `NPCId`(0~5), `NPCConfig`(`DA_SpyNPCConfig`) 지정
- [ ] `WBP_Dialogue`, `WBP_MissionOffer` 위젯 배치 (spec §8-2 목업 기준) — MCP로 위젯 트리 조립 후 사용자가 디자이너에서 compile
- [ ] `USpyUIManager`에 위젯 등록(이름→클래스 매핑, 기존 `MainHUD`/`HpBar` 패턴 따름)

## 인게임 확인 (PIE)

- [ ] 1인 PIE — 레이븐에게 상호작용 → Offer 대사 확인 → 수락 카드 → 수락 → 미션 진행 → **목표 달성 시 보상 없이 즉시 다음(Dialogue) 미션으로 전진 + 자동 수락되는지** → 레이븐에게 재대화 시 카드 없이 그 자리에서 보상 지급 + 팰컨(Gameplay) 미션으로 전진하는지 → 6개 NPC 순회하며 완주
- [ ] 거절 시 상태 변화 없음 + 재대화 시 다시 Offer로 뜨는지 확인
- [ ] **레벨 미션(팰컨) 수락 전에 이미 레벨 3을 넘긴 상태를 의도적으로 만들어**(레이븐 보고 직전 초과 킬 등) — **레이븐 보고 시점에는 완료되지 않고**, **팰컨 Offer를 수락하는 순간에만** 즉시 완료되는지 확인 (spec §5-2-1 레이스 수정 + §5-5 재평가 검증 — 이번 개정의 핵심 리스크)
- [ ] §4-3 검산2(기획서)의 "이중 상호작용" 퇴화 케이스 — 위 시나리오에서 팰컨 수락 직후 InProgress 대사 없이 바로 F를 다시 눌러야 Report 문구가 뜨는지, 위화감이 있는지 확인(D3)
- [ ] 마지막 미션(폭스) 보고 완료 시점에만 "전체 완료" HUD 문구가 뜨는지 확인
- [ ] 2인 PIE — 한 플레이어의 수락/보고가 다른 플레이어 상태에 영향 없는지 확인
- [ ] 데디케이티드 서버(또는 리슨 서버) 기준으로 원격 클라이언트의 상호작용 RPC 왕복 확인

---

## Self-Review 메모

- **Spec coverage**: spec §3(NPC 배치·인터페이스)→Task 4·6, §4(5테이블 데이터)→Task 1·2, §5(미션 컴포넌트, §5-2-1 레이스 수정 포함)→Task 3, §6(4상태 판정)→Task 2, §7(서버 흐름, 카드 없는 보고)→Task 4·6, §8(UI, Offer 전용)→Task 5·6, §9(엣지케이스)→Task 3·4·6 코드와 PIE 체크리스트에 반영.
- **Placeholder scan**: Task 4에 `FSpyNPCRow` 필드 부재로 인한 실제 갭 하나를 명시적으로 남겼다(TBD가 아니라 두 해결 경로(a)/(b)를 제시) — Task 6의 위젯 배선 갭도 동일 패턴(2026-08-01b부터 이어진 기존 갭, 방향만 제시).
- **Type consistency**: `FSpyNPCDialogueResult`(Task 4)의 `bShowMissionCard`가 Task 6 `Client_ReceiveDialogueResult_Implementation`과 일치(`bShowReportCard` 필드 완전히 제거됨 — 2026-08-01b 잔재 없음 확인). `ESpyNPCDialogueState`(Task 2) 4개 값(`Default`/`Offer`/`InProgress`/`Report`)이 Task 2 테스트·Task 4 `switch`문에서 일관됨. `ISpyInteractionHost::NotifyNPCRangeChanged`(Task 6) 시그니처가 Task 4 호출부와 일치. `USpyMissionComponent::GetMissionEntry`/`IsCurrentAccepted`(Task 3, 시그니처 불변)가 Task 4 호출부와 일치. `USpyMissionConfig::GetMissionReward`(Task 1)가 Task 3 `GrantReward()`에서 쓰는 이름과 일치.
- **2026-08-01b 잔재 확인**: `bObjectiveMet`/`ReportCurrentMission`/`IsCurrentObjectiveMet`/`ReadyToReport`/`bShowReportCard`/`Server_ReportCurrentMission`/`LastDialogueState` — 전부 이 plan의 어떤 Task에도 등장하지 않는다(Task 3 Step 2·5에서 명시적으로 삭제 대상으로 지정). `FSpyNPCDialogueRow`(구 5줄 단일 로우)도 Task 2가 완전히 대체한다.
