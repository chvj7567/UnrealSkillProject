# 미션 데이터 DataTable 전환 + NPCId 필드 추가 설계

- **작성일**: 2026-08-03
- **범위**: `USpyMissionConfig::Missions[]`(현재 `UDataAsset`의 `TArray<FSpyMissionEntry>`)를 `UDataTable`(`DT_SpyMission`, row struct `FSpyMissionRow`)로 전환하고, 미션 데이터에서 바로 담당 NPC를 알 수 있도록 `NPCId` 필드를 추가한다.
- **승인 상태**: 브레인스토밍 완료, 사용자 승인 대기
- **선행 시스템**: 미션 시스템(`docs/design/mission-system.md`), NPC 대화 기반 미션 수락(`docs/superpowers/specs/2026-08-01-npc-mission-dialogue-design.md`, `docs/design/npc-mission-dialogue.md`) — 둘 다 이미 구현·커밋된 상태(`431185a`, `0dfe678`). 이 spec은 그 시스템의 **판정 로직·수락 흐름·대사 텍스트는 전혀 바꾸지 않고**, 미션 엔트리를 담는 **저장 방식**만 바꾼다.

---

## 1. 목표

`Missions[]` 배열은 cpp-style.md §14 기준(반복 로우가 있고 밸런싱 때문에 자주 손대는 콘텐츠 데이터는 `UDataTable`)에 정확히 부합하는 데이터인데도, `mission-system.md`가 §14 룰 도입 이전에 만들어져 `UDataAsset` `TArray`로 남아 있다. 이번에 미션 데이터에 `NPCId`(담당 NPC)를 추가하는 김에, 이 grandfathered 상태를 정리해 `DataTable`로 전환한다.

**핵심 동기 2가지**:
1. **배열 위치 = `MissionId`라는 암묵적 규약 제거.** 현재 `USpyMissionComponent`/`USpyMissionConfig`는 "배열 인덱스가 곧 `MissionId`"라는 문서화되지 않은 전제에 의존한다(`docs/superpowers/specs/2026-08-01-npc-mission-dialogue-design.md`의 조사에서도 지적됨). 배열 중간에 미션을 끼워 넣으면 `MissionReward`/`MissionCommunication`의 `MissionId` 참조가 전부 밀린다. `DataTable` 전환과 함께 `MissionId`를 **명시적 필드**로 만들어 이 취약점을 없앤다(단, 실제 값은 기존 배열 인덱스와 동일한 순서를 그대로 쓴다 — 순서 자체는 안 바꾼다. **1~12(1-based) — 2026-08-03 후속 결정(1-based 전환)으로 갱신됨, 초안 작성 시점엔 0~11이었다**).
2. **미션 데이터만 보고 담당 NPC를 알 수 있게.** 지금은 "이 미션이 어느 NPC 담당인지"가 `USpyNPCConfig`의 `MissionCommunicationTable`(NPC 쪽 에셋)에만 있어서, 미션 데이터를 보는 사람이 NPC 쪽 에셋을 따로 열어야 알 수 있다. `FSpyMissionRow`에 `NPCId` 필드를 추가해 미션 쪽에서 바로 보이게 한다.

