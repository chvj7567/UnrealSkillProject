# 미션 데이터 DataTable 전환 + NPCId 필드 추가 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** `USpyMissionConfig::Missions[]`(UDataAsset TArray)를 `UDataTable`(`FSpyMissionRow`)로 전환하고, 미션 데이터에서 바로 담당 NPC를 알 수 있도록 `NPCId` 필드를 추가한다.

**Architecture:** `FSpyMissionEntry`(plain USTRUCT)를 `FSpyMissionRow`(`FTableRowBase` 상속)로 이름을 바꾸고 `MissionId`/`NPCId` 필드를 추가한다. `USpyMissionConfig::Missions`(TArray)를 `MissionTable`(`UDataTable*`)로 바꾸고, `GetMission()`이 배열 인덱스 접근 대신 `MissionId` 필드로 테이블을 선형 스캔한다(기존 `GetMissionReward()`와 동일 패턴). `ResolveMissionProgress`의 판정 로직·`DT_SpyMissionCommunication`·`ASpyNPCCharacter`·`USpyMissionComponent`의 진행/수락 로직은 전혀 바뀌지 않는다 — 오직 미션 데이터를 담는 컨테이너와 조회 방식만 바뀐다.

**Tech Stack:** Unreal Engine 5.7, C++, `UDataTable`/`FTableRowBase`, Unreal Automation Test.

**Spec:** `docs/superpowers/specs/2026-08-03-mission-datatable-npcid-design.md` (사용자 승인 완료)

> **(2026-08-03 후속 결정으로 갱신됨 — 1-based 전환)** 이 plan은 `MissionId`/`NPCId`를 0-based(0~11 / 0~5)로 작성됐으나, 사용자가 이후 별도로 "ID는 1부터"를 지시했고 코드·라이브 데이터가 이미 1-based(`MissionId` 1~12, `NPCId` 1~6)로 전환됐다. 아래 숫자는 전부 1-based로 정정했다 — 상세는 spec(`2026-08-03-mission-datatable-npcid-design.md`)·design(`docs/design/npc-mission-dialogue.md`) 문서의 동일 취지 노트 참조.

## Global Constraints

- 코딩 스타일은 `.claude/rules/cpp-style.md`를 그대로 따른다 — `//#` 주석, `auto`/`!` 금지, 가드 절 중괄호 없이, `TObjectPtr<>` 사용.
- `DataTable` row struct 필드는 `EditAnywhere`를 쓴다(`EditDefaultsOnly` 아님) — 이 프로젝트의 기존 row struct(`FSpyMissionRewardRow`, `FSpyNPCRow` 등)가 전부 이 컨벤션을 쓴다.
- `MissionId`는 밀도 있는 연속 정수(1,2,3...)를 유지해야 `ResolveMissionProgress`의 "+1 = 다음 미션" 산술이 성립한다 — 이 spec은 그 산술을 바꾸지 않는다(spec §2-3).
- **`DT_SpyMissionCommunication`, `DT_SpyNPC`, `DT_SpyMissionReward`, `ASpyNPCCharacter`, `ResolveNPCDialogueState`, `USpyMissionComponent`의 진행/수락 로직(`ProcessProgress`/`AddProgress`/`AcceptCurrentMission`/`GrantReward`)은 이 계획에서 절대 수정하지 않는다** — 타입 이름이 등장하는 곳만 기계적으로 따라간다(spec §3-3).
- 커밋 메시지 포맷: `[Tag] ClassName — 요약`(`.claude/rules/git-conventions.md`). **`git commit`을 직접 실행하지 않는다** — `git add`까지만, 커밋 메시지(안)를 제시한다.
- 이 프로젝트의 Automation 테스트는 순수 함수만 대상으로 한다(`USpyMissionConfig`의 `ResolveMissionProgress`/`GetMission`/`GetMissionReward` 등 — 컴포넌트/액터 레벨은 대상 아님). 기존 `SpyMissionTests.cpp`/`SpyNPCDialogueEdgeCaseTests.cpp`의 픽스처 헬퍼(`NewObject<UDataTable>()` + `Table->RowStruct = ...` + `Table->AddRow(...)`)가 이미 이 패턴을 확립해 뒀다.
- 컴파일 검증은 사용자가 Unreal Editor/Visual Studio에서 수행한다 — 각 태스크는 "컴파일 확인" 스텝을 명시하되 실제 빌드 실행은 사용자 몫이다.

---

### Task 1: `Data/SpyMissionConfig.h/.cpp` — `FSpyMissionEntry` → `FSpyMissionRow` 전환

**Files:**
- Modify: `SkillProject/Source/SkillProject/Data/SpyMissionConfig.h` (전체 파일, 131줄)
- Modify: `SkillProject/Source/SkillProject/Data/SpyMissionConfig.cpp` (전체 파일)

**Interfaces:**
- Produces: `FSpyMissionRow`(struct, `FTableRowBase` 상속) — `MissionId`/`MissionType`/`MatchTag`/`Mode`/`TargetCount`/`DisplayName`/`Description`/`NPCId` 필드(`PreAcceptHintText`는 2026-08-03 후속 결정으로 폐기됨 — 아래 Step 1 참조). `USpyMissionConfig::GetMission(int32 InMissionId) const` → `const FSpyMissionRow*`. `USpyMissionConfig::MissionTable`(`TObjectPtr<UDataTable>`). Task 2·3·4가 이 타입 이름과 시그니처를 그대로 소비한다.

- [ ] **Step 1: 헤더 — `FSpyMissionEntry`를 `FSpyMissionRow`로 교체**

