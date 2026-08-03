# NPC 대화 기반 미션 수락 시스템 설계

- **작성일**: 2026-08-01 (개정: 2026-08-01c — "보고"를 미션 체인 자체로 확장 + 5테이블 정규화 스키마)
- **범위**: NPC 근접 상호작용 → 대화 UI → 미션 수락 → (목표 달성 후) 같은 NPC에게 복귀해 대화로 보고 → 보상 지급 + 다음 미션 노출. 기존 순차 미션 6종(`docs/superpowers/specs/2026-07-22-mission-system-design.md`, `docs/design/mission-system.md`)에 "수락 + 보고" 게이트를 추가한다.
- **승인 상태**: 브레인스토밍 완료, 사용자와 반복 확정한 데이터 구조 위에서 재작성 — 사용자 검토 대기
- **선행 시스템**: 미션 시스템(`docs/design/mission-system.md`, 구현 완료) — 이 spec 이 그 "비목표" 항목(§9의 "미션 수락 / 포기", "NPC 미션 제공자")을 뒤집는다. **mission-system.md §3-1 의 6종 수치(`DisplayName`/`MatchTag`/`Mode`/`TargetCount`/`ExperienceReward`)는 값 그대로 입력으로 쓴다 — 이 spec은 그 값을 바꾸지 않고, 그 값을 담는 배열 구조와 보상 저장 위치만 바꾼다** (§4-6에서 정합성을 확인한다).

**개정 사유(2026-08-01c)**: 2026-08-01b(단일 미션에 "보고" 서브상태를 붙이는 모델)까지 구현이 진행되던 중, "처치 미션이 끝나면 보상 없이 다음 항목으로 넘어가고, 그다음 항목이 '레이븐에게 보고하십시오'라는 별도 미션이며 그 항목이 진짜 보상을 들고 있다"는 방향으로 설계가 바뀌었다. 이어서 NPC/대화/미션 데이터를 사용자와 함께 정규화 원칙(핵심 엔티티 분리 → 선택적/필수 관계 분리 → 복합 키 → 관계 테이블 명명)에 따라 반복 설계한 끝에 **Mission / MissionReward / MissionCommunication / NPC / Dialogue** 5테이블 구조로 수렴했다(이 절차 자체는 이제 `cpp-style.md` §14-1 로 프로젝트 공통 규칙이 됐다). 이 개정판은 그 최종 구조 위에서 spec 전체를 다시 쓴다 — 2026-08-01b 의 `bObjectiveMet`/`ReportCurrentMission()`/`ReadyToReport`/`FSpyNPCDialogueRow`(5줄) 는 전부 폐기한다.

---

## 1. 목표

플레이어가 NPC에게 근접해 상호작용하면 대화창이 뜨고, 그 NPC가 담당하는 미션이 아직 시작 전이면 수락/거절 카드를 통해 미션을 받는다. 목표를 달성하면 **미션 배열의 다음 항목이 자동으로 "그 NPC에게 보고하십시오"라는 별도 미션으로 노출된다** — 이 보고 미션은 별도 수락 절차 없이 즉시 활성 상태이며, 같은 NPC에게 다시 말을 걸어 대화하면(추가 확인 카드 없이) 그 자리에서 완료되어 보상이 지급되고 다음 미션(다음 NPC의 수락 대상)이 열린다.

**핵심 전환 (2026-08-01b 대비)**: "보고"는 한 미션의 서브상태가 아니라 **미션 배열의 독립된 항목**이다. 처치·레벨·콤보 같은 행동형(`Gameplay`) 미션은 완료돼도 보상이 없다 — 보상은 항상 그 뒤에 오는 대화형(`Dialogue`) 미션이 완료될 때 지급된다. `USpyMissionComponent` 는 결과적으로 **2026-08-01b 이전(순수 자동 진행 + 단일 `bAccepted` 게이트)에 가까운 형태로 되돌아간다** — `bObjectiveMet`/`ReportCurrentMission()`은 없다. 대신 미션 타입에 따라 수락이 수동(Gameplay)이거나 자동(Dialogue)이라는 점만 새로 추가된다.

**비목표 (이번 범위 밖)**
- 대화 분기·선택지 — 대사는 상태별로 1개씩만 출력되는 선형 구조
- NPC AI 행동(순찰·전투 등) — 이번 범위는 상호작용·대사·미션 연동만
- 대화 로그·히스토리 저장, 보이스/립싱크
- 미션 수락/진행 이력의 세이브/로드 영속화 (선행 spec과 동일한 이유 — 세이브 시스템 자체가 없다)
- 거절이나 보고를 미루는 것에 대한 페널티나 카운트 제한
- Threshold 모드 미션 일반화 — §5-5의 "수락 시점 재평가"는 현재 유일한 Threshold 사례(레벨 미션)에 한정한 특화 처리다. 범용 상태 샘플러 프레임워크는 만들지 않는다 (YAGNI)
- 대화 다회차(멀티라인 연출) — `Dialogue` 테이블은 `DialogueId`+`DialogueIndex` 복합 키로 여러 줄을 지원할 수 있게 설계하지만, **이번 범위의 실제 데이터는 전부 `DialogueIndex = 0` 한 줄만 채운다.** 여러 줄 재생 연출(순차 표시·입력 대기)은 만들지 않는다

---

## 2. 조사로 확정된 사실

### 2-1. 현재 미션은 "수락" 개념이 없다

`Source/SkillProject/System/SpyMissionComponent.cpp` — `AddProgress()`(92행)는 `HasAuthority()`와 `MissionConfig` null 체크만 하고 바로 `ProcessProgress()`로 진행한다. GA가 신호를 보내는 순간 무조건 카운트되고, 목표 도달 즉시 보상까지 지급된다(`ProcessProgress` → `GrantReward`, 같은 프레임). `FSpyMissionState`는 `MissionIndex`/`Count` 두 필드뿐이다.

### 2-2. NPC/대화/상호작용 관련 기존 코드는 전혀 없다

`**/*NPC*`, `**/*Dialogue*`, `**/*Interact*` 패턴 매칭 파일이 (이 기능 이전) 프로젝트 전체에 없었다.

### 2-3. `ResolveMissionProgress` 순수 함수는 미션 타입을 모른다 — 그래서 그대로 재사용 가능하다

`Data/SpyMissionConfig.cpp:26-73` 을 확인했다. 이 함수는 `MatchTag`/`Mode`/`TargetCount`만 보고 완료를 판정한다 — `Gameplay`/`Dialogue` 어느 쪽이든 `MatchTag`가 맞고 `Count >= TargetCount`면 완료로 처리하고 인덱스를 그대로 +1 한다. **이 spec 전체가 이 사실에 의존한다**: "보고"를 완료시키는 것도 결국 `AddProgress(Event_Mission_Report, 1)` 라는 평범한 진행 신호 하나일 뿐이고, 순수 함수는 이를 처치·콤보 이벤트와 똑같이 처리한다. `ResolveMissionProgress` 는 **한 줄도 바꾸지 않는다.**

### 2-4. 프로젝트에 `UDataTable` row struct 사용 사례가 없다

`FTableRowBase` 상속 검색 결과 0건(이 기능 이전). `cpp-style.md` §14/§14-1 이 이 기능을 계기로 추가됐고, 이 기능이 프로젝트 최초의 `UDataTable` 저작 사례가 된다.