**비목표 (이번 범위 밖 — 대화 중 검토했다가 명시적으로 기각됨)**
- **NPC당 미션 체인을 2단계(처치→보고)에서 3단계(말 걸기→처치→보고)로 재구성하는 것.** 사용자가 검토 후 취소했다 — 기존 2단계 구조·12행 그대로 유지한다.
- **`FSpyMissionEntry`/`FSpyMissionRow`에 `PreAcceptHintText`(미수락 상태 HUD 안내 텍스트) 필드를 추가/제거하는 것.** 직전 사이클(`/start-develop-quick`)에서 이미 구현·코드리뷰 통과해 스테이징된 필드다 — 이번 spec은 이 필드를 **손대지 않고 그대로 이관**만 한다. **(2026-08-03 후속 결정으로 폐기됨 — 이유: HUD 미수락 안내 문구를 NPCId 기반 동적 조회로 대체, 상세는 `SpyMainHUD::ResolveNPCNameHintText` 참조. 사용자가 이 spec 승인 이후 별도로 새로 지시함)**
- **`ESpyMissionType::Dialogue` 타입이나 F키 프롬프트의 "대화하기" 문구를 없애는 것.** 검토했다가 명시적으로 기각됨.
- **`DT_SpyMissionCommunication`(`Role`/`OfferDialogueId`/`InProgressDialogueId`/`ReportDialogueId`) 테이블을 없애거나 Mission 쪽으로 통합하는 것.** 처음엔 통합안이 나왔으나, 사용자가 "이 필드들은 미션 타입에 따라 조건부로만 쓰이는 관계 데이터라 별도 관계 테이블에 두는 게 cpp-style §14-1 원칙에 맞다"고 판단해 **완전히 그대로 유지**하기로 확정했다. 이 테이블의 필드·구조·값은 한 글자도 안 바뀐다.
- `DT_SpyNPC`, `DT_SpyMissionReward` — 이번 범위 아님, 안 건드림.

**(2026-08-03 후속 결정 — 1-based 전환) 이 spec 초안 전체(§1·§2-3·§3-1·§4)는 `MissionId`/`NPCId`를 0-based(0~11 / 0~5)로 서술한다. 사용자가 이 spec 승인 이후 별도로 "ID는 1부터"를 지시했고, 이미 코드(`SpyMissionComponent.h`/`SpyMissionConfig.cpp`)·리뷰가 1-based(1~12 / 1~6)로 전환된 상태다 — 아래 숫자가 나오는 자리마다 정정 노트를 남겼다. 코드가 정답이며 이 문서는 참조용으로만 갱신했다.**

---

## 2. 조사로 확정된 사실

### 2-1. `Missions[]`는 §14 기준상 DataTable 후보다

`FSpyMissionEntry`(`Data/SpyMissionConfig.h`)는 `ESpyMissionType`/`FGameplayTag`/`ESpyMissionMode`/`int32`/`FText`×3 — **오브젝트 참조가 전혀 없다.** cpp-style.md §14의 판단 기준("반복 로우가 있는 밸런스 콘텐츠, 자주 튜닝되는 수치, 기획자가 직접 편집" = `DataTable`, "에셋 참조 필드가 필요하거나 거의 안 바뀌는 정적 배선" = `DataAsset`)으로 보면 명백히 `DataTable` 쪽이다. `mission-system.md` 최초 구현 시점(§14 도입 전)에 `UDataAsset`으로 만들어진 뒤, 이후 NPC 대화 기능 확장(2026-08-01) 때도 §14 "적용 범위"(기존 코드는 손대는 줄만 변환, 일괄 개조 안 함) 원칙에 따라 건드리지 않고 넘어갔다.

### 2-2. `NPCId`를 Mission 쪽에 추가해도 `ASpyNPCCharacter`의 기존 로직에 영향이 없다

`ASpyNPCCharacter::CacheNPCData()`(`NPC/SpyNPCCharacter.cpp:34-`)는 `NPCConfig->NPCTable`과 `NPCConfig->MissionCommunicationTable`만 스캔한다 — `USpyMissionConfig`나 `Missions[]`를 전혀 참조하지 않는다. `DT_SpyMissionCommunication`을 이번에 그대로 유지하기로 했으므로, 이 함수는 **한 글자도 안 바뀐다.** `ResolveNPCDialogueState`/`USpyMissionComponent::ProcessProgress`/`AddProgress`/`AcceptCurrentMission`도 전부 `MissionId`/`MissionIndex`/`bAccepted` 값만 다루지 `NPCId`를 참조하지 않으므로 마찬가지로 **변경 없음.**

즉 `NPCId` 필드는 순수하게 "미션 데이터를 볼 때 담당 NPC가 바로 보인다"는 조회 편의를 위한 것이고, 어떤 게임플레이 로직도 이 필드를 읽지 않는다. (향후 이 필드를 실제 로직에서 읽고 싶어지면 — 예: NPC 배치 검증 자동화 — 별도 spec에서 다룬다.)