`Data/SpyMissionConfig.h:24-56` 현재 코드:

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

	//# 미수락 상태 HUD 안내(예: "레이븐을 찾아가라"). Gameplay 타입만 의미 있음 —
	//# Dialogue/Interact 타입은 진입과 동시에 자동 수락되므로 이 상태 자체가 존재하지 않는다
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	FText PreAcceptHintText;
};
```

아래로 교체:

```cpp
//# 미션 1개의 정의. 명시적 MissionId 로 식별한다 — 배열 위치에 의존하지 않는다(spec §2-1).
//# 이 값은 MissionReward.MissionId / MissionCommunication.MissionId 와 일치해야 하고, 1부터 시작하는
//# 밀도 있는 연속 정수(1,2,3...)여야 ResolveMissionProgress 의 "+1 = 다음 미션" 산술이 성립한다(spec §2-3)
USTRUCT(BlueprintType)
struct FSpyMissionRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	int32 MissionId = 0;

	UPROPERTY(EditAnywhere)
	ESpyMissionType MissionType = ESpyMissionType::Gameplay;

	//# 이 미션이 반응할 이벤트 태그. Dialogue 타입은 전부 공용 Event_Mission_Report 를 쓴다
	UPROPERTY(EditAnywhere)
	FGameplayTag MatchTag;

	UPROPERTY(EditAnywhere)
	ESpyMissionMode Mode = ESpyMissionMode::Accumulate;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "1"))
	int32 TargetCount = 1;

	//# HUD 상시 표시 이름. Dialogue 타입은 이 값 자체가 "시스템 메시지"다
	UPROPERTY(EditAnywhere)
	FText DisplayName;

	//# 수락 카드 서술문. Gameplay 타입만 사용 — Dialogue 타입은 카드가 없으므로 빈 문자열로 둔다
	UPROPERTY(EditAnywhere)
	FText Description;

	//# 미수락 상태 HUD 안내(예: "레이븐을 찾아가라"). Gameplay 타입만 의미 있음 —
	//# Dialogue/Interact 타입은 진입과 동시에 자동 수락되므로 이 상태 자체가 존재하지 않는다
	UPROPERTY(EditAnywhere)
	FText PreAcceptHintText;

	//# 이 미션을 담당하는 NPC. 9999 = 시스템 퀘스트(NPC 없음). 어떤 게임플레이 로직도 이 필드를
	//# 읽지 않는다 — 순수 조회 편의용 sentinel 이다(spec §2-4, cpp-style §14-1-3 예외로 확정)
	UPROPERTY(EditAnywhere)
	int32 NPCId = 9999;
};
```

**(2026-08-03 후속 결정으로 폐기됨)** 위 코드 블록의 `PreAcceptHintText` 필드는 이 Task(DataTable 전환) 이후 별도 지시로 완전히 제거됐다 — HUD 미수락 안내 문구는 `NPCId` 기반 동적 조회(`SpyMainHUD::ResolveNPCNameHintText`)로 대체됐다. 실제 `SpyMissionConfig.h`엔 이 필드가 없다. 상세는 spec(`2026-08-03-mission-datatable-npcid-design.md` §1·§3-1) 정정 노트 참조.

- [ ] **Step 2: 헤더 — `USpyMissionConfig`의 `Missions` 필드를 `MissionTable`로 교체**

`Data/SpyMissionConfig.h:97-130` 현재 코드:

```cpp
UCLASS()
class SKILLPROJECT_API USpyMissionConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	//# 배열 인덱스가 곧 진행 순서다
	//# 값은 DA_SpyMissionConfig 에디터에서 입력한다 (코드 기본값 없음)
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	TArray<FSpyMissionEntry> Missions;

	//# Dialogue 타입 미션의 보상 관계 테이블. RowStruct = FSpyMissionRewardRow
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	TObjectPtr<UDataTable> MissionRewardTable;

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

	//# MissionId 로 보상을 조회한다. 행이 없으면(Gameplay 타입) 0.f — sentinel 이 아니라
	//# "관계 없음"의 정상적인 부재 결과다 (spec §4-3)
	UFUNCTION(BlueprintPure, Category = "Mission")
	float GetMissionReward(int32 InMissionId) const;
};
```

아래로 교체:

```cpp
UCLASS()
class SKILLPROJECT_API USpyMissionConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	//# 미션 데이터. RowStruct = FSpyMissionRow. MissionId 필드가 진행 순서를 결정한다
	//# (배열 위치가 아니다). 값은 DT_SpyMission 에디터에서 입력한다 (코드 기본값 없음)
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	TObjectPtr<UDataTable> MissionTable;

	//# Dialogue 타입 미션의 보상 관계 테이블. RowStruct = FSpyMissionRewardRow
	UPROPERTY(EditDefaultsOnly, Category = "Mission")
	TObjectPtr<UDataTable> MissionRewardTable;

public:
	UFUNCTION(BlueprintPure, Category = "Mission")
	int32 GetMissionCount() const;

	UFUNCTION(BlueprintPure, Category = "Mission")
	bool IsValidMissionIndex(int32 InMissionId) const;

	//# MissionId 로 조회한다(배열 인덱스 아님). 없으면 nullptr
	const FSpyMissionRow* GetMission(int32 InMissionId) const;

	//# 진행 판정 — 부수효과 없음. 한 번의 호출로 최대 1개 미션만 완료한다
	UFUNCTION(BlueprintPure, Category = "Mission")
	FSpyMissionProgressResult ResolveMissionProgress(int32 InIndex, int32 InCount, FGameplayTag InEventTag, int32 InAmount) const;

	//# MissionId 로 보상을 조회한다. 행이 없으면(Gameplay 타입) 0.f — sentinel 이 아니라
	//# "관계 없음"의 정상적인 부재 결과다 (spec §4-3)
	UFUNCTION(BlueprintPure, Category = "Mission")
	float GetMissionReward(int32 InMissionId) const;
};
```

- [ ] **Step 3: cpp — `GetMissionCount`/`IsValidMissionIndex`/`GetMission`을 DataTable 스캔으로 재작성**

`Data/SpyMissionConfig.cpp` 현재 코드(파일 상단):

```cpp
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
		return nullptr;

	return &Missions[InIndex];
}
```

아래로 교체:

```cpp
int32 USpyMissionConfig::GetMissionCount() const
{
	if (MissionTable == nullptr)
		return 0;

	return MissionTable->GetRowMap().Num();
}

bool USpyMissionConfig::IsValidMissionIndex(int32 InMissionId) const
{
	return (GetMission(InMissionId) != nullptr);
}

const FSpyMissionRow* USpyMissionConfig::GetMission(int32 InMissionId) const
{
	if (MissionTable == nullptr)
		return nullptr;

	TArray<FSpyMissionRow*> Rows;
	MissionTable->GetAllRows<FSpyMissionRow>(TEXT("USpyMissionConfig::GetMission"), Rows);

	for (const FSpyMissionRow* Row : Rows)
	{
		if (Row != nullptr && Row->MissionId == InMissionId)
			return Row;
	}

	return nullptr;
}
```

(이 패턴은 같은 파일의 기존 `GetMissionReward()`가 `MissionRewardTable`을 스캔하는 것과 정확히 동일하다 — 새 패턴을 발명하지 않는다.)

- [ ] **Step 4: cpp — `ResolveMissionProgress`의 지역 변수 타입만 교체 (로직 불변)**

`Data/SpyMissionConfig.cpp`의 `ResolveMissionProgress` 함수 안, 현재 코드:

```cpp
	const FSpyMissionEntry* Entry = GetMission(Result.MissionIndex);
```

아래로 교체(이 한 줄만):

```cpp
	const FSpyMissionRow* Entry = GetMission(Result.MissionIndex);