### 2-5. 인터페이스 파일명 컨벤션 확인

`Source/SkillProject/Character/CommonInterface.Character.h`(`ISpyCharacterRoot`), `ManagerComponent/CommonInterface.Manager.h`(§12 도메인별 분할 패턴)가 이미 존재한다. NPC는 별도 도메인이므로 별도 basename(`CommonInterface.NPC.h`)으로 분리한다.

### 2-6. 이전 판(2026-08-01b)이 겪었던 "레벨 신호 소실" 문제는 이번 구조에서도 그대로 존재하고, 해법도 그대로 유효하다

레벨 미션(`Threshold`, `Event.Mission.Level`)은 승급이 실제로 일어나는 프레임에만 발신되는 **엣지 이벤트**다. 처치 미션(index 0)이 끝난 뒤에도 인덱스는 곧바로 레벨 미션으로 가지 않는다 — 그 사이에 "레이븐에게 보고하십시오"(index 1, `Dialogue`)가 끼고, 그다음 "레벨 3 달성"(index 2)이 **NPC1(팰컨)의 Offer 로 수락되기 전까지** 여전히 `bAccepted == false` 라 이벤트를 버린다. 즉 **처치 완료 → 레이븐에게 보고 이동 → 팰컨에게 이동해 수락**, 이 전체 구간 동안 레벨업이 먼저 일어나면 그 신호는 영구히 사라진다. §5-5 에서 이 문제를 그대로 재사용해 해소한다 — **재평가는 "수락 시점의 스냅샷 읽기"라 그 앞에 게이트가 몇 개 끼든 무관하게 항상 정확하다.**

---

## 3. 배치 — 신규 `NPC/` 도메인

기존 최상위 폴더(`AbilitySystem`/`Character`/`Data`/`Input`/`Manager`/`ManagerComponent`/`System`/`AI`/`Item`/`UI`/`Util`) 중 NPC 액터가 속할 자리가 없으므로 `Item/`과 동급으로 `Source/SkillProject/NPC/`를 신설한다.

```
NPC/
├── SpyNPCCharacter.h|.cpp       # 도메인 루트 (cpp-style §13)
└── CommonInterface.NPC.h        # ISpyNPCRoot, FSpyNPCDialogueResult (cpp-style §12)
```

### 3-1. `ASpyNPCCharacter`

- `AModularCharacter` 상속 (unreal-infra §3 — 게임 액터는 Modular* 베이스).
- 하위 구성 요소는 상호작용 감지용 `USphereComponent` 하나뿐이라 §13 "하위 1개면 루트 파사드 생략 가능" 예외 대상이지만, **외부 소비자(플레이어 상호작용 컴포넌트)를 위해 인터페이스는 그대로 뺀다** (§13 "소비자 없어도 미리 뺀다").
- `EditDefaultsOnly` 프로퍼티:
  - `int32 NPCId` — §4 `NPC` 테이블의 행 식별자이자 `MissionCommunication.NPCId` 매칭 키. **매직 넘버 아님** — 데이터 룩업 키로 확정.
  - `TObjectPtr<USpyNPCConfig> NPCConfig` — §4-5 의 3개 DataTable(NPC/Dialogue/MissionCommunication) 참조를 묶은 허브 DataAsset.
- `BeginPlay`에서 `NPCId`로 3개 테이블을 각 1회 스캔해 캐싱한다(§8 — 매 프레임/이벤트 핸들러 조회 금지):
  - `NPC` 테이블에서 `NPCId` 행 → `CachedNPCDisplayName`, `CachedDefaultLine`(`DefaultDialogueId` → `Dialogue` 테이블 조회)
  - `MissionCommunication` 테이블에서 `NPCId == 이 NPC` 인 행을 스캔 → **정확히 2개**(`Role == Offer` 1개, `Role == Report` 1개)를 기대하고 `CachedOfferMissionId`/`CachedOfferLine`/`CachedInProgressLine`/`CachedReportMissionId`/`CachedReportLine` 를 채운다.

```cpp
// NPC/CommonInterface.NPC.h
UINTERFACE(MinimalAPI)
class USpyNPCRoot : public UInterface { GENERATED_BODY() };

class ISpyNPCRoot
{
    GENERATED_BODY()

public:
    //# 서버 권한에서 상호작용 요청을 판정한다. 상태가 Report 면 이 호출 안에서
    //# AddProgress(Event_Mission_Report, 1) 까지 함께 처리한다 (§7).
    virtual FSpyNPCDialogueResult RequestInteract(APlayerController* Requester) = 0;

    virtual int32 GetNPCId() const = 0;
};
```

### 3-2. 플레이어 측 — `USpyInteractionComponent` (신규)

- `ASpyCharacter`에 부착. `ASpyCharacter`는 이미 §13 루트 파사드이므로 이 컴포넌트도 `private` 소유 + `ISpyCharacterRoot::GetInteractionHost()`로만 노출.
- NPC의 `USphereComponent` 오버랩이 발생하면, NPC가 `OtherActor`를 `TScriptInterface<ISpyCharacterRoot>`로 캐스팅해 `GetInteractionHost()`를 얻고, `ISpyInteractionHost::NotifyNPCRangeChanged(this, bool)`를 직접 호출한다 (§8 컴포넌트 탐색 지양 — 이벤트 핸들러에서 `FindComponentByClass` 안 씀).
- `ISpyInteractionHost`는 `ManagerComponent/CommonInterface.Manager.h`에 추가 (§12 — 기존 `ISpyParkourHost`/`ISpyGrappleHost`와 같은 파일).
- Enhanced Input의 `IA_Interact`(F 키)가 눌리면 현재 범위 안의 NPC를 대상으로 `Server_RequestInteract(AActor* TargetNPC)` RPC를 보낸다.
- **GAS를 거치지 않는다** — 상호작용은 어빌리티가 아니므로 `AbilityLocalInputPressed` 경로를 쓰지 않고 Enhanced Input에서 직접 함수 바인딩한다.

```cpp
// ManagerComponent/CommonInterface.Manager.h 에 추가
UINTERFACE(MinimalAPI)
class USpyInteractionHost : public UInterface { GENERATED_BODY() };

class ISpyInteractionHost
{
    GENERATED_BODY()

public:
    virtual void NotifyNPCRangeChanged(AActor* NPCActor, bool bInRange) = 0;
};
```

---

## 4. 데이터 — 5테이블 정규화 스키마

`cpp-style.md` §14-1 절차를 그대로 따른 결과다. 핵심 엔티티(Mission/NPC/Dialogue) 3개와, 그 엔티티들의 **선택적 관계**를 담는 관계 테이블(MissionReward/MissionCommunication) 2개로 나뉜다.

### 4-1. 구조 트리

```
Mission (엔티티, 관계 없음 — Missions[] 배열, MissionId = 배열 인덱스)
├── MissionReward           (Mission 의 선택적 관계 — Dialogue 타입 행에만 존재)
└── MissionCommunication    (Mission 의 선택적 관계 — 모든 행에 정확히 1개씩 존재)
        │
        ├── → NPC           (엔티티, 관계 없음 — 자신의 DefaultDialogueId 만 예외)
        │       └── → Dialogue   (엔티티, 관계 없음 — 복합 키)
        │
        └── → Dialogue       (OfferDialogueId / InProgressDialogueId / ReportDialogueId)
```