### 2-3. `MissionIndex`를 "그다음 정수"로 전진시키는 기존 산술은 `DataTable` 전환 후에도 그대로 유효하다

`Data/SpyMissionConfig.cpp`의 `ResolveMissionProgress`는 완료 시 `Result.MissionIndex += 1`로 전진시키고, `GetMission(InIndex)`/`IsValidMissionIndex(InIndex)`가 그 정수를 해석한다. `MissionId`가 **밀도 있는 연속 정수**(중간에 구멍 없음)로 유지되는 한, "+1 = 다음 미션"이라는 산술은 배열 인덱스든 `DataTable` 조회 키든 동일하게 성립한다 — **1부터 시작하는 1~12(2026-08-03 후속 결정으로 1-based 전환, 초안 작성 시점엔 0부터 시작하는 0~11이었다).** **이 spec은 `ResolveMissionProgress`의 로직을 한 줄도 바꾸지 않는다** — `GetMission()` 내부 구현만 "배열 인덱스 접근"에서 "`MissionId` 필드로 테이블 스캔"으로 바뀐다.

### 2-4. sentinel 값(`NPCId = 9999`)은 cpp-style §14-1-3의 반대 예시와 형태가 같다 — 사용자가 알고도 선택함

cpp-style.md §14-1-3은 "선택적 관계는 sentinel(`-1`/`0`/`None`) 대신 별도 관계 테이블로 분리한다"고 명시하고, 그 반대 예시로 정확히 `FMyQuestRow.GiverNPCId = -1`(NPC 없으면 -1)을 든다. 이번 `NPCId = 9999`(시스템 퀘스트)는 이 예시와 같은 패턴이다. **사용자에게 이 사실을 명시적으로 알렸고, 그럼에도 실용적 이유(`NPCId`가 항상 0 이상이라 9999와 헷갈릴 여지가 실질적으로 없음, `INDEX_NONE` 같은 흔한 UE 관용구에 준함)로 sentinel 방식을 그대로 쓰기로 확정했다.** 이 결정은 재론하지 않는다.

---

## 3. 데이터 — `DT_SpyMission` (신규 DataTable, `Missions[]` 대체)

### 3-1. `FSpyMissionRow` (신규 row struct)

```cpp
// Data/SpyMissionConfig.h
USTRUCT(BlueprintType)
struct FSpyMissionRow : public FTableRowBase
{
    GENERATED_BODY()

public:
    //# 명시적 식별자 — 기존 배열 인덱스 값과 동일한 순서를 그대로 쓴다(1~12, 1-based —
    //# 2026-08-03 후속 결정으로 갱신, 초안 작성 시점엔 0~11). MissionReward.MissionId /
    //# MissionCommunication.MissionId 와 일치해야 하고, 밀도 있는 연속 정수(구멍 없이)여야
    //# ResolveMissionProgress 의 "+1 = 다음 미션" 산술이 성립한다 (spec §2-3)
    UPROPERTY(EditAnywhere)
    int32 MissionId = 0;

    UPROPERTY(EditAnywhere)
    ESpyMissionType MissionType = ESpyMissionType::Gameplay;

    //# 이 미션이 반응할 이벤트 태그
    UPROPERTY(EditAnywhere)
    FGameplayTag MatchTag;

    UPROPERTY(EditAnywhere)
    ESpyMissionMode Mode = ESpyMissionMode::Accumulate;

    UPROPERTY(EditAnywhere, meta = (ClampMin = "1"))
    int32 TargetCount = 1;

    //# HUD 상시 표시 이름
    UPROPERTY(EditAnywhere)
    FText DisplayName;

    //# 수락 카드 서술문. Gameplay 타입만 사용
    UPROPERTY(EditAnywhere)
    FText Description;

    //# 미수락 상태 HUD 안내(기존 필드, 이번엔 안 건드리고 그대로 이관)
    UPROPERTY(EditAnywhere)
    FText PreAcceptHintText;

    //# 이 미션을 담당하는 NPC. 9999 = 시스템 퀘스트(NPC 없음). sentinel 값 — §2-4 참조.
    //# 어떤 게임플레이 로직도 이 필드를 읽지 않는다 — 순수 조회 편의용이다.
    UPROPERTY(EditAnywhere)
    int32 NPCId = 9999;
};
```