```

그 아래 이어지는 태그 매칭·`Mode`/`TargetCount` 판정·`Result.MissionIndex += 1` 로직은 전부 **한 글자도 바꾸지 않는다**(spec §2-3).

- [ ] **Step 5: 컴파일 확인 (부분 실패 예상 — Task 2·3·4 전까지는 정상)**

이 태스크만 반영한 상태로는 `System/SpyMissionComponent.h/.cpp`, `NPC/SpyNPCCharacter.cpp`, `System/Tests/SpyMissionTests.cpp`, `System/Tests/SpyNPCDialogueEdgeCaseTests.cpp`가 여전히 옛 타입 이름(`FSpyMissionEntry`)과 `Missions.Add(...)`를 참조해 **컴파일이 깨지는 게 정상이다.** Task 2·3·4까지 반영해야 전체가 컴파일된다 — 이 시점에 빌드를 시도할 필요는 없다.

- [ ] **Step 6: git add**

```bash
git add SkillProject/Source/SkillProject/Data/SpyMissionConfig.h SkillProject/Source/SkillProject/Data/SpyMissionConfig.cpp
```

(커밋은 Task 4 완료 후 전체를 묶어 한 번에 제시한다 — 중간 상태가 컴파일 안 되므로 개별 커밋 의미가 없다.)

---

### Task 2: `System/SpyMissionComponent.h/.cpp` + `NPC/SpyNPCCharacter.cpp` — 타입 이름 교체 (로직 불변)

**Files:**
- Modify: `SkillProject/Source/SkillProject/System/SpyMissionComponent.h:13,85`
- Modify: `SkillProject/Source/SkillProject/System/SpyMissionComponent.cpp:58,68,78,178,208,218,241`
- Modify: `SkillProject/Source/SkillProject/NPC/SpyNPCCharacter.cpp:179`

**Interfaces:**
- Consumes: `FSpyMissionRow`(Task 1), `USpyMissionConfig::GetMission(int32) const → const FSpyMissionRow*`(Task 1).
- Produces: 없음(타입 이름 교체만, 공개 API·동작 불변).

> ⚠ 이 태스크는 **로직을 절대 바꾸지 않는다** — `FSpyMissionEntry`라는 글자를 `FSpyMissionRow`로 바꾸는 것 외에는 단 한 줄도 건드리지 않는다. 각 줄의 앞뒤 코드(가드 절, `if`/`switch`, 필드 접근)는 정확히 지금 그대로 둔다.

- [ ] **Step 1: `SpyMissionComponent.h` — 전방 선언 + 반환 타입 교체**

`System/SpyMissionComponent.h:13`:

```cpp
struct FSpyMissionEntry;
```

아래로 교체:

```cpp
struct FSpyMissionRow;
```

`System/SpyMissionComponent.h:85`:

```cpp
	const FSpyMissionEntry* GetMissionEntry(int32 InIndex) const;
```

아래로 교체:

```cpp
	const FSpyMissionRow* GetMissionEntry(int32 InIndex) const;
```

- [ ] **Step 2: `SpyMissionComponent.cpp` — 7곳 타입 이름 교체**

아래 7개 줄에서 `FSpyMissionEntry`를 `FSpyMissionRow`로만 바꾼다(그 외 내용 불변):

| 줄(대략) | 현재 | 교체 후 |
|---|---|---|
| `GetTargetCount()` 안 | `const FSpyMissionEntry* Entry = MissionConfig->GetMission(MissionState.MissionIndex);` | `const FSpyMissionRow* Entry = MissionConfig->GetMission(MissionState.MissionIndex);` |
| `GetDisplayName()` 안 | `const FSpyMissionEntry* Entry = MissionConfig->GetMission(MissionState.MissionIndex);` | `const FSpyMissionRow* Entry = MissionConfig->GetMission(MissionState.MissionIndex);` |
| `GetCurrentNPCId()` 안 (2026-08-03 후속 결정 — 이 Task 당시엔 `GetPreAcceptHintText()`였다가 이후 `PreAcceptHintText` 필드 폐기와 함께 이 함수로 대체됨) | `const FSpyMissionEntry* Entry = MissionConfig->GetMission(MissionState.MissionIndex);` | `const FSpyMissionRow* Entry = MissionConfig->GetMission(MissionState.MissionIndex);` |
| `ProcessProgress()` 안 | `const FSpyMissionEntry* NewEntry = GetMissionEntry(MissionState.MissionIndex);` | `const FSpyMissionRow* NewEntry = GetMissionEntry(MissionState.MissionIndex);` |
| `AcceptCurrentMission()` 안 | `const FSpyMissionEntry* CurrentEntry = GetMissionEntry(MissionState.MissionIndex);` | `const FSpyMissionRow* CurrentEntry = GetMissionEntry(MissionState.MissionIndex);` |
| `GetMissionEntry()` 정의부 | `const FSpyMissionEntry* USpyMissionComponent::GetMissionEntry(int32 InIndex) const` | `const FSpyMissionRow* USpyMissionComponent::GetMissionEntry(int32 InIndex) const` |
| `GrantReward()` 안 | `const FSpyMissionEntry* Entry = MissionConfig->GetMission(InCompletedIndex);` | `const FSpyMissionRow* Entry = MissionConfig->GetMission(InCompletedIndex);` |

각 줄의 나머지 부분(`if (Entry ...)`, `->MissionType`, `->DisplayName` 등 멤버 접근)은 전부 그대로 둔다 — `FSpyMissionRow`가 `FSpyMissionEntry`와 동일한 필드를 전부 갖고 있으므로(Task 1에서 필드를 하나도 안 뺐다) 멤버 접근 코드는 손댈 필요가 없다.

- [ ] **Step 3: `NPC/SpyNPCCharacter.cpp` — 1곳 타입 이름 교체**

`NPC/SpyNPCCharacter.cpp:179`:

```cpp
			if (const FSpyMissionEntry* Entry = MissionComp->GetMissionEntry(CachedOfferMissionId))
```

아래로 교체:

```cpp
			if (const FSpyMissionRow* Entry = MissionComp->GetMissionEntry(CachedOfferMissionId))
```

이 파일은 이미 `#include "Data/SpyMissionConfig.h"`를 갖고 있어(8번째 줄 근처) 추가 include가 필요 없다.

- [ ] **Step 4: 컴파일 확인**

Task 1과 이 태스크가 함께 반영된 상태에서, `System/Tests/` 두 파일을 제외하면(Task 3·4에서 처리) 나머지 프로덕션 코드는 전부 컴파일이 통과해야 한다.

- [ ] **Step 5: git add**