### 4-2. `Mission` — 핵심 엔티티 (`USpyMissionConfig::Missions[]`, 변경)

**이 테이블 하나가 무엇을 표현하는가**: 순차 진행되는 미션 1개.

`Data/SpyMissionConfig.h` 의 기존 `FSpyMissionEntry` 를 아래로 바꾼다.

```cpp
//# Util/DefineEnum.h 에 추가 (2개 이상 시스템 참조 — MissionComponent + NPC, cpp-style §11)
UENUM(BlueprintType)
enum class ESpyMissionType : uint8
{
    //# 행동으로 진행되는 미션 (처치/레벨/콤보/파쿠르 등). 수락은 NPC Offer 카드로 수동
    Gameplay,

    //# NPC 보고로 완료되는 미션. 배열에 진입하는 즉시 자동 수락된다 (§5-2)
    Dialogue,
};
```

```cpp
//# 미션 1개의 정의
USTRUCT(BlueprintType)
struct FSpyMissionEntry
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, Category = "Mission")
    ESpyMissionType MissionType = ESpyMissionType::Gameplay;

    //# 이 미션이 반응할 이벤트 태그. Dialogue 타입은 전부 공용 Event_Mission_Report 를 쓴다 (§7 안전성 근거)
    UPROPERTY(EditDefaultsOnly, Category = "Mission")
    FGameplayTag MatchTag;

    UPROPERTY(EditDefaultsOnly, Category = "Mission")
    ESpyMissionMode Mode = ESpyMissionMode::Accumulate;

    UPROPERTY(EditDefaultsOnly, Category = "Mission", meta = (ClampMin = "1"))
    int32 TargetCount = 1;

    //# HUD 상시 표시 이름. Dialogue 타입은 이 값 자체가 "시스템 메시지"다 (예: "레이븐에게 보고하십시오")
    UPROPERTY(EditDefaultsOnly, Category = "Mission")
    FText DisplayName;

    //# 수락 카드 서술문. Gameplay 타입만 사용 — Dialogue 타입은 카드 자체가 없으므로 비워 둔다 (§7)
    UPROPERTY(EditDefaultsOnly, Category = "Mission")
    FText Description;
};
```

**`ExperienceReward` 필드는 삭제한다** — §4-3 `MissionReward` 로 이동. §14-1-3 "필수 1:1 관계"가 아니라 "선택적 관계"이기 때문이다: Gameplay 타입 행은 보상이 **아예 없다**(0으로 채우는 게 아니라 행 자체가 없다).

### 4-3. `MissionReward` — 관계 테이블 (신규 `UDataTable`, `Mission` 의 선택적 관계)

**핵심 엔티티**: 없음 — `Mission` 에 대한 "보상이 있다"는 관계 하나만 표현한다. **명명 규칙(§14-1-5)**: `Mission_Reward`.

```cpp
// Data/SpyMissionConfig.h 에 추가
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

- `USpyMissionConfig` 에 `TObjectPtr<UDataTable> MissionRewardTable`(RowStruct = `FSpyMissionRewardRow`) 필드와 `float GetMissionReward(int32 InMissionId) const` 접근자를 추가한다. 구현은 `MissionRewardTable->GetRowMap()` 선형 스캔 — 행 수가 6개뿐이라 `RowName` 을 `MissionId` 문자열로 강제하는 것보다 필드 스캔이 더 단순하고, `GrantReward()`(미션 완료당 1회)에서만 호출되므로 핫패스가 아니다.
- 행이 없으면(Gameplay 타입 미션의 `MissionId`) `0.f` 를 반환한다 — **이건 sentinel 이 아니라 "관계 없음"의 정상적인 부재 결과**다. `GrantReward()`는 기존처럼 `Reward <= 0.f` 면 GE 를 적용하지 않고 조용히 반환한다(무해).
- **⚠ 단, `GrantReward()`는 완료된 미션이 `MissionType == Dialogue` 인데 `Reward <= 0.f` 인 경우만 경고 로그를 남긴다** — 이 조합은 "Gameplay라 보상이 없다"(정상)가 아니라 "Dialogue인데 `MissionReward` 행을 빠뜨렸다"(에디터 데이터 실수)일 수밖에 없다. 두 경우 모두 `GetMissionReward()`가 똑같이 `0.f`를 반환해 구분이 안 되므로, `GrantReward()` 안에서 `MissionType`을 함께 확인해 후자만 골라 경고한다(design-reviewer 지적 — 기존 `bWarnedMissingConfig` 패턴과 동일하게 1회만 남긴다).

### 4-4. `NPC` — 핵심 엔티티 (신규 `UDataTable`)

**이 테이블 하나가 무엇을 표현하는가**: NPC 1명.

```cpp
// Data/SpyNPCDialogueRow.h
USTRUCT(BlueprintType)
struct FSpyNPCRow : public FTableRowBase
{
    GENERATED_BODY()

public:
    //# 이 NPC의 식별자. MissionCommunication.NPCId 매칭 키이자, ASpyNPCCharacter가
    //# BeginPlay에 NPCTable을 스캔해 "내 행"을 찾는 필터 — 다른 두 관계 테이블과
    //# 동일하게 "전체 스캔 + 필드 비교" 접근 방식으로 통일한다
    UPROPERTY(EditAnywhere)
    int32 NPCId = 0;

    //# 라벨 — 대사가 아니므로 Dialogue 를 타지 않고 직접 FText 로 둔다 (§14-1-3 필수 1:1과는 다른 이유:
    //# 이건 애초에 "관계"가 아니라 엔티티 고유 속성이다)
    UPROPERTY(EditAnywhere)
    FText NPCDisplayName;

    //# 필수 1:1 관계 — 모든 NPC가 항상 정확히 하나씩 갖는다(현재 미션과 무관할 때 보여줄 대사).
    //# §14-1-3에 따라 관계 테이블 없이 직접 FK 필드로 둔다
    UPROPERTY(EditAnywhere)
    int32 DefaultDialogueId = 0;
};
```

**NPC 데이터에는 NPC 고유 정보만 있다** — 어떤 미션을 담당하는지, 어떤 대사가 Offer/Report 인지는 전부 `MissionCommunication`(미션 쪽 관계)에 있다. NPC 자신은 "내가 무슨 미션과 엮여 있는지" 모른다 — `MissionCommunication.NPCId` 로 역참조될 뿐이다.

### 4-5. `Dialogue` — 핵심 엔티티, 복합 키 (신규 `UDataTable`)

**이 테이블 하나가 무엇을 표현하는가**: 대사 한 마디(또는 여러 줄로 구성된 한 그룹).

```cpp
// Data/SpyNPCDialogueRow.h 에 추가
USTRUCT(BlueprintType)
struct FSpyDialogueRow : public FTableRowBase
{
    GENERATED_BODY()

public:
    //# 그룹 식별 — 복합 키(§14-1-4). 한 대사가 여러 줄로 이어질 수 있어 단일 ID로는 부족하다
    UPROPERTY(EditAnywhere)
    int32 DialogueId = 0;

    //# 그룹 내 순서
    UPROPERTY(EditAnywhere)
    int32 DialogueIndex = 0;