**(2026-08-03 후속 결정으로 위 코드 예시 중 `PreAcceptHintText` 필드는 폐기됨 — §1 비목표 정정 노트 참조. 실제 구현은 `NPCId`만 남기고 이름 조회는 `SpyMainHUD::ResolveNPCNameHintText`로 이관했다.)**

`FSpyMissionEntry`(기존 struct)는 **삭제**한다 — `FSpyMissionRow`가 이를 완전히 대체한다.

### 3-2. `USpyMissionConfig` 변경

```cpp
// Data/SpyMissionConfig.h — 변경분만
UCLASS()
class SKILLPROJECT_API USpyMissionConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    //# Missions(TArray<FSpyMissionEntry>) 대체 — RowStruct = FSpyMissionRow
    UPROPERTY(EditDefaultsOnly, Category = "Mission")
    TObjectPtr<UDataTable> MissionTable;

    //# MissionRewardTable — 변경 없음, 그대로 유지

public:
    //# 반환 타입이 FSpyMissionEntry* → FSpyMissionRow* 로 바뀐다. 호출부(SpyMissionComponent,
    //# ASpyNPCCharacter 의 GetMissionEntry 경유 호출)는 시그니처만 맞추면 되고 로직 변경 없음
    const FSpyMissionRow* GetMission(int32 InMissionId) const;

    UFUNCTION(BlueprintPure, Category = "Mission")
    int32 GetMissionCount() const;

    UFUNCTION(BlueprintPure, Category = "Mission")
    bool IsValidMissionIndex(int32 InMissionId) const;

    //# ResolveMissionProgress 시그니처·로직은 변경 없음 — 내부에서 GetMission() 호출 방식만 바뀐다
    UFUNCTION(BlueprintPure, Category = "Mission")
    FSpyMissionProgressResult ResolveMissionProgress(int32 InIndex, int32 InCount, FGameplayTag InEventTag, int32 InAmount) const;

    UFUNCTION(BlueprintPure, Category = "Mission")
    float GetMissionReward(int32 InMissionId) const;
};
```

`GetMission()`/`GetMissionCount()`/`IsValidMissionIndex()` 구현은 `MissionTable->GetAllRows<FSpyMissionRow>(...)` 후 `MissionId` 필드로 선형 스캔한다 — `GetMissionReward()`(기존, `MissionRewardTable` 스캔)와 정확히 같은 패턴이다. 로우 수가 12개뿐이라 성능 문제 없고, `USpyMissionComponent::AddProgress`(미션 진행 이벤트마다 1회)에서만 호출되므로 핫패스도 아니다.

### 3-3. 변경 없는 것 (명시적으로 재확인)

- `DT_SpyMissionCommunication`(`FSpyMissionCommunicationRow`: `MissionId`/`NPCId`/`Role`/`OfferDialogueId`/`InProgressDialogueId`/`ReportDialogueId`) — **완전히 그대로.**
- `DT_SpyMissionReward`(`FSpyMissionRewardRow`) — 그대로.
- `DT_SpyNPC`(`FSpyNPCRow`) — 그대로.
- `ASpyNPCCharacter::CacheNPCData()`, `RequestInteract()`, `ResolveNPCDialogueState()` — 전부 그대로(§2-2).
- `USpyMissionComponent::ProcessProgress()`/`AddProgress()`/`AcceptCurrentMission()`/`GrantReward()` — 전부 그대로. 내부에서 `MissionConfig->GetMission(...)`을 호출하는 지점은 그대로 두되, 반환 타입이 `FSpyMissionEntry*` → `FSpyMissionRow*`로 바뀌므로 그 타입 이름만 따라간다.

---

## 4. 마이그레이션 (사용자 몫)