```bash
git add SkillProject/Source/SkillProject/System/SpyMissionComponent.h SkillProject/Source/SkillProject/System/SpyMissionComponent.cpp SkillProject/Source/SkillProject/NPC/SpyNPCCharacter.cpp
```

---

### Task 3: `System/Tests/SpyMissionTests.cpp` — DataTable 기반 픽스처로 재작성

**Files:**
- Modify: `SkillProject/Source/SkillProject/System/Tests/SpyMissionTests.cpp` (전체 594줄 중 미션 관련 픽스처 부분)

**Interfaces:**
- Consumes: `FSpyMissionRow`, `USpyMissionConfig::MissionTable`(Task 1).
- Produces: 없음(테스트 파일, 다른 태스크가 참조하지 않음).

> 이 태스크의 목적은 **기존 17개 `IMPLEMENT_SIMPLE_AUTOMATION_TEST` 케이스를 전부 유지**하면서 픽스처 헬퍼만 `DataTable` 기반으로 바꾸는 것이다 — 어떤 `TestEqual`/`TestTrue` 단언도 값을 바꾸지 않는다. 이게 "로직이 안 바뀌었다"는 걸 증명하는 회귀 테스트다.

- [ ] **Step 1: `SpyMissionTests_AddMissionRow` 헬퍼 신설**

파일 상단, 기존 `SpyMissionTests_AddReward` 헬퍼(`Data/SpyMissionConfig.h`의 `FSpyMissionRewardRow`용) 바로 다음에 추가:

```cpp
//# 테스트 전용 헬퍼 — 완성된 FSpyMissionRow 를 MissionTable 에 추가한다.
//# 널이면 새로 만든다 (SpyMissionTests_AddReward 와 동일 패턴)
static void SpyMissionTests_AddMissionRow(USpyMissionConfig* Config, const FSpyMissionRow& Row)
{
	if (Config->MissionTable == nullptr)
	{
		UDataTable* Table = NewObject<UDataTable>();
		Table->RowStruct = FSpyMissionRow::StaticStruct();
		Config->MissionTable = Table;
	}

	Config->MissionTable->AddRow(FName(*FString::Printf(TEXT("Mission_%d"), Row.MissionId)), Row);
}
```

- [ ] **Step 2: `SpyMissionTests_MakeConfig()` 재작성**

현재 코드:

```cpp
static USpyMissionConfig* SpyMissionTests_MakeConfig()
{
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	FSpyMissionEntry Vault;
	Vault.MatchTag = SpyGameplayTags::Skill_Move_Vault;
	Vault.Mode = ESpyMissionMode::Accumulate;
	Vault.TargetCount = 3;
	Config->Missions.Add(Vault);
	SpyMissionTests_AddReward(Config, 0, 10.f);

	FSpyMissionEntry Level;
	Level.MatchTag = SpyGameplayTags::Skill_Move_Climb;
	Level.Mode = ESpyMissionMode::Threshold;
	Level.TargetCount = 3;
	Config->Missions.Add(Level);
	SpyMissionTests_AddReward(Config, 1, 20.f);

	return Config;
}
```

아래로 교체:

```cpp
static USpyMissionConfig* SpyMissionTests_MakeConfig()
{
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	FSpyMissionRow Vault;
	Vault.MissionId = 1;
	Vault.MatchTag = SpyGameplayTags::Skill_Move_Vault;
	Vault.Mode = ESpyMissionMode::Accumulate;
	Vault.TargetCount = 3;
	SpyMissionTests_AddMissionRow(Config, Vault);
	SpyMissionTests_AddReward(Config, 1, 10.f);

	FSpyMissionRow Level;
	Level.MissionId = 2;
	Level.MatchTag = SpyGameplayTags::Skill_Move_Climb;
	Level.Mode = ESpyMissionMode::Threshold;
	Level.TargetCount = 3;
	SpyMissionTests_AddMissionRow(Config, Level);
	SpyMissionTests_AddReward(Config, 2, 20.f);

	return Config;
}
```

**(2026-08-03 후속 결정 — 1-based 전환)** `MissionId` 값은 0부터가 아니라 1부터 채번한다(위 예시 Vault=1, Level=2로 갱신 완료). 아래 Step들의 `MissionId`도 전부 1-based로 갱신했다.

`SpyMissionTests_AddReward` 호출부(`MissionRewardTable` 대상)는 **손대지 않는다** — 그대로 둔다.

- [ ] **Step 3: `SpyMissionTests_MakeDesignConfig()` 재작성**

현재 코드(6개 `FSpyMissionEntry` + `Config->Missions.Add(...)` 블록):

```cpp
static USpyMissionConfig* SpyMissionTests_MakeDesignConfig()
{
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	//# 0 — 적 1명 처치
	FSpyMissionEntry Kill;
	Kill.MatchTag = SpyGameplayTags::Event_Mission_Kill;
	Kill.Mode = ESpyMissionMode::Accumulate;
	Kill.TargetCount = 1;
	Kill.DisplayName = FText::FromString(TEXT("적 1명 처치"));
	Config->Missions.Add(Kill);
	SpyMissionTests_AddReward(Config, 0, 20.f);

	//# 1 — 레벨 3 달성 (Threshold)
	FSpyMissionEntry Level;
	Level.MatchTag = SpyGameplayTags::Event_Mission_Level;
	Level.Mode = ESpyMissionMode::Threshold;
	Level.TargetCount = 3;
	Level.DisplayName = FText::FromString(TEXT("레벨 3 달성"));
	Config->Missions.Add(Level);
	SpyMissionTests_AddReward(Config, 1, 10.f);

	//# 2 — 콤보 4회 연결
	FSpyMissionEntry Combo;
	Combo.MatchTag = SpyGameplayTags::Event_Mission_Combo;
	Combo.Mode = ESpyMissionMode::Accumulate;
	Combo.TargetCount = 4;
	Combo.DisplayName = FText::FromString(TEXT("콤보 4회 연결"));
	Config->Missions.Add(Combo);
	SpyMissionTests_AddReward(Config, 2, 10.f);

	//# 3 — 장애물 넘기 5회
	FSpyMissionEntry Vault;
	Vault.MatchTag = SpyGameplayTags::Skill_Move_Vault;
	Vault.Mode = ESpyMissionMode::Accumulate;
	Vault.TargetCount = 5;
	Vault.DisplayName = FText::FromString(TEXT("장애물 넘기 5회"));
	Config->Missions.Add(Vault);
	SpyMissionTests_AddReward(Config, 3, 10.f);

	//# 4 — 벽 타기 3회
	FSpyMissionEntry Climb;
	Climb.MatchTag = SpyGameplayTags::Skill_Move_Climb;
	Climb.Mode = ESpyMissionMode::Accumulate;
	Climb.TargetCount = 3;
	Climb.DisplayName = FText::FromString(TEXT("벽 타기 3회"));
	Config->Missions.Add(Climb);
	SpyMissionTests_AddReward(Config, 4, 15.f);

	//# 5 — 그래플링 3회
	FSpyMissionEntry Grapple;
	Grapple.MatchTag = SpyGameplayTags::Skill_Move_GrappleHook;
	Grapple.Mode = ESpyMissionMode::Accumulate;
	Grapple.TargetCount = 3;
	Grapple.DisplayName = FText::FromString(TEXT("그래플링 3회"));
	Config->Missions.Add(Grapple);
	SpyMissionTests_AddReward(Config, 5, 15.f);

	return Config;
}
```