    UPROPERTY(EditAnywhere)
    FText Text;
};
```

- 조회: `DialogueId` 로 필터 → `DialogueIndex` 오름차순 정렬 → `Text` 를 이어붙여 하나의 `FText` 로 합성한다. 이번 범위(§1 비목표)의 실제 데이터는 전부 `DialogueIndex = 0` 하나뿐이라 사실상 1:1 조회와 동일하게 동작하지만, 스키마는 향후 멀티라인을 위해 복합 키로 미리 열어 둔다.
- 이 합성 로직은 부수효과가 없으므로 `FText ResolveDialogueText(const UDataTable* InDialogueTable, int32 InDialogueId)` 자유 함수로 분리해 Automation 테스트 대상으로 삼는다(§11).

### 4-6. `MissionCommunication` — 관계 테이블 (신규 `UDataTable`, `Mission` 의 필수 관계 · `NPC`/`Dialogue` 로의 FK)

**핵심 엔티티**: 없음 — "이 미션은 이 NPC와 이런 방식(Offer/Report)으로 엮여 있다"는 관계.

```cpp
// Data/SpyNPCDialogueRow.h 에 추가
UENUM(BlueprintType)
enum class ESpyMissionCommRole : uint8
{
    Offer,
    Report,
};

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
```

**⚠ §14-1-3 예외를 의도적으로 남겨 둔다**: `Role` 에 따라 3개 `DialogueId` 필드 중 일부가 미사용으로 남는 것은 형태상 "선택적 관계를 sentinel로 채운" 모양과 비슷해 보이지만, 다른 이유다 — `Role` 은 값이 없는 관계의 부재 표시가 아니라 **행 자체의 종류를 가르는 판별자(태그드 유니온)**이고, 이 구조는 사용자와의 반복 설계 끝에 이미 확정됐다(2026-08-01 대화 기록). 앞으로 유사 사례를 설계할 때는 `Mission_Offer`/`Mission_Report` 두 테이블로 쪼개는 편(§14-1-5 명명)이 §14-1-3 원칙에 더 가깝다는 점을 남겨 둔다 — 이 테이블 자체를 지금 다시 쪼개지는 않는다.

- 모든 `Mission` 행은 이 테이블에 **정확히 1개**의 관계 행을 갖는다(Gameplay 타입은 `Role = Offer`, Dialogue 타입은 `Role = Report`) — "선택적"이라 부르는 이유는 §14-1-3 정의(일부 행에만 존재)가 아니라 **다른 엔티티(NPC)를 참조하는 FK 이기 때문**이다(순수하게 `Mission` 만 보면 이 정보는 `Mission` 테이블에 없다).
- **명명 규칙(§14-1-5)**: `Mission_Communication`.

### 4-7. `USpyNPCConfig` — DataAsset 허브 (신규)

3개 `UDataTable` 참조를 묶는다. `USpyMissionConfig` 와 같은 역할(§14 예외 — 오브젝트 참조 조립).

```cpp
// Data/SpyNPCDialogueRow.h 에 추가
UCLASS()
class SKILLPROJECT_API USpyNPCConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, Category = "NPC")
    TObjectPtr<UDataTable> NPCTable;            //# RowStruct = FSpyNPCRow

    UPROPERTY(EditDefaultsOnly, Category = "NPC")
    TObjectPtr<UDataTable> DialogueTable;       //# RowStruct = FSpyDialogueRow

    UPROPERTY(EditDefaultsOnly, Category = "NPC")
    TObjectPtr<UDataTable> MissionCommunicationTable; //# RowStruct = FSpyMissionCommunicationRow
};
```

NPC 블루프린트 6종은 이 허브 DataAsset **하나**만 참조하면 된다(개별 NPC가 3개 테이블을 따로 지정하지 않는다 — 무증상 실패 지점을 줄인다).

### 4-8. 12행 미션 체인 — 예시 데이터 (실제 값은 §데이터 구조 확인 게이트에서 사용자 승인, `docs/design/`이 최종 확정)

`mission-system.md` §3-1 의 6종 수치를 그대로 입력으로 쓰고, 각 뒤에 보고 미션 하나씩을 끼워 12행으로 확장한 예시다(값은 game-designer 가 `docs/design/npc-mission-dialogue.md` 에서 재확인 후 확정).

| MissionId | MissionType | DisplayName | MatchTag | Mode | TargetCount | Description |
|---|---|---|---|---|---|---|
| 0 | Gameplay | 적 1명 처치 | `Event.Mission.Kill` | Accumulate | 1 | (mission-system.md §3-1 그대로) |
| 1 | Dialogue | 레이븐에게 보고하십시오 | `Event.Mission.Report` | Accumulate | 1 | (미사용) |
| 2 | Gameplay | 레벨 3 달성 | `Event.Mission.Level` | Threshold | 3 | (mission-system.md §3-1 그대로) |
| 3 | Dialogue | 팰컨에게 보고하십시오 | `Event.Mission.Report` | Accumulate | 1 | (미사용) |
| 4 | Gameplay | 콤보 4회 연결 | `Event.Mission.Combo` | Accumulate | 4 | (mission-system.md §3-1 그대로) |
| 5 | Dialogue | 바이퍼에게 보고하십시오 | `Event.Mission.Report` | Accumulate | 1 | (미사용) |
| 6 | Gameplay | 장애물 넘기 5회 | `Skill.Move.Vault` | Accumulate | 5 | (mission-system.md §3-1 그대로) |
| 7 | Dialogue | 스패로우에게 보고하십시오 | `Event.Mission.Report` | Accumulate | 1 | (미사용) |
| 8 | Gameplay | 벽 타기 3회 | `Skill.Move.Climb` | Accumulate | 3 | (mission-system.md §3-1 그대로) |
| 9 | Dialogue | 울프에게 보고하십시오 | `Event.Mission.Report` | Accumulate | 1 | (미사용) |
| 10 | Gameplay | 그래플링 3회 | `Skill.Move.GrappleHook` | Accumulate | 3 | (mission-system.md §3-1 그대로) |
| 11 | Dialogue | 폭스에게 보고하십시오 | `Event.Mission.Report` | Accumulate | 1 | (미사용) |

`MissionReward` (6행, Dialogue 타입에만): `1→20, 3→10, 5→10, 7→10, 9→15, 11→15` — 합 80, mission-system.md §3-1 합과 동일.

`MissionCommunication` (12행): `MissionId 2k`(Gameplay) → `NPCId k, Role=Offer` / `MissionId 2k+1`(Dialogue) → `NPCId k, Role=Report` (k = 0..5).

### 4-9. 정합성 확인 — mission-system.md 와의 관계

- **바꾸지 않은 것**: 6종 `DisplayName`/`MatchTag`/`Mode`/`TargetCount`/`ExperienceReward` 값, 보상 총합(80), 최소 필요 킬 수(2), §2(배치 원칙 — 유한자원 미션을 앞에), §2-3(처치 목표 = 1).
- **바뀐 것**: 보상이 저장되는 위치(`Mission.ExperienceReward` → `MissionReward.ExperienceReward`)와 지급 시점(Gameplay 미션 완료 즉시 → 그 뒤 Dialogue/보고 미션 완료 시점), 배열 길이(6 → 12), 그리고 **각 Gameplay 미션 진입 시 NPC Offer 수락이 필요해졌다는 것**(이전엔 순수 자동 진행).
- **mission-system.md §4 의 프레임 단위 검산표는 참고용으로 격하한다** — "킬 2에서 레벨 이벤트와 미션 완료가 같은 프레임에 겹친다" 같은 정밀한 순서 주장은 그 문서가 가정한 "NPC 게이트 없는 순수 자동 진행" 모델 전제이고, 이 spec 이 그 전제를 깬다. 이 spec 은 그 표를 다시 그리지 않는다 — 대신 **총량(보상 합 80·최소 킬 2·레벨 임계값)이 안 바뀐다는 것**과 **레벨 신호 소실 문제가 §5-5 재평가로 여전히 해소된다는 것**(§2-6) 두 가지만 불변식으로 확인한다. mission-system.md §4 는 "NPC 게이트가 없다면"이라는 조건부 참고 자료로 남는다.

---

## 5. `USpyMissionComponent` 변경 — 2026-08-01b 되돌리기 + `MissionType` 기반 자동 수락

### 5-1. `FSpyMissionState` — `bObjectiveMet` 삭제, `bAccepted` 만 유지

```cpp
// SpyMissionComponent.h
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
    //# Dialogue 타입 미션은 인덱스 진입과 동시에 자동으로 true가 된다 (§5-2)
    UPROPERTY()
    bool bAccepted = false;
};
```

### 5-2. `ProcessProgress()` — 완료 시 즉시 전진 + 보상(2026-08-01 이전 동작으로 복귀) + `MissionType` 기반 자동 수락

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

        GrantReward(CompletedIndex);   //# Gameplay 타입은 MissionReward 행이 없어 조용히 no-op (§4-3)
        OnMissionCompleted.Broadcast(this, CompletedIndex);
    }

    if (bChanged == false)
        return;

    MissionState.MissionIndex = Result.MissionIndex;
    MissionState.Count = Result.Count;

    //# (신규) 새로 진입한 미션이 Dialogue 타입이면 자동 수락 — NPC Offer 절차가 없다
    const FSpyMissionEntry* NewEntry = GetMissionEntry(MissionState.MissionIndex);
    MissionState.bAccepted = (NewEntry != nullptr && NewEntry->MissionType == ESpyMissionType::Dialogue);

    OnMissionProgressChanged.Broadcast(this, MissionState.MissionIndex, MissionState.Count, GetTargetCount());

    if (IsAllCompleted())
        OnAllMissionsCompleted.Broadcast(this);
}
```

