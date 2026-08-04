# 미션 목표 바닥 길 안내 (Ground Path Navigation) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 미션 수락 시점부터 완료까지, 목표 좌표까지의 경로를 NavMesh로 계산해 바닥에 연속 글로우 라인으로 표시하는 로컬 클라이언트 전용 연출을 추가한다.

**Architecture:** `USpyMissionConfig`에 선택적 관계 DataTable(`Mission_TargetLocation`)을 추가하고, `USpyMissionComponent`에 `OnMissionAccepted` 델리게이트와 목표 좌표 조회 패스스루를 추가한다. 신규 `USpyNavigationComponent`(ManagerComponent, 로컬 컨트롤 전용)가 이 델리게이트를 구독해 NavMesh 경로를 주기 재계산하고, 순수 함수 `SpyNavPathMath::BuildSplineSegments`로 만든 세그먼트를 `USplineMeshComponent` 풀에 반영한다.

**Tech Stack:** UE5.7 C++, GameplayAbilities(간접), NavigationSystemV1(NavMesh), UDataTable, Unreal Automation(에디터 내 수동 실행).

**Spec:** `docs/superpowers/specs/2026-08-04-mission-ground-navigation-design.md`

## Global Constraints

- **엔진/모듈**: Unreal Engine 5.7, 대상 모듈은 전부 `SkillProject`(Runtime). 새 클래스 추가 후 Visual Studio 프로젝트 재생성이 필요할 수 있다 (CLAUDE.md "새 C++ 클래스 추가 후").
- **빌드**: 전용 CLI 빌드 명령 없음 — Unreal Editor 또는 Visual Studio에서 컴파일한다 (project.md `mcp` 항목).
- **테스트 실행**: 전용 CLI 테스트 러너 없음 — Unreal Editor `Window > Test Automation`(Session Frontend Automation 탭)에서 `SkillProject.System.Mission.*` / `SkillProject.Navigation.*` 등 필터로 **수동** 실행한다. 각 태스크의 "테스트 실행" 스텝은 이 수동 절차를 가리킨다.
- **코딩 스타일** (cpp-style.md, 예외 없음): 가드 절은 중괄호 없이 개행(§5), 로컬 변수 `auto` 금지(§6, 예외 케이스만), `!` 단항 부정 금지 — `== false`/`== nullptr` 명시(§7), 주석은 `//#`만(§4, 2줄 내외), `TObjectPtr<>` 사용, 매직 넘버 금지(§15) — 튜닝 수치는 `EditDefaultsOnly` 로 노출.
- **DataTable 설계**: 선택적 관계는 sentinel 값이 아니라 별도 관계 테이블로 분리하고, 이름은 `<부모>_<관계명>` 규칙을 따른다(cpp-style §14-1). `Mission_TargetLocation` 이 이 규칙의 적용 사례다.
- **컴포넌트 탐색 금지**: `Tick`/이벤트 핸들러 경로에서 `FindComponentByClass`/`GetAllActorsOfClass` 반복 호출 금지 — 초기화 시점 1회 캐싱 또는 델리게이트 구독으로 대체(cpp-style §8). 본 계획의 `USpyNavigationComponent` 는 `SpyMainHUD::TryBindMissionComponent` 와 동일한 "재시도 타이머 + 1회 바인딩" 패턴을 따른다.
- **직접 참조 방식**: `USpyNavigationComponent` 는 `USpyMissionComponent` 를 인터페이스가 아니라 구체 클래스로 직접 참조한다 — 기존 `USpyMainHUD` 가 이미 동일하게 직접 참조하는 선례를 따른다(spec §8 열린 질문에 대한 확정 답).
- **커밋**: 각 태스크의 "커밋" 스텝은 `git add` 로 스테이징만 하고 커밋 메시지(안)를 제시한다. **`git commit` 은 절대 자동 실행하지 않는다** — 사용자가 직접 수행한다. 메시지 포맷은 `[Tag] ClassName — 요약`(git-conventions.md).
- **네이밍**: 프로젝트 접두사 `Spy`, 파일은 `SkillProject/Source/SkillProject/<도메인>/` 하위, 테스트는 `<도메인>/Tests/Spy<Feature>Tests.cpp`, 등록 문자열 `"SkillProject.<도메인>.<기능>.<케이스>"` (기존 `SpyMissionTests.cpp`/`SpyHUDMathTests.cpp` 선례).

---

### Task 1: `Mission_TargetLocation` 관계 테이블 — `USpyMissionConfig`

**Files:**
- Modify: `SkillProject/Source/SkillProject/Data/SpyMissionConfig.h`
- Modify: `SkillProject/Source/SkillProject/Data/SpyMissionConfig.cpp`
- Test: `SkillProject/Source/SkillProject/System/Tests/SpyMissionTests.cpp` (기존 파일에 케이스 추가)

**Interfaces:**
- Produces: `struct FSpyMission_TargetLocationRow { int32 MissionId; FVector TargetLocation; }` (public, `SpyMissionConfig.h`에 정의). `const FSpyMission_TargetLocationRow* USpyMissionConfig::GetMissionTargetLocation(int32 InMissionId) const` — Task 2가 소비한다.

- [ ] **Step 1: 프로덕션 스텁 추가 (컴파일되는 최소 형태)**

`SpyMissionConfig.h` — `FSpyMissionRewardRow` 정의(기존 65~78행) 바로 뒤, `FSpyMissionProgressResult` 정의(기존 81행) 앞에 삽입:

```cpp
//# Mission 의 선택적 관계(§14-1) — 목표 지점이 정의된 미션에만 행이 존재한다.
//# 명명 규칙(§14-1-5): Mission_TargetLocation. Dialogue 타입도 NPCId 대신 이 좌표를 그대로 쓴다.
USTRUCT(BlueprintType)
struct FSpyMission_TargetLocationRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	int32 MissionId = 0;

	UPROPERTY(EditAnywhere)
	FVector TargetLocation = FVector::ZeroVector;
};
```

같은 파일, `MissionRewardTable` UPROPERTY(기존 116~117행) 바로 뒤에 삽입:

```cpp
	//# 선택적 관계 — 목표 지점이 있는 미션만 행 존재. RowStruct = FSpyMission_TargetLocationRow
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	TObjectPtr<UDataTable> MissionTargetLocationTable;
```

`GetMissionReward` 선언(기존 135~136행) 바로 뒤에 삽입:

```cpp
	//# MissionId 로 목표 좌표를 조회한다. 행이 없으면(목표 미정의) nullptr — sentinel 아님(§14-1)
	const FSpyMission_TargetLocationRow* GetMissionTargetLocation(int32 InMissionId) const;
```

`SpyMissionConfig.cpp` 끝에 스텁(항상 nullptr) 추가:

```cpp
const FSpyMission_TargetLocationRow* USpyMissionConfig::GetMissionTargetLocation(int32 InMissionId) const
{
	return nullptr;
}
```

- [ ] **Step 2: 실패하는 테스트 작성**

`SpyMissionTests.cpp` — 기존 `SpyMissionTests_AddReward` 헬퍼(13~27행) 바로 뒤에 동일 패턴의 헬퍼를 추가:

```cpp
//# 테스트 전용 헬퍼 — MissionId 별 목표 좌표 행을 MissionTargetLocationTable 에 추가한다.
static void SpyMissionTests_AddTargetLocation(USpyMissionConfig* Config, int32 MissionId, const FVector& Location)
{
	if (Config->MissionTargetLocationTable == nullptr)
	{
		UDataTable* Table = NewObject<UDataTable>();
		Table->RowStruct = FSpyMission_TargetLocationRow::StaticStruct();
		Config->MissionTargetLocationTable = Table;
	}

	FSpyMission_TargetLocationRow Row;
	Row.MissionId = MissionId;
	Row.TargetLocation = Location;

	Config->MissionTargetLocationTable->AddRow(FName(*FString::Printf(TEXT("TargetLocation_%d"), MissionId)), Row);
}
```