아래로 교체(각 항목에 `MissionId` 필드 추가, `Config->Missions.Add(X)` → `SpyMissionTests_AddMissionRow(Config, X)`, 그 외 동일):

```cpp
static USpyMissionConfig* SpyMissionTests_MakeDesignConfig()
{
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	//# 1 — 적 1명 처치
	FSpyMissionRow Kill;
	Kill.MissionId = 1;
	Kill.MatchTag = SpyGameplayTags::Event_Mission_Kill;
	Kill.Mode = ESpyMissionMode::Accumulate;
	Kill.TargetCount = 1;
	Kill.DisplayName = FText::FromString(TEXT("적 1명 처치"));
	SpyMissionTests_AddMissionRow(Config, Kill);
	SpyMissionTests_AddReward(Config, 1, 20.f);

	//# 2 — 레벨 3 달성 (Threshold)
	FSpyMissionRow Level;
	Level.MissionId = 2;
	Level.MatchTag = SpyGameplayTags::Event_Mission_Level;
	Level.Mode = ESpyMissionMode::Threshold;
	Level.TargetCount = 3;
	Level.DisplayName = FText::FromString(TEXT("레벨 3 달성"));
	SpyMissionTests_AddMissionRow(Config, Level);
	SpyMissionTests_AddReward(Config, 2, 10.f);

	//# 3 — 콤보 4회 연결
	FSpyMissionRow Combo;
	Combo.MissionId = 3;
	Combo.MatchTag = SpyGameplayTags::Event_Mission_Combo;
	Combo.Mode = ESpyMissionMode::Accumulate;
	Combo.TargetCount = 4;
	Combo.DisplayName = FText::FromString(TEXT("콤보 4회 연결"));
	SpyMissionTests_AddMissionRow(Config, Combo);
	SpyMissionTests_AddReward(Config, 3, 10.f);

	//# 4 — 장애물 넘기 5회
	FSpyMissionRow Vault;
	Vault.MissionId = 4;
	Vault.MatchTag = SpyGameplayTags::Skill_Move_Vault;
	Vault.Mode = ESpyMissionMode::Accumulate;
	Vault.TargetCount = 5;
	Vault.DisplayName = FText::FromString(TEXT("장애물 넘기 5회"));
	SpyMissionTests_AddMissionRow(Config, Vault);
	SpyMissionTests_AddReward(Config, 4, 10.f);

	//# 5 — 벽 타기 3회
	FSpyMissionRow Climb;
	Climb.MissionId = 5;
	Climb.MatchTag = SpyGameplayTags::Skill_Move_Climb;
	Climb.Mode = ESpyMissionMode::Accumulate;
	Climb.TargetCount = 3;
	Climb.DisplayName = FText::FromString(TEXT("벽 타기 3회"));
	SpyMissionTests_AddMissionRow(Config, Climb);
	SpyMissionTests_AddReward(Config, 5, 15.f);

	//# 6 — 그래플링 3회
	FSpyMissionRow Grapple;
	Grapple.MissionId = 6;
	Grapple.MatchTag = SpyGameplayTags::Skill_Move_GrappleHook;
	Grapple.Mode = ESpyMissionMode::Accumulate;
	Grapple.TargetCount = 3;
	Grapple.DisplayName = FText::FromString(TEXT("그래플링 3회"));
	SpyMissionTests_AddMissionRow(Config, Grapple);
	SpyMissionTests_AddReward(Config, 6, 15.f);

	return Config;
}
```

- [ ] **Step 4: `FSpyMissionEmptyConfigTest` — `Config->Missions.Empty();` 줄 삭제**

현재 코드:

```cpp
bool FSpyMissionEmptyConfigTest::RunTest(const FString& Parameters)
{
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();
	Config->Missions.Empty();

	TestEqual(TEXT("No missions"), Config->GetMissionCount(), 0);
```

아래로 교체(`Config->Missions.Empty();` 줄만 삭제 — `MissionTable`은 `NewObject`로 갓 생성된 시점에 이미 `nullptr`이라 별도 초기화가 필요 없다):

```cpp
bool FSpyMissionEmptyConfigTest::RunTest(const FString& Parameters)
{
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	TestEqual(TEXT("No missions"), Config->GetMissionCount(), 0);
```

- [ ] **Step 5: `FSpyMissionHierarchicalTagTest` — 픽스처 부분만 교체**

현재 코드:

```cpp
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	//# 부모 태그 Skill.Move 로 하위 전체를 묶는다.
	//# ⚠ 이 테스트는 판정 함수의 계층 매칭 능력만 검증한다. 실제 미션 데이터는
	//#   6종 전부 leaf 태그를 지정한다 (mission-system.md §6-1) —
	//#   부모 태그를 쓰면 넘기·벽타기·그래플이 서로의 미션에 섞여 집계된다
	FSpyMissionEntry AnyMove;
	AnyMove.MatchTag = SKGameplayTags::Skill_Move;
	AnyMove.Mode = ESpyMissionMode::Accumulate;
	AnyMove.TargetCount = 2;
	Config->Missions.Add(AnyMove);
```

아래로 교체:

```cpp
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	//# 부모 태그 Skill.Move 로 하위 전체를 묶는다.
	//# ⚠ 이 테스트는 판정 함수의 계층 매칭 능력만 검증한다. 실제 미션 데이터는
	//#   6종 전부 leaf 태그를 지정한다 (mission-system.md §6-1) —
	//#   부모 태그를 쓰면 넘기·벽타기·그래플이 서로의 미션에 섞여 집계된다
	FSpyMissionRow AnyMove;
	AnyMove.MissionId = 1;
	AnyMove.MatchTag = SKGameplayTags::Skill_Move;
	AnyMove.Mode = ESpyMissionMode::Accumulate;
	AnyMove.TargetCount = 2;
	SpyMissionTests_AddMissionRow(Config, AnyMove);
```