`AddProgress()`의 최상단 게이트는 2026-08-01 이전으로 되돌아간다 — `bAccepted == false` 한 줄만 확인한다(`bObjectiveMet` 체크 삭제). **다만 재진입 큐(`PendingEvents`) 드레인 경로도 이 게이트를 다시 거쳐야 한다 — §5-2-1.**

### 5-2-1. ⚠ `PendingEvents` 드레인이 `bAccepted` 게이트를 우회하는 경합(race) — 반드시 고친다

**문제(실측 코드로 확인, design-reviewer 지적)**: 기존 `AddProgress`(`SpyMissionComponent.cpp:92-145`)의 드레인 루프는 큐에 쌓인 이벤트를 `ProcessProgress()`로 **직접** 넘긴다 — `AddProgress` 최상단의 `bAccepted` 게이트를 다시 거치지 않는다. `GrantReward()`가 적용하는 경험치 GE는 `SpyLevelComponent::HandleExperienceChanged → TryLevelUp`을 동기 호출하고, `TryLevelUp`은 승급 시 `MissionComponent->AddProgress(Event_Mission_Level, ...)`를 **그 자리에서 다시** 호출한다(`SpyLevelComponent.cpp:168-178, 253-262`).

이 경로가 §5-2의 `ProcessProgress`와 맞물리면: Dialogue 미션(예: `MissionId 1`, 레이븐 보고) 완료 → `GrantReward(1)` 적용 → 그 XP가 마침 레벨업 임계를 넘기면 `TryLevelUp`이 **`ProcessProgress` 실행 도중**(아직 `MissionState.MissionIndex`가 1에서 2로 안 바뀐 시점) `AddProgress(Event_Mission_Level, ...)`를 재호출한다. 이때 `MissionState.bAccepted`는 **아직 mission1의 값(true)**이라 최상단 게이트를 통과하고, `bProcessingProgress == true`라 큐에 쌓인다. 잠시 후 `ProcessProgress`가 인덱스를 2(레벨 미션, Gameplay 타입)로 올리고 `bAccepted = false`로 되돌린 뒤 반환하면, **드레인 루프가 이 큐를 게이트 없이 그대로 `ProcessProgress`에 넘겨** 아직 수락하지 않은 레벨 미션을 완료시켜 버린다 — 플레이어가 팰컨의 Offer 카드를 한 번도 못 본 채 그 미션이 끝나 버릴 수 있다는 뜻이다. 이는 두 문서가 반복해서 명시한 불변식("Gameplay 미션은 NPC Offer로 수동 수락해야 진행된다")을 깨는 실제 경로다.

**해결**: 드레인 루프도 매 반복마다 **그 시점의** `MissionState.bAccepted`를 다시 확인한다 — 큐잉 시점이 아니라 처리 시점의 값을 봐야 한다(그 사이에 미션이 전진했을 수 있으므로).

```cpp
void USpyMissionComponent::AddProgress(FGameplayTag InEventTag, int32 InAmount)
{
    AActor* Owner = GetOwner();
    if (Owner == nullptr || Owner->HasAuthority() == false)
        return;

    if (MissionConfig == nullptr)
    {
        //# (기존 경고 로직 유지)
        return;
    }

    if (bProcessingProgress)
    {
        FSpyMissionPendingEvent Pending;
        Pending.EventTag = InEventTag;
        Pending.Amount = InAmount;
        PendingEvents.Add(Pending);

        return;
    }

    TGuardValue<bool> ReentryGuard(bProcessingProgress, true);

    //# 최초 호출도 게이트를 통과해야 처리된다 (기존과 동일)
    if (MissionState.bAccepted)
        ProcessProgress(InEventTag, InAmount);

    //# 드레인도 매 반복마다 "지금" 상태로 재검증한다 — 큐잉 당시엔 수락 상태였어도
    //# 그 사이 ProcessProgress가 미션을 전진시켜 bAccepted가 false로 바뀌었을 수 있다
    while (PendingEvents.Num() > 0)
    {
        const FSpyMissionPendingEvent Next = PendingEvents[0];
        PendingEvents.RemoveAt(0);

        if (MissionState.bAccepted)
            ProcessProgress(Next.EventTag, Next.Amount);
    }
}
```

**이 수정이 §4-3/§4-2(설계doc)의 D3 퇴화 케이스 서술을 바꾸지 않는다는 것을 확인한다**: 이 수정 덕분에 레이븐 보고 시점에 우연히 Lv3 임계를 넘겨도 그 신호는 (아직 미수락인) 레벨 미션에 적용되지 못하고 조용히 버려진다 — 이후 플레이어가 실제로 팰컨에게 가서 [수락]을 누르는 순간에야 §5-5 재평가가 ASC의 "지금 레벨"을 직접 읽어 정확히 완료시킨다. 즉 **이 수정은 설계doc이 이미 서술한 "수락 시점에 재평가가 발동한다"는 타이밍을 실제로 보장하는 코드다** — 수정 없이는 그보다 먼저(보고 시점에 조용히) 완료돼 버려 두 문서의 서술과 실제 동작이 어긋난다.