파일 끝(`#endif` 바로 앞)에 테스트 두 개를 추가:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetLocationFoundTest,
	"SkillProject.System.Mission.TargetLocation.Found",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetLocationFoundTest::RunTest(const FString& Parameters)
{
	USpyMissionConfig* Config = SpyMissionTests_MakeConfig();
	SpyMissionTests_AddTargetLocation(Config, 1, FVector(100.f, 200.f, 0.f));

	const FSpyMission_TargetLocationRow* Row = Config->GetMissionTargetLocation(1);

	if (Row == nullptr)
	{
		AddError(TEXT("Expected a target location row for MissionId 1"));

		return false;
	}

	TestEqual(TEXT("Target location matches"), Row->TargetLocation, FVector(100.f, 200.f, 0.f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionTargetLocationMissingTest,
	"SkillProject.System.Mission.TargetLocation.Missing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionTargetLocationMissingTest::RunTest(const FString& Parameters)
{
	//# 목표 지점이 정의되지 않은 미션(2)은 조회가 nullptr 이어야 한다 — sentinel 아님(§14-1)
	USpyMissionConfig* Config = SpyMissionTests_MakeConfig();
	SpyMissionTests_AddTargetLocation(Config, 1, FVector(100.f, 200.f, 0.f));

	TestNull(TEXT("Mission 2 has no target location row"), Config->GetMissionTargetLocation(2));

	return true;
}
```

- [ ] **Step 3: 테스트 실행 — 실패 확인**

Unreal Editor에서 컴파일(핫 리로드 또는 재시작) 후 `Window > Test Automation` → Automation 탭 → `SkillProject.System.Mission.TargetLocation` 필터 → 두 테스트 체크 → Start Tests.
Expected: `Found` 테스트 FAIL (스텁이 항상 `nullptr` 반환), `Missing` 테스트는 이미 PASS(우연히 스텁과 일치) — 두 케이스가 서로 다른 경로를 검증하므로 문제 없음.

- [ ] **Step 4: 실제 구현**

`SpyMissionConfig.cpp` 의 스텁을 `GetMission`(기존 21~36행)과 동일한 `GetAllRows` 선형 스캔 패턴으로 교체:

```cpp
const FSpyMission_TargetLocationRow* USpyMissionConfig::GetMissionTargetLocation(int32 InMissionId) const
{
	if (MissionTargetLocationTable == nullptr)
		return nullptr;

	TArray<FSpyMission_TargetLocationRow*> Rows;
	MissionTargetLocationTable->GetAllRows<FSpyMission_TargetLocationRow>(TEXT("USpyMissionConfig::GetMissionTargetLocation"), Rows);

	for (const FSpyMission_TargetLocationRow* Row : Rows)
	{
		if (Row != nullptr && Row->MissionId == InMissionId)
			return Row;
	}

	return nullptr;
}
```

- [ ] **Step 5: 테스트 실행 — 통과 확인**

Step 3 과 동일 절차 재실행. Expected: 두 테스트 모두 PASS.

- [ ] **Step 6: 스테이징 + 커밋 메시지(안) 제시**

```bash
git add SkillProject/Source/SkillProject/Data/SpyMissionConfig.h SkillProject/Source/SkillProject/Data/SpyMissionConfig.cpp SkillProject/Source/SkillProject/System/Tests/SpyMissionTests.cpp
```

제시할 메시지(안): `[Feature] USpyMissionConfig — Mission_TargetLocation 관계 테이블 추가`

---

### Task 2: `SpyMissionComponent` 표면 — `OnMissionAccepted` 델리게이트 + 목표 좌표 패스스루 + 클라이언트 안전성 수정

> ⚠ **game-designer 기획서(`docs/design/mission-ground-navigation.md` §2) 반영**: `AcceptCurrentMission()`/`ProcessProgress()` 안의 브로드캐스트만으로는 **데디케이티드 서버 + 원격 클라이언트 구성에서 `OnMissionAccepted`/`OnMissionCompleted` 가 원격 클라이언트에 영원히 도달하지 않는다** — 이 두 함수는 `HasAuthority()` 경로에서만 실행되는 일반 델리게이트 호출이라 서버 프로세스의 그 인스턴스에서만 리스너가 실행되고, 레플리케이트되지 않는다(1인 PIE 에서만 우연히 정상 동작). 이 태스크는 원래의 Step 1~5(직접 브로드캐스트)에 더해 **Step 6~9(`OnRep_MissionState` 를 통한 클라이언트측 상태-diff 브로드캐스트)** 를 반드시 포함한다 — 기획서 §2-3 규칙 그대로.

**Files:**
- Modify: `SkillProject/Source/SkillProject/System/SpyMissionComponent.h`
- Modify: `SkillProject/Source/SkillProject/System/SpyMissionComponent.cpp`
- Test: `SkillProject/Source/SkillProject/System/Tests/SpyMissionComponentTests.cpp` (신규)
- Create: `SkillProject/Source/SkillProject/System/Tests/SpyMissionComponentTestListener.h` (신규 — 동적 멀티캐스트 델리게이트 발화 검증용 UFUNCTION 리스너)
- Create: `SkillProject/Source/SkillProject/System/Tests/SpyMissionComponentTestListener.cpp` (신규)

**Interfaces:**
- Consumes: Task 1의 `USpyMissionConfig::GetMissionTargetLocation(int32) const`, `FSpyMission_TargetLocationRow`.
- Produces: `DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSpyMission_Accepted, USpyMissionComponent*, MissionComponent, int32, MissionIndex)`, `UPROPERTY(BlueprintAssignable) FSpyMission_Accepted OnMissionAccepted;`, `const FSpyMission_TargetLocationRow* USpyMissionComponent::GetMissionTargetLocation(int32 InMissionId) const`, `void USpyMissionComponent::OnRep_MissionState(FSpyMissionState OldMissionState)`(시그니처 변경) — Task 4가 전부 소비한다.

- [ ] **Step 1: 델리게이트 선언 + 패스스루 스텁 추가**

`SpyMissionComponent.h` 상단, 기존 델리게이트 선언(15~17행) 중 `FSpyMission_AllCompleted` 바로 뒤에 추가:

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSpyMission_Accepted, USpyMissionComponent*, MissionComponent, int32, MissionIndex);
```

같은 파일 상단 forward-declare 목록(11~13행)에 추가:

```cpp
struct FSpyMission_TargetLocationRow;
```

`OnAllMissionsCompleted` UPROPERTY(기존 128~129행) 바로 뒤에 추가:

```cpp
	UPROPERTY(BlueprintAssignable)
	FSpyMission_Accepted OnMissionAccepted;
```

`GetMissionEntry` 선언(기존 84~86행) 바로 뒤에 추가:

```cpp
	//# 인덱스로 목표 좌표를 조회한다(§14-1 선택적 관계). 없으면 nullptr — 데이터는 MissionConfig
	//# 가 소유하고 이 컴포넌트는 위임만 한다(cpp-style §8), GetMissionEntry 와 동일한 패턴
	const FSpyMission_TargetLocationRow* GetMissionTargetLocation(int32 InMissionId) const;
```

`SpyMissionComponent.cpp` 에 스텁 추가(파일 어디든, `GetMissionEntry` 구현부 근처):

```cpp
const FSpyMission_TargetLocationRow* USpyMissionComponent::GetMissionTargetLocation(int32 InMissionId) const
{
	return nullptr;
}
```

- [ ] **Step 2: 실패하는 테스트 작성**

신규 파일 `SkillProject/Source/SkillProject/System/Tests/SpyMissionComponentTests.cpp`:

```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Data/SpyMissionConfig.h"
#include "Engine/DataTable.h"
#include "System/SpyMissionComponent.h"
#include "UObject/UnrealType.h"
#include "Util/SpyGameplayTags.h"

//# MissionConfig 는 protected UPROPERTY 라 프로덕션 코드에 테스트 전용 setter 를 추가하지
//# 않기 위해 리플렉션으로 직접 주입한다 (production API 오염 방지, cpp-style §15 의 "코드
//# 전용" 취지와 동일하게 테스트 접근도 프로덕션 표면을 늘리지 않는다)
static void SpyMissionComponentTests_SetMissionConfig(USpyMissionComponent* Component, USpyMissionConfig* Config)
{
	FObjectProperty* Prop = FindFProperty<FObjectProperty>(USpyMissionComponent::StaticClass(), TEXT("MissionConfig"));
	check(Prop != nullptr);
	Prop->SetObjectPropertyValue_InContainer(Component, Config);
}

static USpyMissionConfig* SpyMissionComponentTests_MakeConfigWithTargetLocation()
{
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	UDataTable* MissionTable = NewObject<UDataTable>();
	MissionTable->RowStruct = FSpyMissionRow::StaticStruct();

	FSpyMissionRow Row;
	Row.MissionId = 1;
	Row.MissionType = ESpyMissionType::Gameplay;
	Row.MatchTag = SpyGameplayTags::Skill_Move_Vault;
	Row.Mode = ESpyMissionMode::Accumulate;
	Row.TargetCount = 3;
	MissionTable->AddRow(TEXT("Mission_1"), Row);
	Config->MissionTable = MissionTable;

	UDataTable* TargetLocationTable = NewObject<UDataTable>();
	TargetLocationTable->RowStruct = FSpyMission_TargetLocationRow::StaticStruct();

	FSpyMission_TargetLocationRow TargetRow;
	TargetRow.MissionId = 1;
	TargetRow.TargetLocation = FVector(500.f, 0.f, 0.f);
	TargetLocationTable->AddRow(TEXT("TargetLocation_1"), TargetRow);
	Config->MissionTargetLocationTable = TargetLocationTable;

	return Config;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionComponentTargetLocationPassthroughTest,
	"SkillProject.System.MissionComponent.TargetLocationPassthrough",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionComponentTargetLocationPassthroughTest::RunTest(const FString& Parameters)
{
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* Component = NewObject<USpyMissionComponent>(Owner);
	SpyMissionComponentTests_SetMissionConfig(Component, SpyMissionComponentTests_MakeConfigWithTargetLocation());

	const FSpyMission_TargetLocationRow* Row = Component->GetMissionTargetLocation(1);

	if (Row == nullptr)
	{
		AddError(TEXT("Expected a target location row for MissionId 1"));

		return false;
	}

	TestEqual(TEXT("Target location matches"), Row->TargetLocation, FVector(500.f, 0.f, 0.f));
	TestNull(TEXT("No target location for MissionId 2"), Component->GetMissionTargetLocation(2));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionComponentAcceptIdempotentTest,
	"SkillProject.System.MissionComponent.AcceptIdempotent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionComponentAcceptIdempotentTest::RunTest(const FString& Parameters)
{
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* Component = NewObject<USpyMissionComponent>(Owner);
	SpyMissionComponentTests_SetMissionConfig(Component, SpyMissionComponentTests_MakeConfigWithTargetLocation());

	TestFalse(TEXT("Not accepted initially"), Component->IsCurrentAccepted());

	const bool bFirstAccept = Component->AcceptCurrentMission();
	TestTrue(TEXT("First accept succeeds"), bFirstAccept);
	TestTrue(TEXT("Now accepted"), Component->IsCurrentAccepted());

	const bool bSecondAccept = Component->AcceptCurrentMission();
	TestTrue(TEXT("Second accept is idempotent (still true)"), bSecondAccept);

	return true;
}

#endif //# WITH_DEV_AUTOMATION_TESTS
```

- [ ] **Step 3: 테스트 실행 — 실패 확인**

컴파일 후 Automation 탭에서 `SkillProject.System.MissionComponent` 필터로 두 테스트 실행.
Expected: `TargetLocationPassthrough` FAIL(스텁이 `nullptr` 만 반환, `Row == nullptr` 분기로 `AddError`). `AcceptIdempotent` 는 이미 PASS(기존 `AcceptCurrentMission` 로직은 이번 변경과 무관하게 이미 멱등) — 이 케이스는 회귀 고정용이다.

- [ ] **Step 4: 실제 구현 — 델리게이트 브로드캐스트 + 패스스루**

`SpyMissionComponent.cpp` 의 `GetMissionTargetLocation` 스텁을 교체:

```cpp
const FSpyMission_TargetLocationRow* USpyMissionComponent::GetMissionTargetLocation(int32 InMissionId) const
{
	return (MissionConfig != nullptr ? MissionConfig->GetMissionTargetLocation(InMissionId) : nullptr);
}
```

`AcceptCurrentMission()` 안, `OnMissionProgressChanged.Broadcast(...)` 호출(기존 203행) 바로 뒤에 추가:

```cpp
	//# 수락 사실 전용 신호 — 진행값이 바뀔 때마다 도는 OnMissionProgressChanged 와 달리
	//# "새 미션을 따라가기 시작해야 한다"를 아는 구독자(내비게이션 등)를 위한 전용 델리게이트
	OnMissionAccepted.Broadcast(this, MissionState.MissionIndex);
```

`ProcessProgress()` 안, `MissionState.bAccepted = (...)` 대입(기존 179~180행) 바로 뒤·`OnMissionProgressChanged.Broadcast(...)`(기존 182행) 바로 앞에 추가:

```cpp
	//# 새 미션이 Dialogue/Interact 타입이라 자동 수락됐다면 그 사실도 전용 델리게이트로 알린다
	if (MissionState.bAccepted)
		OnMissionAccepted.Broadcast(this, MissionState.MissionIndex);
```

- [ ] **Step 5: 테스트 실행 — 통과 확인**

Step 3 과 동일 절차 재실행. Expected: 두 테스트 모두 PASS.

- [ ] **Step 6: 클라이언트 안전성 결함 — 리스너 헬퍼 + 실패하는 테스트 추가**

동적 멀티캐스트 델리게이트(`DECLARE_DYNAMIC_MULTICAST_DELEGATE_*`)는 `AddDynamic` 바인딩 대상이 `UFUNCTION` 을 가진 `UObject` 여야 해서 람다로 검증할 수 없다. 테스트 전용 리스너를 신설한다 — 프로덕션 표면(`USpyMissionComponent` 자체)에는 아무 것도 추가하지 않는다.

`SpyMissionComponentTestListener.h` (신규):

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "SpyMissionComponentTestListener.generated.h"

class USpyMissionComponent;

//# OnMissionAccepted/OnMissionCompleted 발화 횟수·인자를 기록하는 테스트 전용 리스너.
//# 동적 멀티캐스트 델리게이트는 UFUNCTION 바인딩 대상이 필요해 순수 람다로 검증할 수 없다.
UCLASS()
class USpyMissionComponentTestListener : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void HandleMissionAccepted(USpyMissionComponent* MissionComponent, int32 MissionIndex);

	UFUNCTION()
	void HandleMissionCompleted(USpyMissionComponent* MissionComponent, int32 CompletedIndex);

	int32 AcceptedCallCount = 0;
	int32 LastAcceptedIndex = -1;
	int32 AcceptedOrder = -1; //# NextOrder 스냅샷 — Completed 와의 상대 순서를 검증하기 위함(§2-3 순서 고정 회귀)

	int32 CompletedCallCount = 0;
	int32 LastCompletedIndex = -1;
	int32 CompletedOrder = -1;

	int32 NextOrder = 0;
};
```

`SpyMissionComponentTestListener.cpp` (신규):

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "System/Tests/SpyMissionComponentTestListener.h"

void USpyMissionComponentTestListener::HandleMissionAccepted(USpyMissionComponent* MissionComponent, int32 MissionIndex)
{
	++AcceptedCallCount;
	LastAcceptedIndex = MissionIndex;
	AcceptedOrder = NextOrder++;
}

void USpyMissionComponentTestListener::HandleMissionCompleted(USpyMissionComponent* MissionComponent, int32 CompletedIndex)
{
	++CompletedCallCount;
	LastCompletedIndex = CompletedIndex;
	CompletedOrder = NextOrder++;
}
```

`SpyMissionComponentTests.cpp` 상단 include 에 추가:

```cpp
#include "System/Tests/SpyMissionComponentTestListener.h"
```

파일 끝(`#endif` 바로 앞)에 리플렉션 기반 `OnRep_MissionState` 직접 호출 헬퍼 + 테스트 두 개 추가:

```cpp
//# OnRep_MissionState 는 protected UFUNCTION 이라 리플렉션으로 직접 호출한다 — 실제 네트워크
//# 레플리케이션 없이 "원격 클라이언트가 새 MissionState 를 받았다"를 시뮬레이션하는 유일한 방법.
//# UE 표준 RepNotify 파라미터 마샬링과 동일한 파라미터 구조체 레이아웃을 쓴다.
static void SpyMissionComponentTests_SimulateReplication(USpyMissionComponent* Component, const FSpyMissionState& OldState)
{
	UFunction* Func = USpyMissionComponent::StaticClass()->FindFunctionByName(TEXT("OnRep_MissionState"));
	check(Func != nullptr);

	struct FOnRepMissionStateParms
	{
		FSpyMissionState OldMissionState;
	};

	FOnRepMissionStateParms Parms;
	Parms.OldMissionState = OldState;

	Component->ProcessEvent(Func, &Parms);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionComponentRemoteClientAcceptedTest,
	"SkillProject.System.MissionComponent.RemoteClientAccepted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionComponentRemoteClientAcceptedTest::RunTest(const FString& Parameters)
{
	//# 원격 클라이언트 시뮬레이션 — AcceptCurrentMission() 을 절대 호출하지 않는다(서버 전용
	//# 경로). OnRep_MissionState 만으로 OnMissionAccepted 가 발화해야 한다(design §2-3)
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* Component = NewObject<USpyMissionComponent>(Owner);
	SpyMissionComponentTests_SetMissionConfig(Component, SpyMissionComponentTests_MakeConfigWithTargetLocation());

	USpyMissionComponentTestListener* Listener = NewObject<USpyMissionComponentTestListener>();
	Component->OnMissionAccepted.AddDynamic(Listener, &USpyMissionComponentTestListener::HandleMissionAccepted);

	const FSpyMissionState OldState; //# 기본값 — MissionIndex=1, bAccepted=false (서버가 레플리케이트하기 전 초기 상태)

	//# "서버가 방금 미션 1을 수락 상태로 레플리케이트했다"를 시뮬레이션 — 리플렉션으로
	//# MissionState 를 먼저 갱신한 뒤(실제 네트워크는 이 갱신을 RepNotify 호출 전에 끝낸다)
	FStructProperty* MissionStateProp = FindFProperty<FStructProperty>(USpyMissionComponent::StaticClass(), TEXT("MissionState"));
	check(MissionStateProp != nullptr);

	FSpyMissionState NewState;
	NewState.MissionIndex = 1;
	NewState.Count = 0;
	NewState.bAccepted = true;
	MissionStateProp->SetValue_InContainer(Component, &NewState);

	SpyMissionComponentTests_SimulateReplication(Component, OldState);

	TestEqual(TEXT("OnMissionAccepted fired exactly once via OnRep"), Listener->AcceptedCallCount, 1);
	TestEqual(TEXT("Accepted index matches"), Listener->LastAcceptedIndex, 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionComponentRemoteClientCompletedTest,
	"SkillProject.System.MissionComponent.RemoteClientCompleted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionComponentRemoteClientCompletedTest::RunTest(const FString& Parameters)
{
	//# 미션 1(수락됨) -> 미션 2 로 인덱스가 넘어간 스냅샷을 시뮬레이션 — 완료된 인덱스(1)로
	//# OnMissionCompleted 가 발화해야 한다(design §2-3 "완료된 인덱스는 이전 값이다")
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* Component = NewObject<USpyMissionComponent>(Owner);
	SpyMissionComponentTests_SetMissionConfig(Component, SpyMissionComponentTests_MakeConfigWithTargetLocation());

	USpyMissionComponentTestListener* Listener = NewObject<USpyMissionComponentTestListener>();
	Component->OnMissionCompleted.AddDynamic(Listener, &USpyMissionComponentTestListener::HandleMissionCompleted);

	FSpyMissionState OldState;
	OldState.MissionIndex = 1;
	OldState.Count = 3;
	OldState.bAccepted = true;

	FStructProperty* MissionStateProp = FindFProperty<FStructProperty>(USpyMissionComponent::StaticClass(), TEXT("MissionState"));
	check(MissionStateProp != nullptr);

	FSpyMissionState NewState;
	NewState.MissionIndex = 2;
	NewState.Count = 0;
	NewState.bAccepted = false;
	MissionStateProp->SetValue_InContainer(Component, &NewState);

	SpyMissionComponentTests_SimulateReplication(Component, OldState);

	TestEqual(TEXT("OnMissionCompleted fired exactly once via OnRep"), Listener->CompletedCallCount, 1);
	TestEqual(TEXT("Completed index is the OLD index (1), not the new one"), Listener->LastCompletedIndex, 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyMissionComponentRemoteClientOrderTest,
	"SkillProject.System.MissionComponent.RemoteClientCompletedBeforeAccepted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyMissionComponentRemoteClientOrderTest::RunTest(const FString& Parameters)
{
	//# design §2-3(3차 개정) 회귀 고정 — 완료(1)와 자동수락(2, Dialogue 타입)이 같은 OnRep
	//# 호출 한 번에 동시에 일어날 때, Completed 가 Accepted 보다 반드시 먼저 발화해야 한다.
	//# 뒤집히면 USpyNavigationComponent::HandleMissionCompleted(무조건 StopPath) 가 같은
	//# 프레임에 막 시작된 새 경로를 지워버린다(design-reviewer 1차검토 BLOCKER)
	USpyMissionConfig* Config = SpyMissionComponentTests_MakeConfigWithTargetLocation();

	UDataTable* SecondMissionTable = Config->MissionTable;
	FSpyMissionRow DialogueRow;
	DialogueRow.MissionId = 2;
	DialogueRow.MissionType = ESpyMissionType::Dialogue;
	DialogueRow.MatchTag = SpyGameplayTags::Event_Mission_Report;
	SecondMissionTable->AddRow(TEXT("Mission_2"), DialogueRow);

	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* Component = NewObject<USpyMissionComponent>(Owner);
	SpyMissionComponentTests_SetMissionConfig(Component, Config);

	USpyMissionComponentTestListener* Listener = NewObject<USpyMissionComponentTestListener>();
	Component->OnMissionAccepted.AddDynamic(Listener, &USpyMissionComponentTestListener::HandleMissionAccepted);
	Component->OnMissionCompleted.AddDynamic(Listener, &USpyMissionComponentTestListener::HandleMissionCompleted);

	//# 미션 1(수락됨, 완료 직전) -> 미션 2(Dialogue, 진입과 동시에 자동수락) 로 한 번에 전이
	FSpyMissionState OldState;
	OldState.MissionIndex = 1;
	OldState.Count = 3;
	OldState.bAccepted = true;

	FStructProperty* MissionStateProp = FindFProperty<FStructProperty>(USpyMissionComponent::StaticClass(), TEXT("MissionState"));
	check(MissionStateProp != nullptr);

	FSpyMissionState NewState;
	NewState.MissionIndex = 2;
	NewState.Count = 0;
	NewState.bAccepted = true;
	MissionStateProp->SetValue_InContainer(Component, &NewState);

	SpyMissionComponentTests_SimulateReplication(Component, OldState);

	TestEqual(TEXT("Both fired exactly once"), Listener->CompletedCallCount, 1);
	TestEqual(TEXT("Both fired exactly once (accepted)"), Listener->AcceptedCallCount, 1);
	TestTrue(TEXT("Completed fired strictly before Accepted"), Listener->CompletedOrder < Listener->AcceptedOrder);

	return true;
}
```

- [ ] **Step 7: 테스트 실행 — 실패 확인**

컴파일 후 `SkillProject.System.MissionComponent.RemoteClient` 필터로 세 테스트(Accepted/Completed/CompletedBeforeAccepted) 실행.
Expected: 셋 다 FAIL — 현재 `OnRep_MissionState()` 는 파라미터가 없어 `FindFunctionByName` 이 찾은 함수의 파라미터 구조와 테스트의 `FOnRepMissionStateParms` 가 안 맞아 컴파일 단계에서부터 어긋난다(이 자체가 "아직 시그니처가 안 바뀌었다"는 RED 신호). Step 8 이전에는 이 스텝이 정상적으로 컴파일되지 않을 수 있다 — C++ 특성상 이 태스크는 "컴파일 실패가 RED" 케이스다(Task 1 Global Constraints 참고).

- [ ] **Step 8: 실제 구현 — `OnRep_MissionState` 확장**

`SpyMissionComponent.h` 의 `OnRep_MissionState` 선언(기존 118~119행)을 교체:

```cpp
	UFUNCTION()
	void OnRep_MissionState(FSpyMissionState OldMissionState);
```

`SpyMissionComponent.cpp` 의 `OnRep_MissionState` 구현(기존 91~100행)을 교체:

```cpp
void USpyMissionComponent::OnRep_MissionState(FSpyMissionState OldMissionState)
{
	//# 클라이언트 표시 갱신
	OnMissionProgressChanged.Broadcast(this, MissionState.MissionIndex, MissionState.Count, GetTargetCount());

	//# 원격 클라이언트에서는 AcceptCurrentMission()/ProcessProgress() 가 전혀 실행되지
	//# 않는다(서버 권한 가드) — 그 안의 OnMissionAccepted/OnMissionCompleted 브로드캐스트는
	//# 리슨 서버 호스트에서만 도달한다. 여기서 상태-diff 로 같은 신호를 재현한다(design §2-3)
	//#
	//# ⚠ 순서 고정 — Completed 를 Accepted 보다 반드시 먼저 판정한다. ProcessProgress()(서버
	//# 권한 경로, :153-186)도 이 순서다. 뒤집으면 USpyNavigationComponent::HandleMissionCompleted
	//# (인덱스 무관 무조건 StopPath)가 같은 호출 안에서 막 시작된 새 경로를 즉시 지운다 —
	//# Dialogue 자동수락 전이(§5-2 MissionId 2/4/6/8/10/12)마다 매번 재현되는 BLOCKER 였다
	//# (design §2-3 3차 개정, design-reviewer 1차검토에서 발견)
	if (OldMissionState.MissionIndex != MissionState.MissionIndex)
		OnMissionCompleted.Broadcast(this, OldMissionState.MissionIndex);

	const bool bAcceptedTransition = (OldMissionState.bAccepted == false && MissionState.bAccepted == true) ||
		(OldMissionState.MissionIndex != MissionState.MissionIndex && MissionState.bAccepted == true);

	if (bAcceptedTransition)
		OnMissionAccepted.Broadcast(this, MissionState.MissionIndex);

	if (IsAllCompleted())
	{
		OnAllMissionsCompleted.Broadcast(this);
	}
}
```

- [ ] **Step 9: 테스트 실행 — 통과 확인**

Step 7 과 동일 절차 재실행. Expected: `SkillProject.System.MissionComponent` 전체(5개 테스트) 모두 PASS.

- [ ] **Step 10: 스테이징 + 커밋 메시지(안) 제시**

```bash
git add SkillProject/Source/SkillProject/System/SpyMissionComponent.h SkillProject/Source/SkillProject/System/SpyMissionComponent.cpp SkillProject/Source/SkillProject/System/Tests/SpyMissionComponentTests.cpp SkillProject/Source/SkillProject/System/Tests/SpyMissionComponentTestListener.h SkillProject/Source/SkillProject/System/Tests/SpyMissionComponentTestListener.cpp
```

제시할 메시지(안): `[Fix] USpyMissionComponent — OnMissionAccepted 델리게이트 추가 + 원격 클라이언트 트리거 결함 수정`

---

### Task 3: `SpyNavPathMath::BuildSplineSegments` 순수 헬퍼

**Files:**
- Create: `SkillProject/Source/SkillProject/System/SpyNavPathMath.h`
- Create: `SkillProject/Source/SkillProject/System/SpyNavPathMath.cpp`
- Test: `SkillProject/Source/SkillProject/System/Tests/SpyNavPathMathTests.cpp`

**Interfaces:**
- Produces: `namespace SpyNavPathMath { SKILLPROJECT_API TArray<TPair<FVector, FVector>> BuildSplineSegments(const TArray<FVector>& PathPoints); }` — Task 5가 소비한다(세그먼트 시작/끝 좌표 쌍 → `USplineMeshComponent::SetStartAndEnd`).

- [ ] **Step 1: 헤더 + 스텁 작성**

`SpyNavPathMath.h` (신규, `SpyHUDMath.h` 패턴 그대로 미러링):

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

//# 내비게이션 경로 표현 로직 순수함수 모음 — 컴포넌트/월드 없이 테스트 가능하게 분리한다
namespace SpyNavPathMath {
//# 경로점을 세그먼트 시작/끝 좌표 쌍으로 변환한다. 점이 2개 미만이면 빈 배열.
SKILLPROJECT_API TArray<TPair<FVector, FVector>> BuildSplineSegments(const TArray<FVector>& PathPoints);
} //namespace SpyNavPathMath
```

`SpyNavPathMath.cpp` (신규, 스텁):

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "System/SpyNavPathMath.h"

TArray<TPair<FVector, FVector>> SpyNavPathMath::BuildSplineSegments(const TArray<FVector>& PathPoints)
{
	return TArray<TPair<FVector, FVector>>();
}
```

- [ ] **Step 2: 실패하는 테스트 작성**

`SpyNavPathMathTests.cpp` (신규):

```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "System/SpyNavPathMath.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavPathMathBuildSegmentsTest,
	"SkillProject.Navigation.Math.BuildSplineSegments",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavPathMathBuildSegmentsTest::RunTest(const FString& Parameters)
{
	TArray<FVector> PathPoints;
	PathPoints.Add(FVector(0.f, 0.f, 0.f));
	PathPoints.Add(FVector(100.f, 0.f, 0.f));
	PathPoints.Add(FVector(100.f, 100.f, 0.f));

	const TArray<TPair<FVector, FVector>> Segments = SpyNavPathMath::BuildSplineSegments(PathPoints);

	TestEqual(TEXT("3 points produce 2 segments"), Segments.Num(), 2);

	if (Segments.Num() == 2)
	{
		TestEqual(TEXT("Segment 0 start"), Segments[0].Key, FVector(0.f, 0.f, 0.f));
		TestEqual(TEXT("Segment 0 end"), Segments[0].Value, FVector(100.f, 0.f, 0.f));
		TestEqual(TEXT("Segment 1 start"), Segments[1].Key, FVector(100.f, 0.f, 0.f));
		TestEqual(TEXT("Segment 1 end"), Segments[1].Value, FVector(100.f, 100.f, 0.f));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavPathMathBuildSegmentsTooFewPointsTest,
	"SkillProject.Navigation.Math.BuildSplineSegmentsTooFewPoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavPathMathBuildSegmentsTooFewPointsTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Empty input"), SpyNavPathMath::BuildSplineSegments(TArray<FVector>()).Num(), 0);

	TArray<FVector> SinglePoint;
	SinglePoint.Add(FVector::ZeroVector);
	TestEqual(TEXT("Single point produces no segments"), SpyNavPathMath::BuildSplineSegments(SinglePoint).Num(), 0);

	return true;
}

#endif //# WITH_DEV_AUTOMATION_TESTS
```

- [ ] **Step 3: 테스트 실행 — 실패 확인**

컴파일 후 Automation 탭에서 `SkillProject.Navigation.Math` 필터로 실행.
Expected: `BuildSplineSegments` FAIL(스텁이 항상 빈 배열, `Segments.Num()` 이 0으로 나와 `TestEqual(..., 2)` 실패). `BuildSplineSegmentsTooFewPoints` 는 이미 PASS(우연히 스텁과 일치).

- [ ] **Step 4: 실제 구현**

```cpp
TArray<TPair<FVector, FVector>> SpyNavPathMath::BuildSplineSegments(const TArray<FVector>& PathPoints)
{
	TArray<TPair<FVector, FVector>> Segments;

	if (PathPoints.Num() < 2)
		return Segments;

	for (int32 Index = 0; Index < PathPoints.Num() - 1; ++Index)
	{
		Segments.Add(TPair<FVector, FVector>(PathPoints[Index], PathPoints[Index + 1]));
	}

	return Segments;
}
```

- [ ] **Step 5: 테스트 실행 — 통과 확인**

Step 3 과 동일 절차 재실행. Expected: 두 테스트 모두 PASS.

- [ ] **Step 6: §4-3 가시성 상태 머신용 순수 함수 3종 — 실패하는 테스트 추가**

> game-designer 기획서(`docs/design/mission-ground-navigation.md` §7-5 항목3) 반영 — 발밑 시야 가림 방지(시작 오프셋 트리밍)와 도착 임계값 히스테리시스를 렌더 경로에 직접 박지 않고 `SpyNavPathMath`(순수 함수, 테스트 가능)에 둔다.

`SpyNavPathMath.h` 에 세 함수를 추가:

```cpp
//# 경로점 배열에서 시작(플레이어 위치)으로부터 TrimDistanceCm 만큼을 잘라낸 새 배열을 만든다.
//# 발밑 시야 가림 방지(§4-1) — 잘라낸 지점은 원래 세그먼트를 보간해 정확히 TrimDistanceCm 위치에 놓는다.
//# 점이 2개 미만이거나 TrimDistanceCm<=0 이면 원본을 그대로 반환한다.
SKILLPROJECT_API TArray<FVector> TrimLeadingDistance(const TArray<FVector>& PathPoints, float TrimDistanceCm);

//# 경로점을 잇는 총 길이(cm). 점이 2개 미만이면 0.
SKILLPROJECT_API float ComputePathLength(const TArray<FVector>& PathPoints);

//# 도착 임계값 히스테리시스(§4-3) — 보이는 상태(bPreviouslyVisible=true)에서는 길이가
//# HideThresholdCm 아래로 내려가야 숨고, 숨은 상태에서는 ReshowThresholdCm 위로 올라가야
//# 다시 보인다. 두 임계값 사이(히스테리시스 밴드)에서는 이전 상태를 유지한다.
SKILLPROJECT_API bool EvaluateHysteresisVisibility(float PathLengthCm, float HideThresholdCm, float ReshowThresholdCm, bool bPreviouslyVisible);
```

`SpyNavPathMathTests.cpp` 의 `#endif` 바로 앞에 테스트 4개 추가:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavPathMathTrimLeadingDistanceTest,
	"SkillProject.Navigation.Math.TrimLeadingDistance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavPathMathTrimLeadingDistanceTest::RunTest(const FString& Parameters)
{
	TArray<FVector> PathPoints;
	PathPoints.Add(FVector(0.f, 0.f, 0.f));
	PathPoints.Add(FVector(200.f, 0.f, 0.f));

	const TArray<FVector> Trimmed = SpyNavPathMath::TrimLeadingDistance(PathPoints, 100.f);

	TestEqual(TEXT("Trimmed keeps 2 points"), Trimmed.Num(), 2);
	if (Trimmed.Num() == 2)
	{
		TestEqual(TEXT("Start moved forward by 100cm"), Trimmed[0], FVector(100.f, 0.f, 0.f));
		TestEqual(TEXT("End unchanged"), Trimmed[1], FVector(200.f, 0.f, 0.f));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavPathMathTrimLeadingDistanceExceedsPathTest,
	"SkillProject.Navigation.Math.TrimLeadingDistanceExceedsPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavPathMathTrimLeadingDistanceExceedsPathTest::RunTest(const FString& Parameters)
{
	//# 트림 거리(500)가 전체 경로 길이(200)보다 길면 마지막 점 하나만 남는다(도착 직전)
	TArray<FVector> PathPoints;
	PathPoints.Add(FVector(0.f, 0.f, 0.f));
	PathPoints.Add(FVector(200.f, 0.f, 0.f));

	const TArray<FVector> Trimmed = SpyNavPathMath::TrimLeadingDistance(PathPoints, 500.f);

	TestEqual(TEXT("Only the last point remains"), Trimmed.Num(), 1);
	if (Trimmed.Num() == 1)
		TestEqual(TEXT("Last point is the target"), Trimmed[0], FVector(200.f, 0.f, 0.f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavPathMathComputePathLengthTest,
	"SkillProject.Navigation.Math.ComputePathLength",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavPathMathComputePathLengthTest::RunTest(const FString& Parameters)
{
	TArray<FVector> PathPoints;
	PathPoints.Add(FVector(0.f, 0.f, 0.f));
	PathPoints.Add(FVector(100.f, 0.f, 0.f));
	PathPoints.Add(FVector(100.f, 100.f, 0.f));

	TestEqual(TEXT("Length is 100+100=200"), SpyNavPathMath::ComputePathLength(PathPoints), 200.f);
	TestEqual(TEXT("Empty path has 0 length"), SpyNavPathMath::ComputePathLength(TArray<FVector>()), 0.f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavPathMathHysteresisVisibilityTest,
	"SkillProject.Navigation.Math.HysteresisVisibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavPathMathHysteresisVisibilityTest::RunTest(const FString& Parameters)
{
	//# design §4-3 확정값: Hide=300, Reshow=400
	TestTrue(TEXT("Far away + was visible -> stays visible"), SpyNavPathMath::EvaluateHysteresisVisibility(1000.f, 300.f, 400.f, true));
	TestFalse(TEXT("Close + was visible -> hides"), SpyNavPathMath::EvaluateHysteresisVisibility(200.f, 300.f, 400.f, true));

	//# 밴드(300~400) 안에서는 "이전 상태 유지" — 보이던 상태였으면 계속 보임
	TestTrue(TEXT("Inside band + was visible -> stays visible"), SpyNavPathMath::EvaluateHysteresisVisibility(350.f, 300.f, 400.f, true));
	//# 밴드 안에서 숨어 있었으면 계속 숨음
	TestFalse(TEXT("Inside band + was hidden -> stays hidden"), SpyNavPathMath::EvaluateHysteresisVisibility(350.f, 300.f, 400.f, false));

	TestTrue(TEXT("Far away + was hidden -> reshows"), SpyNavPathMath::EvaluateHysteresisVisibility(1000.f, 300.f, 400.f, false));
	TestFalse(TEXT("Close + was hidden -> stays hidden"), SpyNavPathMath::EvaluateHysteresisVisibility(200.f, 300.f, 400.f, false));

	return true;
}
```

- [ ] **Step 7: 테스트 실행 — 실패 확인**

`SkillProject.Navigation.Math` 필터로 전체 실행. Expected: Step 6 에서 추가한 4개 신규 테스트 FAIL(함수가 아직 선언만 있고 정의가 없어 링크 에러 — Task 1 Global Constraints 의 컴파일 실패 RED 규칙), 기존 2개는 그대로 PASS.

- [ ] **Step 8: 실제 구현**

`SpyNavPathMath.cpp` 에 세 함수 구현 추가:

```cpp
TArray<FVector> SpyNavPathMath::TrimLeadingDistance(const TArray<FVector>& PathPoints, float TrimDistanceCm)
{
	if (PathPoints.Num() < 2 || TrimDistanceCm <= 0.f)
		return PathPoints;

	float RemainingTrim = TrimDistanceCm;
	int32 StartIndex = PathPoints.Num() - 1;
	FVector StartPoint = PathPoints.Last();

	for (int32 Index = 0; Index < PathPoints.Num() - 1; ++Index)
	{
		const float SegLength = FVector::Dist(PathPoints[Index], PathPoints[Index + 1]);

		if (SegLength >= RemainingTrim)
		{
			const float Alpha = (SegLength > 0.f ? RemainingTrim / SegLength : 0.f);
			StartPoint = FMath::Lerp(PathPoints[Index], PathPoints[Index + 1], Alpha);
			StartIndex = Index + 1;
			break;
		}

		RemainingTrim -= SegLength;
	}

	TArray<FVector> Trimmed;
	Trimmed.Add(StartPoint);

	for (int32 Index = StartIndex; Index < PathPoints.Num(); ++Index)
	{
		if (PathPoints[Index].Equals(StartPoint))
			continue;

		Trimmed.Add(PathPoints[Index]);
	}

	return Trimmed;
}

float SpyNavPathMath::ComputePathLength(const TArray<FVector>& PathPoints)
{
	float Length = 0.f;

	for (int32 Index = 0; Index < PathPoints.Num() - 1; ++Index)
	{
		Length += FVector::Dist(PathPoints[Index], PathPoints[Index + 1]);
	}

	return Length;
}

bool SpyNavPathMath::EvaluateHysteresisVisibility(float PathLengthCm, float HideThresholdCm, float ReshowThresholdCm, bool bPreviouslyVisible)
{
	if (bPreviouslyVisible)
		return (PathLengthCm >= HideThresholdCm);

	return (PathLengthCm >= ReshowThresholdCm);
}
```

- [ ] **Step 9: 테스트 실행 — 통과 확인**

Step 7 과 동일 절차 재실행. Expected: `SkillProject.Navigation.Math` 전체(6개 테스트) PASS.

- [ ] **Step 10: 스테이징 + 커밋 메시지(안) 제시**

```bash
git add SkillProject/Source/SkillProject/System/SpyNavPathMath.h SkillProject/Source/SkillProject/System/SpyNavPathMath.cpp SkillProject/Source/SkillProject/System/Tests/SpyNavPathMathTests.cpp
```

제시할 메시지(안): `[Feature] SpyNavPathMath — 경로점→스플라인 세그먼트 변환 + §4-3 가시성 히스테리시스 순수 함수 추가`

---

### Task 4: `USpyNavigationComponent` — 수락/완료 반응 상태 머신 (렌더링 제외)

**Files:**
- Create: `SkillProject/Source/SkillProject/ManagerComponent/SpyNavigationComponent.h`
- Create: `SkillProject/Source/SkillProject/ManagerComponent/SpyNavigationComponent.cpp`
- Test: `SkillProject/Source/SkillProject/ManagerComponent/Tests/SpyNavigationComponentTests.cpp` (신규 — `ManagerComponent/Tests/` 폴더도 이 태스크에서 처음 생김)

**Interfaces:**
- Consumes: Task 2의 `USpyMissionComponent::OnMissionAccepted`/`OnMissionCompleted`/`OnAllMissionsCompleted`, `GetMissionTargetLocation(int32) const`.
- Produces: `bool IsPathActive() const`, `FVector GetCurrentTargetLocation() const`, `void BindMissionComponent(USpyMissionComponent*)`, `void UnbindMissionComponent()` — Task 5가 `StartPathTo`/`StopPath`/`RecomputePath` 를 확장해 이 위에 렌더링을 얹는다.

- [ ] **Step 1: 헤더 + 최소 구현 작성**

`SpyNavigationComponent.h` (신규):

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "SpyNavigationComponent.generated.h"

class USpyMissionComponent;

//# 활성 미션의 목표 지점까지 바닥 글로우 라인으로 안내하는 로컬 클라이언트 전용 연출 컴포넌트.
//# 서버/타 플레이어에 레플리케이트하지 않는다 — 소유 폰이 로컬 컨트롤일 때만 동작한다.
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SKILLPROJECT_API USpyNavigationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USpyNavigationComponent();

	UFUNCTION(BlueprintPure)
	bool IsPathActive() const { return bPathActive; }

	UFUNCTION(BlueprintPure)
	FVector GetCurrentTargetLocation() const { return CurrentTargetLocation; }

	//# SpyMainHUD::TryBindMissionComponent 와 동일한 재시도 바인딩 흐름의 실제 바인딩 단계.
	//# 테스트에서도 컨트롤러/PlayerState 체인 없이 직접 호출한다(cpp-style §8 탐색 지양의
	//# 대안인 "명시적 주입"에 해당 — 프로덕션에서는 AutoDiscoverAndBindMissionComponent 가 호출)
	void BindMissionComponent(USpyMissionComponent* InMissionComponent);
	void UnbindMissionComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool AutoDiscoverAndBindMissionComponent();

	UFUNCTION()
	void HandleMissionAccepted(USpyMissionComponent* MissionComponent, int32 MissionIndex);

	UFUNCTION()
	void HandleMissionCompleted(USpyMissionComponent* MissionComponent, int32 CompletedIndex);

	UFUNCTION()
	void HandleAllMissionsCompleted(USpyMissionComponent* MissionComponent);

	void StartPathTo(const FVector& InTargetLocation);
	void StopPath();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	float UpdateIntervalSeconds = 0.75f;

	UPROPERTY(Transient)
	TObjectPtr<USpyMissionComponent> BoundMissionComponent;

	FTimerHandle BindRetryTimerHandle;

	FVector CurrentTargetLocation = FVector::ZeroVector;
	bool bPathActive = false;
};
```

`SpyNavigationComponent.cpp` (신규):

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "ManagerComponent/SpyNavigationComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "System/SpyMissionComponent.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyNavigationComponent)

USpyNavigationComponent::USpyNavigationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USpyNavigationComponent::BeginPlay()
{
	Super::BeginPlay();

	const APawn* OwningPawn = Cast<APawn>(GetOwner());
	if (OwningPawn == nullptr || OwningPawn->IsLocallyControlled() == false)
		return;

	if (AutoDiscoverAndBindMissionComponent())
		return;

	//# 클라이언트에서는 이 시점에 Controller/PlayerState 가 아직 없을 수 있다 —
	//# SpyMainHUD::TryBindMissionComponent 와 동일한 재시도 타이머 패턴
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(BindRetryTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]() {
			if (AutoDiscoverAndBindMissionComponent() == false)
				return;

			if (UWorld* InnerWorld = GetWorld())
				InnerWorld->GetTimerManager().ClearTimer(BindRetryTimerHandle);
		}), 0.2f, true);
	}
}

void USpyNavigationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(BindRetryTimerHandle);

	StopPath();
	UnbindMissionComponent();

	Super::EndPlay(EndPlayReason);
}

bool USpyNavigationComponent::AutoDiscoverAndBindMissionComponent()
{
	if (BoundMissionComponent != nullptr)
		return true;

	const APawn* OwningPawn = Cast<APawn>(GetOwner());
	if (OwningPawn == nullptr)
		return false;

	AController* OwningController = OwningPawn->GetController();
	if (OwningController == nullptr)
		return false;

	APlayerState* OwningState = OwningController->PlayerState;
	if (OwningState == nullptr)
		return false;

	USpyMissionComponent* MissionComponent = USpyMissionComponent::FindMissionComponent(OwningState);
	if (MissionComponent == nullptr)
		return false;

	BindMissionComponent(MissionComponent);

	return true;
}

void USpyNavigationComponent::BindMissionComponent(USpyMissionComponent* InMissionComponent)
{
	if (InMissionComponent == nullptr || InMissionComponent == BoundMissionComponent)
		return;

	UnbindMissionComponent();

	BoundMissionComponent = InMissionComponent;
	BoundMissionComponent->OnMissionAccepted.AddDynamic(this, &USpyNavigationComponent::HandleMissionAccepted);
	BoundMissionComponent->OnMissionCompleted.AddDynamic(this, &USpyNavigationComponent::HandleMissionCompleted);
	BoundMissionComponent->OnAllMissionsCompleted.AddDynamic(this, &USpyNavigationComponent::HandleAllMissionsCompleted);
}

void USpyNavigationComponent::UnbindMissionComponent()
{
	if (BoundMissionComponent == nullptr)
		return;

	BoundMissionComponent->OnMissionAccepted.RemoveDynamic(this, &USpyNavigationComponent::HandleMissionAccepted);
	BoundMissionComponent->OnMissionCompleted.RemoveDynamic(this, &USpyNavigationComponent::HandleMissionCompleted);
	BoundMissionComponent->OnAllMissionsCompleted.RemoveDynamic(this, &USpyNavigationComponent::HandleAllMissionsCompleted);

	BoundMissionComponent = nullptr;
}

void USpyNavigationComponent::HandleMissionAccepted(USpyMissionComponent* MissionComponent, int32 MissionIndex)
{
	if (BoundMissionComponent == nullptr)
		return;

	const FSpyMission_TargetLocationRow* TargetRow = BoundMissionComponent->GetMissionTargetLocation(MissionIndex);
	if (TargetRow == nullptr)
	{
		StopPath();

		return;
	}

	StartPathTo(TargetRow->TargetLocation);
}

void USpyNavigationComponent::HandleMissionCompleted(USpyMissionComponent* MissionComponent, int32 CompletedIndex)
{
	StopPath();
}

void USpyNavigationComponent::HandleAllMissionsCompleted(USpyMissionComponent* MissionComponent)
{
	StopPath();
}

void USpyNavigationComponent::StartPathTo(const FVector& InTargetLocation)
{
	CurrentTargetLocation = InTargetLocation;
	bPathActive = true;
}

void USpyNavigationComponent::StopPath()
{
	bPathActive = false;
	CurrentTargetLocation = FVector::ZeroVector;
}
```

- [ ] **Step 2: 테스트 작성**

`SpyNavigationComponentTests.cpp` (신규, `ManagerComponent/Tests/` 폴더 신규 생성):

```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Data/SpyMissionConfig.h"
#include "Engine/DataTable.h"
#include "ManagerComponent/SpyNavigationComponent.h"
#include "System/SpyMissionComponent.h"
#include "UObject/UnrealType.h"
#include "Util/SpyGameplayTags.h"

//# System/Tests/SpyMissionComponentTests.cpp 의 동일 헬퍼를 이 파일 전용으로 다시 둔다 —
//# 기존 테스트 파일들도 픽스처 헬퍼를 파일마다 static 으로 독립 소유한다(SpyMissionTests.cpp 선례)
static void SpyNavigationComponentTests_SetMissionConfig(USpyMissionComponent* Component, USpyMissionConfig* Config)
{
	FObjectProperty* Prop = FindFProperty<FObjectProperty>(USpyMissionComponent::StaticClass(), TEXT("MissionConfig"));
	check(Prop != nullptr);
	Prop->SetObjectPropertyValue_InContainer(Component, Config);
}

static USpyMissionConfig* SpyNavigationComponentTests_MakeConfigWithTargetLocation()
{
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	UDataTable* MissionTable = NewObject<UDataTable>();
	MissionTable->RowStruct = FSpyMissionRow::StaticStruct();

	FSpyMissionRow Row;
	Row.MissionId = 1;
	Row.MissionType = ESpyMissionType::Gameplay;
	Row.MatchTag = SpyGameplayTags::Skill_Move_Vault;
	Row.Mode = ESpyMissionMode::Accumulate;
	Row.TargetCount = 3;
	MissionTable->AddRow(TEXT("Mission_1"), Row);
	Config->MissionTable = MissionTable;

	UDataTable* TargetLocationTable = NewObject<UDataTable>();
	TargetLocationTable->RowStruct = FSpyMission_TargetLocationRow::StaticStruct();

	FSpyMission_TargetLocationRow TargetRow;
	TargetRow.MissionId = 1;
	TargetRow.TargetLocation = FVector(500.f, 0.f, 0.f);
	TargetLocationTable->AddRow(TEXT("TargetLocation_1"), TargetRow);
	Config->MissionTargetLocationTable = TargetLocationTable;

	return Config;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentAcceptStartsPathTest,
	"SkillProject.Navigation.Component.AcceptStartsPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentAcceptStartsPathTest::RunTest(const FString& Parameters)
{
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, SpyNavigationComponentTests_MakeConfigWithTargetLocation());

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);

	TestFalse(TEXT("Path inactive before accept"), NavComponent->IsPathActive());

	MissionComponent->AcceptCurrentMission();

	TestTrue(TEXT("Path active after accept"), NavComponent->IsPathActive());
	TestEqual(TEXT("Target location matches Mission_TargetLocation row"), NavComponent->GetCurrentTargetLocation(), FVector(500.f, 0.f, 0.f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentNoTargetLocationTest,
	"SkillProject.Navigation.Component.NoTargetLocation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentNoTargetLocationTest::RunTest(const FString& Parameters)
{
	//# 목표 좌표가 정의되지 않은 미션은 수락해도 경로가 활성화되지 않는다 (spec 범위 §2 "제외")
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);

	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();
	UDataTable* MissionTable = NewObject<UDataTable>();
	MissionTable->RowStruct = FSpyMissionRow::StaticStruct();

	FSpyMissionRow Row;
	Row.MissionId = 1;
	Row.MissionType = ESpyMissionType::Gameplay;
	Row.MatchTag = SpyGameplayTags::Event_Mission_Kill;
	Row.Mode = ESpyMissionMode::Accumulate;
	Row.TargetCount = 5;
	MissionTable->AddRow(TEXT("Mission_1"), Row);
	Config->MissionTable = MissionTable;

	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, Config);

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);

	MissionComponent->AcceptCurrentMission();

	TestFalse(TEXT("No target location row -> path stays inactive"), NavComponent->IsPathActive());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpyNavigationComponentCompleteStopsPathTest,
	"SkillProject.Navigation.Component.CompleteStopsPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpyNavigationComponentCompleteStopsPathTest::RunTest(const FString& Parameters)
{
	AActor* Owner = NewObject<AActor>(GetTransientPackage());
	USpyMissionComponent* MissionComponent = NewObject<USpyMissionComponent>(Owner);
	SpyNavigationComponentTests_SetMissionConfig(MissionComponent, SpyNavigationComponentTests_MakeConfigWithTargetLocation());

	USpyNavigationComponent* NavComponent = NewObject<USpyNavigationComponent>(Owner);
	NavComponent->BindMissionComponent(MissionComponent);

	MissionComponent->AcceptCurrentMission();
	TestTrue(TEXT("Path active after accept"), NavComponent->IsPathActive());

	//# 목표 3회 중 3회 채워 완료시킨다 (Vault 미션, Accumulate)
	MissionComponent->AddProgress(SpyGameplayTags::Skill_Move_Vault, 3);

	TestFalse(TEXT("Path inactive after mission completed"), NavComponent->IsPathActive());

	return true;
}

#endif //# WITH_DEV_AUTOMATION_TESTS
```

- [ ] **Step 3: 테스트 실행 — 통과 확인**

컴파일 후 Automation 탭에서 `SkillProject.Navigation.Component` 필터로 세 테스트 실행.
Expected: 세 테스트 모두 PASS(Step 1에서 이미 실제 구현을 작성했으므로 이 태스크는 RED 단계가 없다 — 상태 머신이 단순해 스텁을 따로 두지 않고 바로 구현했다. Step 1의 코드가 실제로 의도대로 동작하는지 이 스텝에서 처음 확인한다).

- [ ] **Step 4: 스테이징 + 커밋 메시지(안) 제시**

```bash
git add SkillProject/Source/SkillProject/ManagerComponent/SpyNavigationComponent.h SkillProject/Source/SkillProject/ManagerComponent/SpyNavigationComponent.cpp SkillProject/Source/SkillProject/ManagerComponent/Tests/SpyNavigationComponentTests.cpp
```

제시할 메시지(안): `[Feature] USpyNavigationComponent — 미션 수락/완료 반응 상태 머신 추가`

---

### Task 5: NavMesh 재경로 + 스플라인 메시 렌더링

**Files:**
- Modify: `SkillProject/Source/SkillProject/ManagerComponent/SpyNavigationComponent.h`
- Modify: `SkillProject/Source/SkillProject/ManagerComponent/SpyNavigationComponent.cpp`
- Check: `SkillProject/Source/SkillProject/SkillProject.Build.cs` (NavigationSystem 모듈 의존성 확인)

**Interfaces:**
- Consumes: Task 3의 `SpyNavPathMath::{BuildSplineSegments, TrimLeadingDistance, ComputePathLength, EvaluateHysteresisVisibility}`, `UNavigationSystemV1::FindPathToLocationSynchronously` (엔진 API), `USpyAssetManager::GetAssetByName<T>` (plugin-skassetcore.md §2).
- Produces: 실제 렌더링 — 자동화 테스트로 검증하지 않는다(NavMesh/월드 필요). 이 태스크는 수동 PIE 검증으로 마무리한다. design §7-5 항목2·4·6(임계값 필드·Z오프셋·실패시 숨김) 전부 이 태스크에서 반영.

- [ ] **Step 1: Build.cs 의존성 확인**

`SkillProject.Build.cs` 를 열어 `PublicDependencyModuleNames`/`PrivateDependencyModuleNames` 에 `"NavigationSystem"` 이 있는지 확인한다 — `AI/Tests`, `BTTask_FindRandomPos.cpp` 가 이미 `UNavigationSystemV1` 을 쓰고 있으므로 대개 이미 있다. 없으면 `PrivateDependencyModuleNames` 에 추가.

- [ ] **Step 2: 헤더 확장**

`SpyNavigationComponent.h` — `class USpyMissionComponent;` 바로 뒤에 forward-declare 추가:

```cpp
class USplineComponent;
class USplineMeshComponent;
```

`FTimerHandle BindRetryTimerHandle;` 바로 뒤에 추가:

```cpp
	FTimerHandle RepathTimerHandle;
```

`bool bPathActive = false;` 바로 뒤(protected 섹션)에 추가:

```cpp

	UPROPERTY(Transient)
	TObjectPtr<USplineComponent> PathSpline;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USplineMeshComponent>> PathSegmentPool;

	//# §4-3 가시성 히스테리시스의 "이전 프레임 상태" — SpyNavPathMath::EvaluateHysteresisVisibility 가
	//# 순수 함수로 남도록 이 컴포넌트가 상태를 들고 있는다(Task 3 확장 참고)
	bool bPathVisible = false;

	//# design §7-5 항목2(mission-ground-navigation.md) — §4 합성 규칙 확정값을 그대로 기본값으로 노출
	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	float StartOffsetDistanceCm = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	float ArrivalHideDistanceCm = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	float ArrivalReshowDistanceCm = 400.f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	float GroundZOffsetCm = 3.f;
```

`void StartPathTo(...)`/`void StopPath();` 선언 바로 뒤에 추가:

```cpp
	void RecomputePath();
	void ApplyPathPoints(const TArray<FVector>& InPathPoints);
	void EnsureSegmentPoolSize(int32 InRequiredCount);
	void HideVisual();
```

- [ ] **Step 3: cpp 확장 — include 추가**

`SpyNavigationComponent.cpp` 상단 include 목록에 추가:

```cpp
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Manager/SpyAssetManager.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "System/SpyNavPathMath.h"
```

- [ ] **Step 4: `BeginPlay` 에 스플라인 루트 생성 추가**

`BeginPlay()` 안, `IsLocallyControlled() == false` 가드 통과 직후(즉 `AutoDiscoverAndBindMissionComponent()` 호출 이전)에 삽입:

```cpp
	PathSpline = NewObject<USplineComponent>(GetOwner(), TEXT("SpyNavigationPathSpline"));
	PathSpline->SetupAttachment(GetOwner()->GetRootComponent());
	PathSpline->RegisterComponent();
	PathSpline->SetVisibility(false);
```

- [ ] **Step 5: `StartPathTo`/`StopPath` 확장 + `RecomputePath`/`ApplyPathPoints`/`EnsureSegmentPoolSize` 구현**

`StartPathTo` 를 교체:

```cpp
void USpyNavigationComponent::StartPathTo(const FVector& InTargetLocation)
{
	CurrentTargetLocation = InTargetLocation;
	bPathActive = true;

	//# 콜드 스타트 — 지킬 "이전 프레임 가시성"이 없다. "이전에 보이고 있었다"로 시드해
	//# 규칙1의 <300 숨김 분기만 적용되게 한다(design §4-3 콜드 스타트 조항, code-reviewer 1차검토 BLOCKER).
	//# false 로 두면 목표가 400cm 미만인 미션은 재접근 전까지 라인이 한 번도 안 보인다.
	bPathVisible = true;

	RecomputePath();

	if (UWorld* World = GetWorld())
		World->GetTimerManager().SetTimer(RepathTimerHandle, this, &USpyNavigationComponent::RecomputePath, UpdateIntervalSeconds, true);
}
```

`StopPath` 를 교체:

```cpp
void USpyNavigationComponent::StopPath()
{
	bPathActive = false;
	CurrentTargetLocation = FVector::ZeroVector;

	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(RepathTimerHandle);

	if (PathSpline != nullptr)
		PathSpline->ClearSplinePoints(true);

	HideVisual();
}
```

파일 끝에 함수 추가(`HideVisual` 신규 — design §7-5 항목6, NavMesh 질의 실패 시 낡은 경로를 그대로 두지 않고 숨긴다):

```cpp
void USpyNavigationComponent::HideVisual()
{
	bPathVisible = false;

	if (PathSpline != nullptr)
		PathSpline->SetVisibility(false);

	for (USplineMeshComponent* Segment : PathSegmentPool)
	{
		if (Segment != nullptr)
			Segment->SetVisibility(false);
	}
}

void USpyNavigationComponent::RecomputePath()
{
	if (bPathActive == false)
		return;

	AActor* Owner = GetOwner();
	if (Owner == nullptr)
		return;

	UWorld* World = GetWorld();
	if (World == nullptr)
		return;

	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(World);
	if (NavSystem == nullptr)
	{
		//# design §7-5 항목6 — 질의 자체가 불가능한 프레임도 "낡은 경로 유지"가 아니라 숨김으로 처리
		HideVisual();

		return;
	}

	UNavigationPath* NavPath = NavSystem->FindPathToLocationSynchronously(World, Owner->GetActorLocation(), CurrentTargetLocation);
	if (NavPath == nullptr || NavPath->IsValid() == false)
	{
		//# 목표 좌표가 NavMesh 밖(§8 조건 3·4 위반 등)이면 매번 이 경로를 탄다 — 낡은 경로를
		//# 가리킨 채 영구 정지하는 무증상 실패를 막는다(design §2 와 같은 계열, §7-5 항목6)
		HideVisual();

		return;
	}

	ApplyPathPoints(NavPath->PathPoints);
}

void USpyNavigationComponent::ApplyPathPoints(const TArray<FVector>& InPathPoints)
{
	if (PathSpline == nullptr)
		return;

	//# §4-3 규칙 순서 — 반드시 트리밍 전 "원본" 경로 길이로 히스테리시스를 먼저 판정한다.
	//# (design-reviewer 2차검토 BLOCKER: 트림 후 길이로 판정하면 숨김/재표시 경계가 300/400
	//# 이 아니라 400/500 으로 밀려, §4-2 가 없애려던 "발밑에서 분리된 조각"이 형태만 바꿔 재발한다)
	const float RemainingPathLength = SpyNavPathMath::ComputePathLength(InPathPoints);
	bPathVisible = SpyNavPathMath::EvaluateHysteresisVisibility(RemainingPathLength, ArrivalHideDistanceCm, ArrivalReshowDistanceCm, bPathVisible);

	if (bPathVisible == false)
	{
		HideVisual();

		return;
	}

	//# §4-3 규칙2 — 오프셋은 조건부다. 남은 길이가 ArrivalReshowDistanceCm(400) 이상일 때만
	//# StartOffsetDistanceCm(100) 을 트리밍하고, 그 미만(보이는 채로 300~400cm 구간)이면
	//# 오프셋을 0 으로 낮춰 플레이어 실제 위치부터 그려 "발밑 분리 조각"을 방지한다
	const float TrimDistanceCm = (RemainingPathLength >= ArrivalReshowDistanceCm) ? StartOffsetDistanceCm : 0.f;
	const TArray<FVector> TrimmedPoints = SpyNavPathMath::TrimLeadingDistance(InPathPoints, TrimDistanceCm);

	//# §7-5 항목4 — 지면 z-fighting 방지용 Z 오프셋을 렌더용 사본에만 적용(NavMesh 질의 좌표 자체는 불변)
	TArray<FVector> OffsetPoints;
	OffsetPoints.Reserve(TrimmedPoints.Num());
	for (const FVector& Point : TrimmedPoints)
	{
		OffsetPoints.Add(Point + FVector(0.f, 0.f, GroundZOffsetCm));
	}

	PathSpline->SetVisibility(true);
	PathSpline->ClearSplinePoints(false);
	for (const FVector& Point : OffsetPoints)
	{
		PathSpline->AddSplinePoint(Point, ESplineCoordinateSpace::World, false);
	}
	PathSpline->UpdateSpline();

	const TArray<TPair<FVector, FVector>> Segments = SpyNavPathMath::BuildSplineSegments(OffsetPoints);
	EnsureSegmentPoolSize(Segments.Num());

	for (int32 Index = 0; Index < Segments.Num(); ++Index)
	{
		USplineMeshComponent* Segment = PathSegmentPool[Index];
		const FVector Tangent = (Segments[Index].Value - Segments[Index].Key);

		Segment->SetVisibility(true);
		Segment->SetStartAndEnd(Segments[Index].Key, Tangent, Segments[Index].Value, Tangent, true);
	}

	for (int32 Index = Segments.Num(); Index < PathSegmentPool.Num(); ++Index)
	{
		PathSegmentPool[Index]->SetVisibility(false);
	}
}

void USpyNavigationComponent::EnsureSegmentPoolSize(int32 InRequiredCount)
{
	while (PathSegmentPool.Num() < InRequiredCount)
	{
		USplineMeshComponent* Segment = NewObject<USplineMeshComponent>(GetOwner());
		Segment->SetMobility(EComponentMobility::Movable);
		Segment->SetupAttachment(GetOwner()->GetRootComponent());
		Segment->RegisterComponent();

		//# 글로우 머티리얼 에셋 제작은 이번 스펙 범위 밖(아트 작업) — 미등록 시 엔진 기본 머티리얼로 표시된다
		if (UMaterialInterface* GlowMaterial = USpyAssetManager::GetAssetByName<UMaterialInterface>(TEXT("NavPathGlow")))
			Segment->SetMaterial(0, GlowMaterial);

		PathSegmentPool.Add(Segment);
	}
}
```

- [ ] **Step 6: 수동 PIE 검증**

자동화 테스트로 대체할 수 없는 부분(NavMesh 통합) — PIE 로 직접 확인한다:
1. `DevMap` 로드 (레벨 기본 열림이 빈 레벨이므로 직접 연다 — memory `reference_devmap_coordinates`).
2. Task 1에서 만든 `Mission_TargetLocation` DataTable 에 실제 미션 1개의 좌표를 임시로 채운다(에디터에서 직접, 또는 Task 6 완료 후 정식 데이터로).
3. PIE 진입 → 해당 미션 수락 → 바닥에 스플라인 메시 세그먼트가 나타나는지 확인. 발밑에서 분리된 조각이나 뚝 끊김 없이 자연스럽게 시작되는지(design §4-2/M-nav-3) 확인.
4. 플레이어가 이동하면 0.75초 간격으로 경로가 갱신되는지 확인(장애물을 사이에 두고 이동해 우회하는지).
5. 목표에 접근해 300cm(`ArrivalHideDistanceCm`) 안쪽으로 들어가면 라인이 사라지고, 다시 400cm(`ArrivalReshowDistanceCm`) 밖으로 나가면 재표시되는지 확인(M-nav-2/M-nav-3). 300~400cm 사이를 오갈 때 깜빡이지(진동) 않는지도 함께 확인.
6. 미션 완료 시 라인이 사라지는지 확인.
7. `Mission_TargetLocation` 좌표를 일부러 NavMesh 밖(예: 벽 안쪽)으로 설정해보고, 라인이 낡은 경로에 멈춰있지 않고 사라지는지 확인(design §7-5 항목6).
8. ⚠ **1인 PIE 만으로는 Task 2 Step 6~9 에서 고친 결함이 통과된다** — 반드시 **2인 이상 PIE**(Editor Preferences → Play → Number of Players ≥ 2, Net Mode = Play As Client)로 원격 클라이언트 화면에서도 위 1~7이 동일하게 보이는지 별도로 확인한다(design §2-4, M-nav-5). 호스트 화면만 보고 통과 판정하지 않는다.

- [ ] **Step 7: 스테이징 + 커밋 메시지(안) 제시**

```bash
git add SkillProject/Source/SkillProject/ManagerComponent/SpyNavigationComponent.h SkillProject/Source/SkillProject/ManagerComponent/SpyNavigationComponent.cpp
```

(Build.cs 를 수정했다면 그 파일도 함께 add.)

제시할 메시지(안): `[Feature] USpyNavigationComponent — NavMesh 재경로 + 스플라인 메시 렌더링 + 가시성 히스테리시스`

---

### Task 6: 캐릭터 컴포넌트 주입 등록 + 최종 확인

**Files:**
- Modify (에셋, 코드 아님): `DA_SpyCharacterAssetData` (또는 동일 클래스의 실제 인스턴스 — Content Browser 에서 클래스로 검색)

**Interfaces:**
- Consumes: `USpyCharacterAssetData::FCharacterAssetSet::CommonComponentClasses` (`TArray<TSubclassOf<UActorComponent>>`, 기존 필드).
- Produces: 없음(데이터 전용 최종 태스크).

- [ ] **Step 1: 대상 에셋 찾기**

정확한 경로가 코드에 하드코딩돼 있지 않다(의도된 설계 — CharacterAssetData 인스턴스는 BP 기본값으로 지정됨). `mcp__unreal-mcp__find_assets_by_class` 로 클래스 `SpyCharacterAssetData` 를 검색해 실제 에셋 경로를 확인하거나, Content Browser 에서 직접 찾는다(`Content/Spy/Data/` 하위일 가능성이 높다).

- [ ] **Step 2: `USpyNavigationComponent` 를 `CommonComponentClasses` 에 추가**

**우선 시도 — unreal-mcp**: `get_asset_properties` 로 현재 `CharacterAssets.CommonComponentClasses` 배열을 확인 → `set_asset_property` 로 `USpyNavigationComponent` 클래스를 추가 시도. `EditDefaultsOnly` 중첩 struct(`FCharacterAssetSet`) 안의 배열이라 단순 `set_editor_property` 가 `"cannot be edited on instances"` 로 막힐 수 있다 — 이 경우 ui-workflow.md §2-2 의 `export_text()`/`import_text()` 왕복 패턴으로 우회한다.

**대체 — 에디터 수동 편집**: 위 MCP 경로가 막히면 Unreal Editor 에서 해당 DataAsset 을 열어 `Character Assets > Common Component Classes` 배열에 `SpyNavigationComponent` 를 직접 추가하고 저장한다. `SpyPawnExtensionComponent::HandleChangeInitState`(기존 `Character/SpyPawnExtensionComponent.cpp`)의 `CommonComponentClasses` 순회 로직이 InitState_Spawned 시점에 이 배열을 읽어 자동으로 `NewObject`+`RegisterComponent` 하므로, 이 배열에 등록하는 것 외에 추가 코드 변경은 필요 없다.

- [ ] **Step 3: 저장 검증**

에셋을 되읽어(`get_asset_properties` 또는 `.uasset` 타임스탬프) `CommonComponentClasses` 에 `SpyNavigationComponent` 가 실제로 반영됐는지 확인한다(ui-workflow.md "신뢰 못 하는 도구는 되읽어 검증" 원칙).

- [ ] **Step 4: 통합 PIE 검증**

PIE 진입 → 플레이어 폰에 `USpyNavigationComponent` 가 실제로 부착됐는지(Outliner/디테일 패널 또는 로그의 `"# Success Attach Component: SpyNavigationComponent"` 메시지, `SpyPawnExtensionComponent.cpp` 기존 로그 라인) 확인 → Task 5 Step 6 의 시나리오를 다시 한번 전체 플로우로 재현한다.

- [ ] **Step 5: 스테이징 + 커밋 메시지(안) 제시**

에셋 변경만 있다면(코드 변경 0건) — project.md "에셋 한정 사이클" 게이트에 따라 code-reviewer 를 곧장 부르지 않고 사용자에게 "리뷰 진행 / 생략" 선택지를 먼저 제시한다.

```bash
git add <에셋 경로 .uasset>
```

제시할 메시지(안): `[Chore] DA_SpyCharacterAssetData — USpyNavigationComponent 런타임 주입 등록`

---

## Self-Review 메모 (계획 작성자 기록)

- **스펙 커버리지**: spec §3(데이터)→Task 1, §4(델리게이트)→Task 2, §5(컴포넌트 라이프사이클)→Task 4+5, §6(렌더링)→Task 5, §7(테스트 범위)→각 태스크에 포함, §8(열린 질문)→Global Constraints에서 "직접 참조"로 확정.
- **타입 일관성 확인**: `FSpyMission_TargetLocationRow`, `USpyMissionComponent::GetMissionTargetLocation`, `USpyNavigationComponent::{IsPathActive, GetCurrentTargetLocation, BindMissionComponent}` 이름이 Task 1→2→4→5 전체에서 동일하게 유지됨을 확인했다.
- **알려진 한계**: Task 5(NavMesh/렌더링 통합)는 이 프로젝트에 CLI 테스트 러너가 없어 자동화 테스트로 커버하지 못한다 — Step 6 수동 PIE 체크리스트가 유일한 검증 수단이다. test-engineer 단계(start-develop 파이프라인 계속 시)에서 이 한계를 다시 확인할 것.
- **2026-08-04 개정 1차 — design-reviewer 이전, game-designer 기획서(§2) 반영**: Task 2 에 Step 6~9(`OnRep_MissionState` 확장 + 원격 클라이언트 상태-diff 브로드캐스트)를 추가했다. 원래 Step 1~5(직접 브로드캐스트)만으로는 데디케이티드 서버 + 원격 클라이언트 구성에서 `OnMissionAccepted`/`OnMissionCompleted` 가 영원히 도달하지 않는 결함이 있었다(1인 PIE 에서만 우연히 통과). Task 5 Step 6 수동 검증에 "2인 이상 PIE 필수" 항목을 추가해 이 결함이 회귀하지 않는지 확인하게 했다.
- **2026-08-04 개정 2차 — design-reviewer 1차검토 BLOCKER 반영**: `OnRep_MissionState` 안에서 `OnMissionCompleted` 를 `OnMissionAccepted` 보다 먼저 판정하도록 순서를 고정했다(원안은 반대 순서라 `HandleMissionCompleted`(무조건 `StopPath`)가 막 시작된 새 경로를 즉시 지웠다 — Dialogue 자동수락 전이 MissionId 2/4/6/8/10/12 전부에서 상시 재현). Task 2 Step 6 에 순서 회귀 테스트(`FSpyMissionComponentRemoteClientOrderTest`, 리스너에 발화 순번 기록) 추가.
- **2026-08-04 개정 3차 — design-reviewer 1차검토 MAJOR(§7-5 항목2/3/4/6 미반영) 해소**: Task 3 에 Step 6~10(가시성 히스테리시스용 순수 함수 3종 — `TrimLeadingDistance`/`ComputePathLength`/`EvaluateHysteresisVisibility`) 추가. Task 5 헤더에 4개 `EditDefaultsOnly` 임계값 필드 + `bPathVisible` 상태, `ApplyPathPoints` 에 트리밍·히스테리시스·Z오프셋 적용, `RecomputePath`/`HideVisual` 로 NavMesh 질의 실패 시 낡은 경로를 숨기도록 반영. 이로써 §7-5 표 6항목 전부 plan 에 반영 완료.
- **2026-08-04 개정 4차 — design-reviewer 2차검토 BLOCKER(plan 전용, design 문서 무관) 해소**: `ApplyPathPoints`(Task 5 Step 5)가 §4-3 순서를 어겼다 — 히스테리시스 판정에 **트림 후** 길이를 넣어 숨김/재표시 경계가 300/400 이 아니라 400/500 으로 밀리는 결함(개별 순수 함수는 정상, 호출부 조합만 오류). `RemainingPathLength` 를 **원본(트림 전)** 경로로 계산해 먼저 히스테리시스를 판정하고, 트림량 자체를 `RemainingPathLength >= ArrivalReshowDistanceCm ? StartOffsetDistanceCm : 0.f` 로 조건부화하도록 수정(§4-3 규칙1→2 순서 정확히 반영). 같은 검토에서 지적된 Task 3 Step 6 의 죽은(vacuous) 단정문(`X == false && false`)도 제거.
- **2026-08-04 개정 5차 — 구현 후 code-reviewer 1차검토 BLOCKER(콜드 스타트 명세 공백) 해소**: gameplay-programmer 구현물에서 `bPathVisible` 이 `false` 로 초기화된 채 첫 `RecomputePath` 를 돌아, 목표<400cm 인 미션은 라인이 재접근 전까지 한 번도 안 보이는 결함 발견(§4-2 보다 나쁜 "완전 비표시"). design §4-3 에 콜드 스타트 조항을 신설(게임 디자이너 확정) 후, `StartPathTo()`(Task 5 Step 5)에 `bPathVisible = true` 초기화 한 줄 추가로 반영.