**(1-based 전환)** 이 픽스처의 `ResolveMissionProgress` 시작 인자도 `1`(위 `AnyMove.MissionId`와 동일)로 호출해야 한다 — 실제 테스트 코드(`SpyMissionTests.cpp`)에 이미 반영됨.

- [ ] **Step 6: `FSpyMissionCppDefaultsEmptyTest` — 타입 이름만 교체 + `NPCId`/`MissionId` 기본값 단언 추가**

현재 코드:

```cpp
bool FSpyMissionCppDefaultsEmptyTest::RunTest(const FString& Parameters)
{
	//# 미션 값은 에디터 DataAsset 이 단일 진실이다 (§6-3).
	//# C++ 에서 기본 미션을 채우기 시작하면 두 곳에 진실이 생기므로 비어 있음을 고정한다
	const USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	TestEqual(TEXT("Fresh config has no missions in C++"), Config->GetMissionCount(), 0);
	TestFalse(TEXT("Index 0 is invalid on a fresh config"), Config->IsValidMissionIndex(0));

	//# 엔트리 구조체 기본값도 함께 고정한다
	const FSpyMissionEntry Default;
	TestTrue(TEXT("Default mode is Accumulate"), Default.Mode == ESpyMissionMode::Accumulate);
	TestEqual(TEXT("Default target count"), Default.TargetCount, 1);
	TestFalse(TEXT("Default match tag is invalid"), Default.MatchTag.IsValid());

	//# 보상은 더 이상 FSpyMissionEntry 필드가 아니라 MissionRewardTable 관계다 —
	//# 테이블 미지정 상태에서는 "관계 없음"으로 0.f 를 반환해야 한다 (§4-3)
	TestEqual(TEXT("Reward is 0 with no MissionRewardTable"), Config->GetMissionReward(0), 0.f);

	return true;
}
```

아래로 교체:

```cpp
bool FSpyMissionCppDefaultsEmptyTest::RunTest(const FString& Parameters)
{
	//# 미션 값은 에디터 DataTable 이 단일 진실이다 (§6-3).
	//# C++ 에서 기본 미션을 채우기 시작하면 두 곳에 진실이 생기므로 비어 있음을 고정한다
	const USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	TestEqual(TEXT("Fresh config has no missions in C++"), Config->GetMissionCount(), 0);
	TestFalse(TEXT("Index 0 is invalid on a fresh config"), Config->IsValidMissionIndex(0));

	//# 로우 구조체 기본값도 함께 고정한다
	const FSpyMissionRow Default;
	TestEqual(TEXT("Default MissionId"), Default.MissionId, 0);
	TestTrue(TEXT("Default mode is Accumulate"), Default.Mode == ESpyMissionMode::Accumulate);
	TestEqual(TEXT("Default target count"), Default.TargetCount, 1);
	TestFalse(TEXT("Default match tag is invalid"), Default.MatchTag.IsValid());
	TestEqual(TEXT("Default NPCId is the system-quest sentinel"), Default.NPCId, 9999);

	//# 보상은 더 이상 FSpyMissionRow 필드가 아니라 MissionRewardTable 관계다 —
	//# 테이블 미지정 상태에서는 "관계 없음"으로 0.f 를 반환해야 한다 (§4-3)
	TestEqual(TEXT("Reward is 0 with no MissionRewardTable"), Config->GetMissionReward(0), 0.f);

	return true;
}
```

**(1-based 전환 관련 확인 — 값 변경 없음)** `Default.MissionId, 0`은 1-based 전환 후에도 **그대로 0**이 맞다 — `MissionId`의 유효 범위는 1~12로 바뀌었지만, 기본 생성자 값 `0`은 "실제 미션이 아닌 미설정 sentinel"로 재해석될 뿐 리터럴 자체는 바뀌지 않는다(실제 코드 `SpyMissionConfig.h`도 `MissionId = 0` 그대로 유지). `Config->IsValidMissionIndex(0)`/`GetMissionReward(0)`도 같은 이유로 미변경.

- [ ] **Step 7: `FSpyMissionInvalidMatchTagTest` — 픽스처 부분만 교체**

현재 코드:

```cpp
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	FSpyMissionEntry Broken;
	Broken.MatchTag = FGameplayTag();
	Broken.Mode = ESpyMissionMode::Accumulate;
	Broken.TargetCount = 2;
	Config->Missions.Add(Broken);

	const FSpyMissionEntry* BrokenEntry = Config->GetMission(0);
```

아래로 교체:

```cpp
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	FSpyMissionRow Broken;
	Broken.MissionId = 1;
	Broken.MatchTag = FGameplayTag();
	Broken.Mode = ESpyMissionMode::Accumulate;
	Broken.TargetCount = 2;
	SpyMissionTests_AddMissionRow(Config, Broken);

	const FSpyMissionRow* BrokenEntry = Config->GetMission(1);
```

(이 함수의 나머지 부분 — `AddError`, `TestFalse`, 이후의 `Design->ResolveMissionProgress(...)` 호출 — 은 그대로 둔다. `SpyMissionTests_MakeDesignConfig()`는 Step 3에서 이미 재작성했다.)

- [ ] **Step 8: 나머지 12개 테스트 케이스 — 변경 불필요 확인**

`FSpyMissionTagMismatchTest`/`FSpyMissionAccumulatePartialTest`/`FSpyMissionAccumulateExactTest`/`FSpyMissionAccumulateOvershootTest`/`FSpyMissionThresholdTest`/`FSpyMissionThresholdNoAccumulateTest`/`FSpyMissionAfterAllCompletedTest`/`FSpyMissionDesignChainTest`/`FSpyMissionDesignTableTest`/`FSpyMissionEventBeforeItsTurnTest`/`FSpyMissionThresholdJumpTest`/`FSpyMissionSameTagAfterCompleteTest`/`FSpyMissionNonPositiveAmountTest`/`FSpyMissionNegativeIndexTest`/`FSpyMissionIndexBoundaryTest`/`FSpyMissionSameTagAfterCompleteTest`는 전부 `SpyMissionTests_MakeConfig()`/`SpyMissionTests_MakeDesignConfig()`가 반환한 `Config`를 받아 `ResolveMissionProgress`/`GetMission`/`GetMissionCount`/`IsValidMissionIndex`/`GetMissionReward`만 호출한다 — 이 함수들의 시그니처(`int32` 인자·반환)가 Task 1에서 안 바뀌었으므로 **이 테스트 함수들의 본문은 단 한 글자도 안 바뀐다.**

- [ ] **Step 9: 컴파일 + 실행 확인**

Task 1·2와 함께 빌드해 컴파일 통과 확인 후, `SkillProject.System.Mission.*`(17개 케이스 전부) Automation 테스트를 실행해 전부 PASS 확인.

- [ ] **Step 10: git add**