### 5-3. `ReportCurrentMission()` — 삭제

**서버 RPC 로 별도 호출하지 않는다.** "보고"의 완료는 §7 서버 흐름에서 `ASpyNPCCharacter::RequestInteract` 가 **같은 호출 안에서** `AddProgress(Event_Mission_Report, 1)` 를 호출하는 것으로 끝난다 — Report 미션은 확인 카드가 없으므로 별도 확정 RPC가 필요 없다.

### 5-4. `AcceptCurrentMission()` — 시그니처 동일, 여전히 Gameplay 타입 Offer 카드의 "수락" 버튼에서만 호출

```cpp
bool USpyMissionComponent::AcceptCurrentMission()
{
    AActor* Owner = GetOwner();
    if (Owner == nullptr || Owner->HasAuthority() == false)
        return false;

    if (IsAllCompleted())
        return false;

    if (MissionState.bAccepted)
        return true;   //# 멱등

    MissionState.bAccepted = true;

    //# §5-5 — 레벨 재평가 (변경 없음, 아래 그대로 유지)
    const FSpyMissionEntry* CurrentEntry = GetMissionEntry(MissionState.MissionIndex);
    if (CurrentEntry != nullptr && CurrentEntry->MatchTag == SpyGameplayTags::Event_Mission_Level && AbilitySystemComponent != nullptr)
    {
        const float CurrentLevel = AbilitySystemComponent->GetNumericAttribute(USpyCharacterAttributeSet::GetLevelAttribute());
        AddProgress(SpyGameplayTags::Event_Mission_Level, FMath::RoundToInt(CurrentLevel));
    }

    return true;
}
```

Dialogue 타입 미션은 이 함수를 호출하지 않는다 — §5-2 에서 인덱스 진입과 동시에 자동 수락되기 때문에 UI 에 "수락" 버튼 자체가 없다(§8).

### 5-5. 레벨 재평가 — 그대로 유지, 적용 범위만 재확인

이전 판(spec §5-6)의 메커니즘·코드를 **그대로** 쓴다. 이번 개정에서 달라지는 것은 "그 앞에 몇 개의 게이트가 끼는가"뿐이다:

- 이전(2026-08-01b): 처치 미션 완료 → **그 즉시** 레벨 미션이 현재 미션 (게이트 없음, "보고 대기"만 있음)
- 이번(2026-08-01c): 처치 미션 완료 → 레이븐에게 **보고**(Dialogue, 자동 수락, NPC 상호작용 대기) → 레벨 미션이 현재 미션이 되지만 **여전히 미수락**(Gameplay 타입) → 팰컨에게 **Offer 수락**까지 필요

**재평가가 스냅샷 기반(그 순간의 실제 레벨을 직접 읽음)이라 이 차이는 정확성에 영향을 주지 않는다** — 그 사이에 게이트가 1개(2026-08-01b)든 2개(이번 판: 보고 + 수락)든, `AcceptCurrentMission()`이 호출되는 시점에 ASC 의 현재 레벨을 그대로 재주입하므로 결과는 동일하다. 다만 **노출 구간(신호가 소실될 수 있는 시간)은 이번 판이 더 길다** — 처치 후 레이븐에게 걸어가는 시간 + 팰컨에게 걸어가는 시간 전부가 노출 구간이다. 재평가가 이 구간의 길이와 무관하게 항상 정확하므로 문제가 되지 않는다(§2-6).

**전제 확인**: `USpyCharacterAttributeSet`이 `USpyMissionComponent`와 같은 ASC(PlayerState의 ASC)에 붙어 있다는 전제는 이전과 동일 — gameplay-programmer가 구현 착수 전 재확인한다.

### 5-6. 공개 접근자 — 변경 없음

```cpp
UFUNCTION(BlueprintPure) bool IsCurrentAccepted() const { return MissionState.bAccepted; }

//# 인덱스로 임의 미션 엔트리를 조회한다. NPC가 Offer 카드용 Description을 채울 때, MissionComponent가
//# 새로 진입한 미션의 MissionType을 확인할 때 쓴다.
const FSpyMissionEntry* GetMissionEntry(int32 InIndex) const;
```

`IsCurrentObjectiveMet()` 은 삭제한다 — 대응하는 상태(`bObjectiveMet`)가 없다.

---

## 6. 상호작용 → 대사 상태 판정 (순수 함수)

2026-08-01b 의 5상태(`Locked`/`Offer`/`InProgress`/`ReadyToReport`/`Completed`)를 **4상태**로 단순화한다 — `Locked`와 `Completed`를 하나(`Default`)로 합친다. NPC 는 "아직 내 차례가 아님"과 "이미 내 차례가 끝남"을 구분해서 보여줄 필요가 없다(§4-4 `NPC.DefaultDialogueId` 가 그 통합된 개념이다). 이 통합 덕분에 NPC 당 대사가 5줄에서 4줄로 준다(§4-8).

```cpp
// Util/DefineEnum.h 에 추가
UENUM()
enum class ESpyNPCDialogueState : uint8
{
    Default,     //# 현재 미션이 이 NPC와 무관함 (아직 차례 아님 / 이미 끝남 — 구분하지 않는다)
    Offer,       //# 이 NPC가 담당하는 Gameplay 미션이 현재 미션, 미수락 — 수락 카드 표시
    InProgress,  //# 이 NPC가 담당하는 Gameplay 미션이 현재 미션, 수락됨 — 독려 멘트만
    Report,      //# 이 NPC가 담당하는 Dialogue(보고) 미션이 현재 미션 — 상호작용 즉시 완료
};

//# 부수효과 없음 — Automation 테스트 대상
ESpyNPCDialogueState ResolveNPCDialogueState(
    int32 CurrentMissionId, bool bAccepted, int32 OfferMissionId, int32 ReportMissionId);
```

판정:
- `CurrentMissionId == OfferMissionId` → `bAccepted == false` 면 `Offer`, 아니면 `InProgress`
- `CurrentMissionId == ReportMissionId` → `Report`
- 그 외 → `Default`

`FSpyNPCDialogueResult`(반환 구조체, `NPC/CommonInterface.NPC.h`):

```cpp
USTRUCT()
struct FSpyNPCDialogueResult
{
    GENERATED_BODY()

public:
    UPROPERTY() ESpyNPCDialogueState State = ESpyNPCDialogueState::Default;
    UPROPERTY() FText NPCName;
    UPROPERTY() FText Line;

    //# Offer 상태일 때만 true — 미션 수락 카드를 띄운다. Report 는 카드가 없다(§7)
    UPROPERTY() bool bShowMissionCard = false;

    //# Offer 상태일 때만 채운다. 보상 텍스트는 없다 — Gameplay 타입은 보상이 없다(§4-3)
    UPROPERTY() FText MissionTitle;
    UPROPERTY() FText MissionDescription;
};
```

---

## 7. 서버 흐름