1. `DT_SpyMission` DataTable 신규 생성(RowStruct = `FSpyMissionRow`).
2. 기존 `DA_SpyMissionConfig`의 `Missions[]` 12행 값을 그대로 옮겨 입력(`MissionId` = 기존 배열 순서와 동일한 1~12, 1-based — **2026-08-03 후속 결정으로 갱신, 초안 작성 시점엔 0~11이었다**. 나머지 필드 값 동일).
3. `docs/design/npc-mission-dialogue.md` §3-4(`MissionCommunication` 12행 표)를 참고해 각 `MissionId`에 대응하는 `NPCId`를 채운다(예: `MissionId 1`/`2` → `NPCId 1`(레이븐), `MissionId 3`/`4` → `NPCId 2`(팰컨) ...). `Interact`/시스템 퀘스트가 있다면 `9999`.
4. `DA_SpyMissionConfig`의 `MissionTable` 필드에 새로 만든 `DT_SpyMission`을 연결.
5. `DT_SpyMissionCommunication`은 **손대지 않는다** — 이미 있는 값 그대로 둔다.

---

## 5. 테스트 (Unreal Automation)

기존 `System/Tests/SpyMissionTests.cpp`가 `USpyMissionConfig::ResolveMissionProgress`/`GetMission`/`GetMissionCount`/`IsValidMissionIndex`/`GetMissionReward`를 픽스처(`SpyMissionTests_MakeConfig`, `SpyMissionTests_MakeDesignConfig` — `NewObject<USpyMissionConfig>()` + 코드로 `Missions.Add(...)`)로 광범위하게 테스트하고 있다. **이 픽스처들은 전부 `Missions.Add(FSpyMissionEntry)` 방식이라 `DataTable` 전환 후 그대로 컴파일되지 않는다** — `NewObject<UDataTable>()` + `Table->RowStruct = FSpyMissionRow::StaticStruct()` + `Table->AddRow(...)` 방식으로 재작성해야 한다(정확히 같은 파일의 `SpyMissionTests_AddReward` 헬퍼가 이미 이 패턴을 쓰고 있다 — 그대로 따라 하면 된다).

기존 테스트 케이스(태그 불일치, Accumulate 부분/정확/초과, Threshold, 빈 config, 계층 태그, 인덱스 경계, 음수 인덱스 등 총 17개 `IMPLEMENT_SIMPLE_AUTOMATION_TEST`)는 **전부 그대로 유지**하되 픽스처 헬퍼만 `DataTable` 기반으로 교체한다 — 이게 이번 spec의 실질적인 회귀 테스트 범위다(로직이 안 바뀌었다는 걸 기존 케이스들이 여전히 통과하는 것으로 증명).

신규 케이스 후보:
- `NPCId` 필드가 `DataTable`에서 정상적으로 읽히는지(기본값 `9999` 포함)
- `MissionId`가 `DataTable`의 `RowName`이 아니라 필드 값 기준으로 조회되는지(RowName과 MissionId가 일부러 다른 픽스처로 검증)

---

## 6. 변경 파일 목록

**수정**
- `SkillProject/Source/SkillProject/Data/SpyMissionConfig.h` — `FSpyMissionEntry` 삭제, `FSpyMissionRow` 신설(`NPCId` 포함), `USpyMissionConfig::Missions` → `MissionTable`
- `SkillProject/Source/SkillProject/Data/SpyMissionConfig.cpp` — `GetMission`/`GetMissionCount`/`IsValidMissionIndex` 구현을 배열 접근 → `DataTable` 스캔으로 교체
- `SkillProject/Source/SkillProject/System/Tests/SpyMissionTests.cpp` — 픽스처 헬퍼를 `DataTable` 기반으로 재작성(로직·케이스 자체는 불변)

**변경 없음(명시적 확인 — §3-3)**
- `System/SpyMissionComponent.h/.cpp`, `NPC/SpyNPCCharacter.h/.cpp`, `Data/SpyNPCDialogueRow.h/.cpp`, `ManagerComponent/SpyInteractionComponent.h/.cpp`

**에셋(사용자, §4)**
- `DT_SpyMission` 신규 생성 + 12행 입력, `DA_SpyMissionConfig.MissionTable` 연결