```bash
git add SkillProject/Source/SkillProject/System/Tests/SpyMissionTests.cpp
```

---

### Task 4: `System/Tests/SpyNPCDialogueEdgeCaseTests.cpp` — DataTable 기반 픽스처로 재작성

**Files:**
- Modify: `SkillProject/Source/SkillProject/System/Tests/SpyNPCDialogueEdgeCaseTests.cpp:378-589` (미션 레이스 미러 섹션)

**Interfaces:**
- Consumes: `FSpyMissionRow`, `USpyMissionConfig::MissionTable`(Task 1).
- Produces: 없음(테스트 파일).

> 이 파일의 `SpyMissionRaceMirror_*` 함수들은 `USpyMissionComponent::ProcessProgress`/`AddProgress`의 동작을 **미러링(재현)**하는 테스트 전용 코드다 — 프로덕션 로직이 아니다. `Config->GetMission(...)`을 직접 호출하는 부분이 있어 Task 1의 타입 변경 영향을 받는다.

- [ ] **Step 1: 픽스처 헬퍼 신설**

이 파일에 `SpyMissionTests_AddMissionRow`와 동일한 헬퍼가 없다 — 파일 상단(첫 `IMPLEMENT_SIMPLE_AUTOMATION_TEST` 이전)에 추가:

```cpp
//# 테스트 전용 헬퍼 — 완성된 FSpyMissionRow 를 MissionTable 에 추가한다.
//# System/Tests/SpyMissionTests.cpp 의 SpyMissionTests_AddMissionRow 와 동일 패턴
static void SpyMissionRaceMirror_AddMissionRow(USpyMissionConfig* Config, const FSpyMissionRow& Row)
{
	if (Config->MissionTable == nullptr)
	{
		UDataTable* Table = NewObject<UDataTable>();
		Table->RowStruct = FSpyMissionRow::StaticStruct();
		Config->MissionTable = Table;
	}

	Config->MissionTable->AddRow(FName(*FString::Printf(TEXT("Mission_%d"), Row.MissionId)), Row);
}
```

(이 파일에 이미 `Engine/DataTable.h` include가 없다면, `Data/SpyMissionConfig.h`가 이미 그 헤더를 포함하고 있어 전이적으로 사용 가능하다 — 별도 include는 컴파일 에러가 나면 그때 추가한다.)

- [ ] **Step 2: `SpyMissionRaceMirror_ProcessProgress()` — 타입 이름 2곳 교체**

현재 코드(`:385`, `:408` 부근):

```cpp
	if (Result.bCompletedNow)
	{
		const FSpyMissionEntry* CompletedEntry = Config->GetMission(State.MissionIndex);
```

아래로 교체:

```cpp
	if (Result.bCompletedNow)
	{
		const FSpyMissionRow* CompletedEntry = Config->GetMission(State.MissionIndex);
```

그리고:

```cpp
	//# ProcessProgress 자동 수락 조건의 미러 — Dialogue/Interact 둘 다 카드 없는 자동 수락
	//# (SpyMissionComponent.cpp:167-170 과 동일 조건, plan Task 2 Step 1)
	const FSpyMissionEntry* NewEntry = Config->GetMission(State.MissionIndex);
```

아래로 교체:

```cpp
	//# ProcessProgress 자동 수락 조건의 미러 — Dialogue/Interact 둘 다 카드 없는 자동 수락
	//# (SpyMissionComponent.cpp:167-170 과 동일 조건)
	const FSpyMissionRow* NewEntry = Config->GetMission(State.MissionIndex);
```

이 함수의 나머지 로직(`bChanged` 판정, `SpyMissionRaceMirror_AddProgress` 재귀 호출, `State.bAccepted` 대입)은 전부 그대로 둔다.

- [ ] **Step 3: `SpyMissionRaceMirror_MakeConfig()` 재작성**

현재 코드:

```cpp
static USpyMissionConfig* SpyMissionRaceMirror_MakeConfig()
{
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	FSpyMissionEntry Kill;
	Kill.MissionType = ESpyMissionType::Gameplay;
	Kill.MatchTag = SpyGameplayTags::Event_Mission_Kill;
	Kill.Mode = ESpyMissionMode::Accumulate;
	Kill.TargetCount = 1;
	Config->Missions.Add(Kill);

	FSpyMissionEntry Report;
	Report.MissionType = ESpyMissionType::Dialogue;
	Report.MatchTag = SpyGameplayTags::Event_Mission_Report;
	Report.Mode = ESpyMissionMode::Accumulate;
	Report.TargetCount = 1;
	Config->Missions.Add(Report);

	FSpyMissionEntry Level;
	Level.MissionType = ESpyMissionType::Gameplay;
	Level.MatchTag = SpyGameplayTags::Event_Mission_Level;
	Level.Mode = ESpyMissionMode::Threshold;
	Level.TargetCount = 3;
	Config->Missions.Add(Level);

	return Config;
}
```

아래로 교체:

```cpp
static USpyMissionConfig* SpyMissionRaceMirror_MakeConfig()
{
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	FSpyMissionRow Kill;
	Kill.MissionId = 1;
	Kill.MissionType = ESpyMissionType::Gameplay;
	Kill.MatchTag = SpyGameplayTags::Event_Mission_Kill;
	Kill.Mode = ESpyMissionMode::Accumulate;
	Kill.TargetCount = 1;
	SpyMissionRaceMirror_AddMissionRow(Config, Kill);

	FSpyMissionRow Report;
	Report.MissionId = 2;
	Report.MissionType = ESpyMissionType::Dialogue;
	Report.MatchTag = SpyGameplayTags::Event_Mission_Report;
	Report.Mode = ESpyMissionMode::Accumulate;
	Report.TargetCount = 1;
	SpyMissionRaceMirror_AddMissionRow(Config, Report);

	FSpyMissionRow Level;
	Level.MissionId = 3;
	Level.MissionType = ESpyMissionType::Gameplay;
	Level.MatchTag = SpyGameplayTags::Event_Mission_Level;
	Level.Mode = ESpyMissionMode::Threshold;
	Level.TargetCount = 3;
	SpyMissionRaceMirror_AddMissionRow(Config, Level);

	return Config;
}
```

- [ ] **Step 4: `SpyMissionRaceMirror_MakeInteractConfig()` 재작성**

현재 코드:

```cpp
static USpyMissionConfig* SpyMissionRaceMirror_MakeInteractConfig()
{
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	FSpyMissionEntry Kill;
	Kill.MissionType = ESpyMissionType::Gameplay;
	Kill.MatchTag = SpyGameplayTags::Event_Mission_Kill;
	Kill.Mode = ESpyMissionMode::Accumulate;
	Kill.TargetCount = 1;
	Config->Missions.Add(Kill);

	FSpyMissionEntry Interact;
	Interact.MissionType = ESpyMissionType::Interact;
	Interact.MatchTag = SpyGameplayTags::Event_Mission_Interact;
	Interact.Mode = ESpyMissionMode::Accumulate;
	Interact.TargetCount = 1;
	Config->Missions.Add(Interact);

	return Config;
}
```

아래로 교체:

```cpp
static USpyMissionConfig* SpyMissionRaceMirror_MakeInteractConfig()
{
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	FSpyMissionRow Kill;
	Kill.MissionId = 1;
	Kill.MissionType = ESpyMissionType::Gameplay;
	Kill.MatchTag = SpyGameplayTags::Event_Mission_Kill;
	Kill.Mode = ESpyMissionMode::Accumulate;
	Kill.TargetCount = 1;
	SpyMissionRaceMirror_AddMissionRow(Config, Kill);

	FSpyMissionRow Interact;
	Interact.MissionId = 2;
	Interact.MissionType = ESpyMissionType::Interact;
	Interact.MatchTag = SpyGameplayTags::Event_Mission_Interact;
	Interact.Mode = ESpyMissionMode::Accumulate;
	Interact.TargetCount = 1;
	SpyMissionRaceMirror_AddMissionRow(Config, Interact);

	return Config;
}
```

- [ ] **Step 5: `SpyMissionRaceMirror_MakeInteractThenGameplayConfig()` 재작성**

현재 코드:

```cpp
static USpyMissionConfig* SpyMissionRaceMirror_MakeInteractThenGameplayConfig()
{
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	FSpyMissionEntry Interact;
	Interact.MissionType = ESpyMissionType::Interact;
	Interact.MatchTag = SpyGameplayTags::Event_Mission_Interact;
	Interact.Mode = ESpyMissionMode::Accumulate;
	Interact.TargetCount = 1;
	Config->Missions.Add(Interact);

	FSpyMissionEntry Kill;
	Kill.MissionType = ESpyMissionType::Gameplay;
	Kill.MatchTag = SpyGameplayTags::Event_Mission_Kill;
	Kill.Mode = ESpyMissionMode::Accumulate;
	Kill.TargetCount = 1;
	Config->Missions.Add(Kill);

	return Config;
}
```

아래로 교체:

```cpp
static USpyMissionConfig* SpyMissionRaceMirror_MakeInteractThenGameplayConfig()
{
	USpyMissionConfig* Config = NewObject<USpyMissionConfig>();

	FSpyMissionRow Interact;
	Interact.MissionId = 1;
	Interact.MissionType = ESpyMissionType::Interact;
	Interact.MatchTag = SpyGameplayTags::Event_Mission_Interact;
	Interact.Mode = ESpyMissionMode::Accumulate;
	Interact.TargetCount = 1;
	SpyMissionRaceMirror_AddMissionRow(Config, Interact);

	FSpyMissionRow Kill;
	Kill.MissionId = 2;
	Kill.MissionType = ESpyMissionType::Gameplay;
	Kill.MatchTag = SpyGameplayTags::Event_Mission_Kill;
	Kill.Mode = ESpyMissionMode::Accumulate;
	Kill.TargetCount = 1;
	SpyMissionRaceMirror_AddMissionRow(Config, Kill);

	return Config;
}
```

**(1-based 전환)** 위 두 픽스처(Step 4·5)와 뒤이은 테스트 본문의 `State.MissionIndex` 기댓값(`FSpyMissionRaceMirrorState.MissionIndex` 기본값 포함)도 전부 1-based로 갱신해야 한다 — 실제 테스트 코드(`SpyNPCDialogueEdgeCaseTests.cpp`)에 이미 반영됨.

- [ ] **Step 6: 나머지 테스트 함수 — 변경 불필요 확인**

`FSpyMissionRaceWithoutRecheckTest`/`FSpyMissionRaceWithRecheckTest`/`FSpyMissionAutoAcceptInteractTest`/`FSpyMissionAutoAcceptGameplayNotAcceptedTest`(및 그 뒤 이어지는 유사 케이스들)는 위 3개 `Make*Config()` 함수가 반환한 `Config`와 `FSpyMissionRaceMirrorState`만 다루고, `SpyMissionRaceMirror_AddProgress`/`SpyMissionRaceMirror_ProcessProgress`(Step 2에서 이미 재작성)를 통해서만 `Config`에 접근한다 — **본문은 한 글자도 안 바뀐다.**

- [ ] **Step 7: 컴파일 + 실행 확인**

Task 1·2·3과 함께 빌드해 컴파일 통과 확인 후, `SkillProject.System.NPCDialogue.Race.*`, `SkillProject.System.Mission.AutoAccept.*` Automation 테스트 실행해 전부 PASS 확인.

- [ ] **Step 8: git add + 전체 커밋 메시지(안) 제시**

```bash
git add SkillProject/Source/SkillProject/System/Tests/SpyNPCDialogueEdgeCaseTests.cpp
```

Task 1~4가 서로 컴파일 의존이 있어(같은 이유로 Task 5의 interactable-object-mission plan에서도 적용됐던 원칙) 개별 커밋이 아니라 **하나로 묶어 제시**한다:

커밋 메시지(안):
```
[Refactor] USpyMissionConfig — Missions[] DataTable 전환 + NPCId 추가

- FSpyMissionEntry(UDataAsset TArray) → FSpyMissionRow(DataTable) 전환
- MissionId 명시적 필드로 식별(기존 배열 인덱스 암묵 규약 제거)
- NPCId 필드 신설(담당 NPC 조회용, 9999=시스템 퀘스트)
- DT_SpyMissionCommunication 등 나머지 데이터·로직은 무변경
- Automation 테스트 픽스처를 DataTable 기반으로 재작성(케이스 자체는 불변)
```

---

## 완료 후 남는 것 (이 계획 범위 밖, spec §4 "마이그레이션")

- `DT_SpyMission` DataTable 신규 생성 + 기존 12행 데이터 재입력(`MissionId` 1~12, 1-based + `NPCId` 채우기): **사용자**. **(2026-08-03 후속 결정으로 갱신 — 이 작업은 오늘 세션에 이미 완료됐다.)**
- `DA_SpyMissionConfig.MissionTable` 필드에 새 `DT_SpyMission` 연결: **사용자**.
- `DT_SpyMissionCommunication`은 손대지 않으므로 재입력 불필요.
- code-reviewer 검토(cpp-style §14 DataTable 전환 타당성, 타입 리네임 누락 확인) — `/start-develop` 파이프라인으로 진행한다면 Task 4 이후 자동으로 이어진다.