```
[클라] F 입력 → USpyInteractionComponent::TryInteract()
   → Server RPC: Server_RequestInteract(TargetNPC)

[서버] ASpyNPCCharacter::RequestInteract(RequesterController)
   1. 거리 재검증 (SphereComponent 반경 내인지 서버가 직접 확인 — 클라 신고 신뢰 안 함)
   2. RequesterController → Pawn → PlayerState → USpyMissionComponent::FindMissionComponent
   3. State = ResolveNPCDialogueState(MC->GetMissionIndex(), MC->IsCurrentAccepted(),
                                       CachedOfferMissionId, CachedReportMissionId)
   4. State == Offer  → Line = CachedOfferLine,       bShowMissionCard = true,
                         MissionTitle/Description = MC->GetMissionEntry(CachedOfferMissionId) 조회(§5-6)
      State == InProgress → Line = CachedInProgressLine, 카드 없음
      State == Report  → Line = CachedReportLine, 카드 없음,
                          **이 분기에서만** MC->AddProgress(Event_Mission_Report, 1) 을 함께 호출한다
                          (완료 여부는 AddProgress 내부의 MatchTag/TargetCount 판정에 그대로 맡긴다 — §2-3)
      State == Default → Line = CachedDefaultLine, 카드 없음
   5. 결과(FSpyNPCDialogueResult)를 요청한 클라이언트에게만 Client RPC로 전달

[클라] 대사 상태 수신 → USpyUIManager::OpenSpyUI(ESpyUIType::Dialogue)
   → "계속" 입력 시 대화창 닫힘
      → Offer 상태(bShowMissionCard) → ESpyUIType::MissionOffer 카드
           → [수락] → Server RPC: Server_AcceptCurrentMission() → USpyMissionComponent::AcceptCurrentMission()
           → [거절] → 카드만 닫음, RPC 없음
      → InProgress/Report/Default → 카드 없이 대화창만 닫힘 (Report 는 이미 3단계에서 완료 처리가 끝난 상태)
```

**"보고"에 별도 확인 버튼이 없는 이유**: Dialogue 타입 미션은 "받을지 말지"를 고를 대상이 아니다 — 목표를 이미 달성한 뒤 그 NPC에게 말을 거는 행위 자체가 보고다. 그래서 3단계(`RequestInteract`)에서 대사 판정과 완료 처리를 같은 서버 호출 안에서 함께 끝낸다. 클라이언트는 결과로 받은 `ReportLine` 을 보여주기만 하면 된다.

`USpyMissionComponent`에 다음 `BlueprintPure` 접근자가 필요하다(§6 판정 호출용, §5-6과 동일):
- `IsCurrentAccepted()`
- `GetMissionIndex()` (기존)

---

## 8. UI

### 8-1. 신규 위젯

- `UI/SpyDialogueWidget.h|.cpp` — `USpyUserWidget` 상속. `BindWidgetOptional`로 이름/대사 텍스트만 캡슐화, 외부는 `ShowLine(FText Name, FText Line)` 의도 API만 호출.
- `UI/SpyMissionOfferWidget.h|.cpp` — **Offer 카드 전용, 겸용 아님** (2026-08-01b 의 "수락/보고 겸용" 설계 폐기 — Report 는 카드가 없으므로 겸용할 대상이 없다). `ShowMission(FText Title, FText Description)` + 두 버튼(`OnAcceptClicked`/`OnDeclineClicked`)만 노출. **`RewardText` 인자 없음** — Gameplay 타입 미션은 보상이 없다(§4-3).
- `Util/DefineEnum.h`의 `ESpyUIType`에 `Dialogue`, `MissionOffer` 두 항목 추가.

### 8-2. 목업 (2단계 전환, 카드는 Offer 1종뿐)

```
[1] 근접 상호작용 프롬프트
┌──────────────────────────────────────────────┐
│                  [ NPC 3D 모델 ]                │
│                 ╭─────────────╮                │
│                 │  F  대화하기  │                │
│                 ╰─────────────╯                │
└──────────────────────────────────────────────┘

[2-A] 대화창 — Offer 상태
┌────────────────────────────────────────────────────┐
│ ┌────┐  정보원 "레이븐"                                │
│ │NPC │  "새 임무가 들어왔다. 확인해봐."                     │
│ └────┘                              [Space ▶ 계속]     │
└──────────────────────────────────────────────────────┘
   → 계속 시 대화창이 닫히고 [3] 수락 카드로 전환

[2-B] 대화창 — InProgress 상태 (선택지 없음)
┌──────────────────────────────────────────────────────┐
│ ┌────┐  정보원 "레이븐"                                │
│ │NPC │  "아직 진행 중이군. 계속해."                        │
│ └────┘                              [Space ▶ 닫기]     │
└──────────────────────────────────────────────────────┘

[2-C] 대화창 — Report 상태 (서버는 이 시점에 이미 완료 처리를 끝냈다)
┌────────────────────────────────────────────────────┐
│ ┌────┐  정보원 "레이븐"                                │
│ │NPC │  "정리는 끝났나. 수고했다."                          │
│ └────┘                              [Space ▶ 닫기]     │
└──────────────────────────────────────────────────────┘

[2-D] 대화창 — Default 상태 (선택지 없음)
┌──────────────────────────────────────────────────────┐
│ ┌────┐  정보원 "레이븐"                                │
│ │NPC │  (DefaultLine)                                │
│ └────┘                              [Space ▶ 닫기]     │
└──────────────────────────────────────────────────────┘

[3] 미션 수락 카드 (Offer 상태 → [2-A]에서 전환) — 보상 텍스트 없음
┌──────────────────────────────────────┐
│                새 임무                 │
│   적 처치                              │
│   목표 : 감시 중인 적 요원 1명을 제거하라    │
│      [ 수락 ]        [ 거절 ]           │
└──────────────────────────────────────┘
```

### 8-3. 위젯 배치는 사용자가 디자이너에서 직접 수행

MCP는 위젯 트리 조립까지만 담당하고 `compile_blueprint()`는 호출하지 않는다 (ui-workflow.md).

---

## 9. 엣지 케이스

- **동시 다중 플레이어 상호작용**: NPC는 어떤 플레이어별 상태도 들고 있지 않다 — `RequestInteract`는 매번 요청자의 `MissionComponent`만 참조.
- **보고 위조 방지**: `Server_RequestInteract`는 서버가 거리·현재 미션 인덱스를 재검증한 뒤에만 `AddProgress(Event_Mission_Report, 1)`을 호출한다. 목표 달성 여부와 무관하게 Report 상태에 도달했다는 것 자체가 이미 "목표 달성 → 인덱스가 Dialogue 미션으로 넘어왔다"는 뜻이므로(§2-3, `ResolveMissionProgress`가 이미 완료 판정을 마쳤다), 위조 여지가 없다.
- **로컬 컨트롤 폰만 프롬프트 갱신**: NPC의 오버랩 콜백이 `IsLocallyControlled()` 필터를 거친 뒤에만 `ISpyInteractionHost`를 호출.
- **`NPCId`/`MissionCommunication` 불일치(기획 실수)**: 한 `NPCId`에 대해 `MissionCommunication`에 `Role == Offer` 행이 없거나 2개 이상이면 `CachedOfferMissionId`가 정의되지 않는다 — `BeginPlay` 캐싱 시 정확히 1개씩(Offer 1 · Report 1) 나와야 한다고 검증(assert/로그)하고, 어긋나면 에디터 데이터 오류로 로그를 남긴다.
- **거절 반복**: 페널티·제한 없음(§1 비목표).
- **공용 `Event_Mission_Report` 태그의 안전성**: 모든 Dialogue 미션이 같은 태그를 쓰지만, 서버는 "현재 플레이어의 `MissionIndex`가 정확히 이 NPC의 `ReportMissionId`와 같을 때만" `AddProgress`를 호출한다 — 그 시점에 플레이어의 현재 미션은 전역적으로 단 하나뿐이므로, 다른 NPC에게 말을 걸어도 그 NPC의 `RequestInteract`는 `State == Default`로 판정돼 `AddProgress`를 호출하지 않는다. 태그 자체가 아니라 **이 인덱스 일치 확인이 안전성의 근거**다.
- **마지막 미션(11) 완료 시점**: `OnAllMissionsCompleted`는 `MissionIndex`가 배열 범위를 벗어날 때(2026-08-01 이전과 동일한 `IsAllCompleted()` 정의) 발화한다 — 폭스에게 보고를 마친 순간이다.
- **레벨 미션 재평가의 한계**: §5-5 그대로 — 수락 시점의 스냅샷 1회. 재평가 자체가 실패하는 경우(ASC 세팅 순서 문제)는 없는지 인게임 확인 대상.
- **보상 GE가 유발한 재진입 레벨업이 아직 미수락인 다음 미션을 조용히 완료시키는 경합**: §5-2-1에서 해소 — `PendingEvents` 드레인 루프가 매 반복 `bAccepted`를 재검증하지 않으면, Dialogue 미션 보고 시 지급된 보상이 우연히 레벨업을 유발할 때 그 신호가 아직 수락 전인 다음 Gameplay 미션(레벨 미션)에 게이트 없이 적용돼 버린다. **이 수정 없이 구현하면 안 된다.**

---

## 10. 변경 파일 목록

**신규**
- `NPC/SpyNPCCharacter.h|.cpp`
- `NPC/CommonInterface.NPC.h`
- `Data/SpyNPCDialogueRow.h|.cpp` — `FSpyNPCRow`/`FSpyDialogueRow`/`ESpyMissionCommRole`/`FSpyMissionCommunicationRow`/`USpyNPCConfig`, `ResolveDialogueText` 자유 함수
- `ManagerComponent/SpyInteractionComponent.h|.cpp`
- `UI/SpyDialogueWidget.h|.cpp`
- `UI/SpyMissionOfferWidget.h|.cpp`
- `System/Tests/SpyNPCDialogueTests.cpp`

**수정**
- `Data/SpyMissionConfig.h|.cpp` — `FSpyMissionEntry.ExperienceReward` 삭제, `MissionType` 필드 추가, `FSpyMissionRewardRow`/`MissionRewardTable`/`GetMissionReward()` 추가
- `System/SpyMissionComponent.h|.cpp` — `bObjectiveMet`/`ReportCurrentMission()`/`IsCurrentObjectiveMet()` 삭제, `ProcessProgress`를 2026-08-01 이전 동작(즉시 전진+보상)으로 복귀 + `MissionType` 기반 자동 수락 추가
- `Character/SpyCharacter.h|.cpp` / `Character/CommonInterface.Character.h` — 상호작용 컴포넌트 접근자
- `ManagerComponent/CommonInterface.Manager.h` — `ISpyInteractionHost` 추가
- `Util/DefineEnum.h` — `ESpyMissionType`, `ESpyNPCDialogueState`, `ESpyUIType::Dialogue`, `ESpyUIType::MissionOffer`
- `Util/SpyGameplayTags.h|.cpp` — `Event_Mission_Report` 신규 태그
- `Input/` 관련 파일 — `IA_Interact` 액션 추가

**폐기 (2026-08-01b 산출물 — plan Task 재작성 시 되돌린다)**
- 이전 `Data/SpyNPCDialogueRow.h|.cpp`(5줄 단일 NPC 로우 구조), `System/Tests/SpyNPCDialogueTests.cpp`(4-state 판정 테스트)는 **파일명은 재사용하되 내용을 전면 교체**한다 — 5테이블 스키마가 이를 대체한다.

**에셋 (사용자 / MCP, 데이터 구조 확인 게이트 + UI 목업 승인 이후 착수)**
- `DA_SpyNPCConfig` DataAsset 생성 + 3개 DataTable(`DT_SpyNPC` 6행, `DT_SpyDialogue` 24행, `DT_SpyMissionCommunication` 12행) 생성·연결
- `DA_SpyMissionConfig`의 `Missions` 12행 재입력 + `DT_SpyMissionReward` 6행 생성·연결
- NPC 블루프린트 6종 — `NPCId`, `NPCConfig` 지정
- `WBP_Dialogue`, `WBP_MissionOffer` 위젯 배치 (§8-2 목업 기준)

---

## 11. 테스트 (Unreal Automation)

`System/Tests/SpyNPCDialogueTests.cpp`, 기존 `SpyMissionTests.cpp`와 동일한 등록 규칙(`"SkillProject.System.NPCDialogue.<케이스>"`).

| 케이스 | 기대 |
|---|---|
| `ResolveNPCDialogueState` — Current == OfferMissionId, 미수락 | `Offer` |
| Current == OfferMissionId, 수락됨 | `InProgress` |
| Current == ReportMissionId | `Report` |
| Current 가 둘 다 아님 | `Default` |
| `ResolveDialogueText` — `DialogueIndex` 0 하나만 있는 그룹 | 그 한 줄 그대로 |
| `ResolveDialogueText` — 여러 `DialogueIndex` 가 있는 그룹 | 오름차순으로 이어붙인 결과 |
| `ResolveDialogueText` — 없는 `DialogueId` | 빈 `FText` |
| `USpyMissionConfig::GetMissionReward` — `MissionReward` 행이 있는 `MissionId` | 해당 `ExperienceReward` |
| `GetMissionReward` — 행이 없는 `MissionId`(Gameplay 타입) | `0.f` |
| `ResolveMissionProgress`(기존, 미변경) — Dialogue 타입 미션에 `Event_Mission_Report` 1회 | `bCompletedNow == true`, 인덱스 +1 (§2-3, 순수 함수 재검증) |

**컴포넌트 레벨은 Automation 커버 불가** (기존과 동일한 이유 — `MissionConfig`가 `protected`, `HasAuthority()`가 액터 컨텍스트 필요). PIE 확인 대상:
- `AddProgress` — 미수락 상태에서 이벤트 발생 시 진행도 변화 없음
- `ProcessProgress` — Gameplay 미션 완료 시 보상 없이 즉시 전진 + 다음(Dialogue) 미션 자동 수락(`bAccepted == true`) 확인
- `AcceptCurrentMission` — 정상/멱등/전체완료 케이스, **레벨 미션 재평가(§5-5)가 실제로 즉시 판정되는지**(게이트 2개를 거친 뒤에도)
- `RequestInteract` — Report 상태 도달 시 대화만으로(추가 버튼 없이) 보상 지급 + 인덱스 전진 + 다음 NPC의 Offer 로 넘어가는지
- 거리 재검증, 서버 RPC 왕복, 클라이언트 UI 표시 동기화

**인게임 확인**: 1인 PIE — 레이븐에게 말 걸어 수락 → 봇 처치 → **레이븐에게 재대화 시 즉시 보상 지급 + 팰컨 Offer 로 HUD 문구가 바뀌는지** → 팰컨 수락(레벨 3 이미 넘긴 상태로 시도해 §5-5 재평가 확인) → ... 6개 NPC 전체 순회. 2인 PIE — 한 플레이어의 수락/보고가 다른 플레이어 상태에 영향 없는지.
