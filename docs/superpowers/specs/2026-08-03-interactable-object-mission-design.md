# 오브젝트 상호작용 미션 설계

- **작성일**: 2026-08-03
- **범위**: 레벨에 배치된 단일 액터(레버·스위치·단서 등)를 F키로 상호작용해 완료되는 새 미션 타입. NPC 대화(`ESpyMissionType::Dialogue`)와 별개 경로 — 대화 UI를 거치지 않고 상호작용 즉시 진행도만 갱신된다.
- **승인 상태**: 브레인스토밍 완료, 사용자 승인 대기
- **선행 시스템**: 미션 시스템(`docs/design/mission-system.md`), NPC 대화 기반 미션 수락(`docs/superpowers/specs/2026-08-01-npc-mission-dialogue-design.md`). 이 spec은 두 시스템의 값·로직을 바꾸지 않고 세 번째 미션 타입과 세 번째 상호작용 경로를 나란히 추가한다.

---

## 1. 목표

플레이어가 레벨에 배치된 상호작용 오브젝트(레버·단서·터미널 등, 대사 없음)에 근접해 F키를 누르면 그 자리에서 미션 진행도가 +1 되고, 오브젝트는 즉시 소진(1회성)된다. NPC 대화 없이도 "가서 무언가를 조작하라" 유형의 미션을 만들 수 있게 한다.

**핵심 결정**: 이 오브젝트는 NPC가 아니다 — `ISpyNPCRoot`/`FSpyNPCDialogueResult`/대화 상태 머신을 재사용하지 않는다. NPC 코드는 한 줄도 건드리지 않고, 완전히 별도인 `Interactable/` 도메인을 신설해 병렬 경로로 붙인다. 이유:
- `FSpyNPCDialogueResult`는 `NPCName`/`bShowMissionCard`/`MissionTitle` 등 대화 전용 필드로 이루어져 있어 "말없이 조용히 완료"라는 요구와 형태가 맞지 않는다.
- NPC 파이프라인을 그대로 재사용하면 회귀 위험이 생긴다 — 별도 경로는 NPC 동작에 영향 0.

**비목표 (이번 범위 밖)**
- DevMap 실제 배치 — 사용자가 MCP/에디터에서 직접 수행
- 여러 오브젝트/NPC 순차 상호작용, 아이템 인벤토리 연동
- 오브젝트별 고유 연출(VFX/사운드) — `BlueprintImplementableEvent` 훅 하나만 열어 두고 실제 연출은 범위 밖
- 재사용 가능한(반복 상호작용) 오브젝트 — 이번 타입은 전부 1회성

---

## 2. 조사로 확정된 사실

### 2-1. "NPC와 상호작용해야 완료되는 미션"은 이미 일부 존재한다

`ESpyMissionType::Dialogue`가 NPC의 Report 대사 트리거 시 `AddProgress(Event_Mission_Report, 1)`를 호출해 정확히 이 개념을 구현하고 있다(`SpyNPCCharacter.cpp:191-197`). 이번 기능은 이걸 확장하는 게 아니라 **"NPC가 아닌 오브젝트"**라는 별도 트리거 소스를 추가하는 것이다.

### 2-2. 상호작용 트리거는 현재 NPC 전용으로 하드코딩돼 있다

`SpyInteractionComponent.cpp:27-28`에 이미 이 사실을 명시한 주석이 있다:
> `//# 트리거는 현재 NPC 전용이라 동사를 고정한다 — 트리거 구조 일반화는 이번 범위 밖`

`CachedInteractVerb`가 NPC 오버랩 시 `"대화하기"`로 하드 세팅되고, `NearbyNPC`(단일 슬롯)·`Server_RequestInteract(AActor*)`(내부에서 `Cast<ISpyNPCRoot>`)가 전부 NPC를 전제한다. 이번 기능이 이 주석이 가리키던 일반화 작업이다.

### 2-3. `AddProgress`/`ResolveMissionProgress`는 태그·인덱스만 보고 신호원을 모른다

`SpyMissionComponent::AddProgress(FGameplayTag, int32)`는 이벤트 발생 지점이 GA든 NPC든 오브젝트든 신경 쓰지 않는다(`SpyLevelComponent.cpp`의 Kill/Level, `SpyGameplayAbility_SkillAction.cpp`의 Combo가 이미 같은 방식). 오브젝트도 이 진입점을 그대로 호출하면 되고, 미션 판정 로직(`ResolveMissionProgress`, `Data/SpyMissionConfig.cpp:26-73`)은 한 줄도 바꿀 필요가 없다.

### 2-4. 자동 수락은 `MissionType`으로만 갈린다

`ProcessProgress`(`SpyMissionComponent.cpp:169`)는 새로 진입한 미션이 `Dialogue`면 자동 수락, 아니면(`Gameplay`) 수동 수락(NPC Offer 카드)을 요구한다. 오브젝트 미션은 카드를 보여줄 NPC가 없으므로 `Dialogue`와 같은 처지다 — `MissionType`에 `Interact`를 추가하고 자동 수락 조건에 포함시킨다.

---

## 3. 배치 — 신규 `Interactable/` 도메인

```
Interactable/
├── SpyInteractableObject.h|.cpp   # 도메인 루트 (cpp-style §13)
└── CommonInterface.Interactable.h # ISpyInteractableRoot (cpp-style §12)
```

### 3-1. `ASpyInteractableObject`

- `AActor` 상속(캐릭터가 아니라 정적 배치 오브젝트라 `AModularCharacter` 대상 아님. `ModularGameplayActors`의 `AModularCharacter`/`AModularPawn`은 폰류 전용 — 이 액터는 GameFeature 컴포넌트 확장 대상이 아니다).
- 하위 구성 요소는 상호작용 감지용 `USphereComponent` 하나뿐 — §13 "하위 1개면 루트 파사드 생략 가능" 예외 대상이지만, 소비자(`USpyInteractionComponent`)를 위해 인터페이스는 그대로 뺀다(§13 "소비자 없어도 미리 뺀다").
- `EditAnywhere` 프로퍼티(레벨 배치 시 인스턴스별로 값을 다르게 줄 수 있어야 하므로 `EditDefaultsOnly`가 아니라 `EditAnywhere`):
  - `FGameplayTag MissionEventTag` — 이 오브젝트가 상호작용 시 발신할 태그. `Event.Mission.Interact` 계열(§4-1)을 계층 매칭으로 세분화 가능(Kill 미션과 동일 원리).
  - `FText InteractVerb` — 근접 프롬프트에 표시할 동사. 기본값 `"조사하기"`.
- `Replicated(OnRep = OnRep_Consumed) bool bConsumed` — 서버가 상호작용 처리 후 `true`로 세팅. `OnRep_Consumed`에서 `InteractionSphere->SetCollisionEnabled(NoCollision)`을 걸어 재상호작용을 막는다. 서버 자신은 `OnRep`이 발화하지 않으므로 `RequestInteract` 안에서 같은 정리 로직을 직접 호출한다(공유 private 헬퍼로 묶는다).
- `BlueprintImplementableEvent void OnConsumed()` — 연출 훅(VFX/사운드는 이번 범위 밖, 자리만 마련).

```cpp
// Interactable/CommonInterface.Interactable.h
UINTERFACE(MinimalAPI)
class USpyInteractableRoot : public UInterface { GENERATED_BODY() };

class ISpyInteractableRoot
{
    GENERATED_BODY()

public:
    //# 서버 권한에서만 유효. 상호작용 처리 + AddProgress + 소진 처리까지 이 안에서 끝낸다.
    virtual void RequestInteract(APlayerController* Requester) = 0;

    //# 서버 재검증 전용 — 트리거(오버랩)와 동일한 기하로 재확인한다 (NPC 패턴과 동일 이유)
    virtual bool IsPawnInRange(const AActor* RequesterPawn) const = 0;

    virtual FText GetInteractVerb() const = 0;
};
```

`RequestInteract` 내부 흐름:
1. `HasAuthority()` 가드
2. `bConsumed` 가드(이미 소진 — 조용히 반환, 중복 진행 방지)
3. `Requester->GetPawn() → GetPlayerState() → USpyMissionComponent::FindMissionComponent(...)` (NPC의 `RequestInteract`와 동일 패턴)
4. `MissionComp->AddProgress(MissionEventTag, 1)`
5. `bConsumed = true` + 정리 헬퍼 직접 호출(위 §3-1)

### 3-2. NPC와 동일한 오버랩 → 알림 패턴

`BeginPlay`에서 `InteractionSphere`의 `OnComponentBeginOverlap`/`OnComponentEndOverlap`을 바인딩하고, `ASpyNPCCharacter::OnInteractionSphereBeginOverlap`(`SpyNPCCharacter.cpp:94-112`)과 완전히 동일한 필터(로컬 컨트롤 폰만, `Cast<ISpyCharacterRoot>`로 인터페이스 구현 여부 판정)를 거쳐 `ISpyInteractionHost::NotifyInteractableRangeChanged(this, bool)`를 호출한다. 코드는 다르지만(별도 액터·별도 함수) 구조는 그대로 베낀다 — 공유 헬퍼로 묶기엔 두 액터가 서로 다른 인터페이스(`ISpyNPCRoot` vs `ISpyInteractableRoot`)를 캐스팅해야 해서 추상화 이득이 없다(YAGNI).

---

## 4. 데이터 — `ESpyMissionType` 확장

### 4-1. 신규 태그

```cpp
// Util/SpyGameplayTags.h / .cpp — 기존 Event_Mission_Kill/Combo/Level/Report 옆에 추가
SKILLPROJECT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Mission_Interact);
// .cpp: UE_DEFINE_GAMEPLAY_TAG(Event_Mission_Interact, "Event.Mission.Interact");
```

`FSpyMissionEntry::MatchTag`가 이 태그(또는 계층 하위 세부 태그)를 갖는 미션 엔트리가 "오브젝트 상호작용 미션"이 된다. 오브젝트의 `MissionEventTag`는 에디터에서 이 태그로 지정한다.

### 4-2. `ESpyMissionType`에 `Interact` 추가

```cpp
// Util/DefineEnum.h
//# 미션 1개의 수락 방식을 가른다. Gameplay는 NPC Offer 카드로 수동 수락,
//# Dialogue/Interact는 배열 진입과 동시에 자동 수락된다 (카드를 보여줄 주체가 없다)
UENUM(BlueprintType)
enum class ESpyMissionType : uint8
{
    Gameplay,
    Dialogue,
    Interact,
};
```

### 4-3. `USpyMissionComponent` 변경 (2곳, 조건 확장만)

- `ProcessProgress`(`cpp:169`) 자동 수락 조건:
  ```cpp
  MissionState.bAccepted = (NewEntry != nullptr &&
      (NewEntry->MissionType == ESpyMissionType::Dialogue || NewEntry->MissionType == ESpyMissionType::Interact));
  ```
- `GrantReward`(`cpp:240`) 보상 누락 경고 조건 — `Interact`도 자동 수락·완료형이라 보상이 있는 게 정상인 정책을 Dialogue와 공유한다:
  ```cpp
  if (Entry->MissionType == ESpyMissionType::Dialogue || Entry->MissionType == ESpyMissionType::Interact)
  {
      UE_LOG(..., TEXT("MissionReward 행이 없습니다..."), ...);
  }
  ```
- `FSpyMissionRewardRow`/`MissionRewardTable`는 기존 그대로 재사용 — `Interact` 타입 미션도 `Dialogue`와 동일하게 이 관계 테이블에 보상 행을 둔다. 스키마 변경 없음.
- `ResolveMissionProgress`(`Data/SpyMissionConfig.cpp`)는 **변경 없음** — §2-3.

---

## 5. 플레이어 측 — `SpyInteractionComponent` 확장

### 5-1. `ISpyInteractionHost`에 메서드 추가 (`ManagerComponent/CommonInterface.Manager.h`)

```cpp
virtual void NotifyInteractableRangeChanged(AActor* InteractableActor, bool bInRange) = 0;
```

### 5-2. `USpyInteractionComponent` 신규 필드/함수

- `UPROPERTY(Transient) TObjectPtr<AActor> NearbyInteractable;` — `NearbyNPC`와 별개 슬롯(플레이어가 NPC와 오브젝트 사이에 동시에 있는 드문 경우도 각자 독립적으로 추적).
- `NotifyInteractableRangeChanged(AActor*, bool)` — `NotifyNPCRangeChanged`(`cpp:21-65`)와 같은 구조. 차이점:
  - `CachedInteractVerb`를 하드코딩 `"대화하기"` 대신 `Cast<ISpyInteractableRoot>(InteractableActor)->GetInteractVerb()`로 채운다 — §2-2의 주석이 가리키던 일반화를 이 지점에서 실현.
  - 프롬프트 open/close는 동일하게 `ESpyUIType::InteractPrompt` 재사용(별도 위젯 불필요).
- `TryInteract()` 확장:
  ```cpp
  void USpyInteractionComponent::TryInteract()
  {
      if (bDialogueOpen)
      {
          AdvanceOrCloseDialogue();
          return;
      }

      if (NearbyNPC != nullptr)
      {
          Server_RequestInteract(NearbyNPC);
          return;
      }

      if (NearbyInteractable != nullptr)
          Server_RequestInteractObject(NearbyInteractable);
  }
  ```
  NPC가 우선순위를 갖는다(동시에 범위 안이면 NPC 먼저) — 이번 범위에서 실제로 겹칠 일은 없지만 기본값으로 명시해 둔다.
- 신규 RPC:
  ```cpp
  UFUNCTION(Server, Reliable)
  void Server_RequestInteractObject(AActor* TargetObject);
  ```
  구현은 `Server_RequestInteract`(`cpp:143-171`)와 같은 뼈대(Owner 권한 확인 → `Cast<ISpyInteractableRoot>` → `IsPawnInRange` 재검증 → `RequestInteract(PC)` 호출)이되, **Client RPC로 결과를 돌려보내지 않는다** — void 반환, 대화 UI 없음. 미션 HUD 갱신은 `USpyMissionComponent::OnMissionProgressChanged`(레플리케이션 기반 기존 델리게이트)가 자동으로 처리한다.

`Server_RequestInteract`와 통합하지 않는 이유(대안 기각): 한 함수에서 `Cast<ISpyNPCRoot>` 실패 시 `Cast<ISpyInteractableRoot>`로 폴백하는 방식도 가능하지만, 두 경로가 반환 타입(`FSpyNPCDialogueResult` 유무)과 후속 Client RPC 유무가 달라 분기가 늘어난다. 별도 함수 2개가 각자 단일 책임을 유지해 더 명확하다.

---

## 6. 엣지 케이스

- **재상호작용 방지**: `bConsumed` 가드가 `RequestInteract` 최상단에 있어 클라이언트가 프롬프트 닫히기 전에 F를 연타해도 `AddProgress`가 2번 호출되지 않는다.
- **오브젝트가 현재 미션과 무관할 때**: `MissionEventTag`가 현재 활성 미션의 `MatchTag`와 매칭되지 않으면 `AddProgress`가 조용히 no-op(§2-3, 기존 `ResolveMissionProgress` 동작 그대로) — 별도 방어 코드 불필요.
- **미수락 상태에서 상호작용**: `Interact` 타입은 배열 진입과 동시에 자동 수락되므로(§4-3) 이 케이스는 발생하지 않는다. 다만 오브젝트에 걸린 태그가 아직 도달하지 않은(다른 미션이 현재인) 인덱스를 가리키면 마찬가지로 no-op.
- **레플리케이션 타이밍**: `bConsumed`는 `COND_None`(전체 복제) — NPC의 `MissionState`와 달리 이 값은 모든 클라이언트가 봐야 한다(같은 방을 도는 다른 플레이어도 소진된 오브젝트와 상호작용하면 안 됨 — 단, 미션 진행도 자체는 플레이어별이라 실제 완료 여부는 개인 `MissionComponent`에 달림. 오브젝트 소진은 "이 액터는 이미 한 번 쓰였다"는 월드 상태로 전 플레이어 공용).
- **NPC 파이프라인 회귀**: `NPC/` 폴더·`ISpyNPCRoot`·`Server_RequestInteract`는 이번 변경에서 손대지 않는다(§1 핵심 결정) — 회귀 위험 없음.

---

## 7. 변경 파일 목록

**신규**
- `Interactable/SpyInteractableObject.h|.cpp`
- `Interactable/CommonInterface.Interactable.h`
- `System/Tests/SpyInteractableObjectTests.cpp`(또는 기존 미션 테스트 파일에 케이스 추가)

**수정**
- `Util/DefineEnum.h` — `ESpyMissionType::Interact` 추가
- `Util/SpyGameplayTags.h|.cpp` — `Event_Mission_Interact` 신규 태그
- `System/SpyMissionComponent.cpp` — 자동 수락·보상 경고 조건 2곳 확장
- `ManagerComponent/CommonInterface.Manager.h` — `ISpyInteractionHost::NotifyInteractableRangeChanged` 추가
- `ManagerComponent/SpyInteractionComponent.h|.cpp` — `NearbyInteractable` 필드, `NotifyInteractableRangeChanged`, `Server_RequestInteractObject`, `TryInteract` 확장

**에셋 (사용자, 이번 spec 범위 밖)**
- `BP_InteractableObject`(또는 C++ 클래스 직접 배치) — DevMap에 실제 배치, `MissionEventTag`/`InteractVerb` 지정
- `DA_SpyMissionConfig`에 `Interact` 타입 미션 엔트리 1개 추가(검증용)

---

## 8. 테스트 (Unreal Automation)

| 케이스 | 기대 |
|---|---|
| `ProcessProgress` — 새 인덱스가 `Interact` 타입 | `bAccepted == true`(자동 수락) |
| `ProcessProgress` — 새 인덱스가 `Gameplay` 타입(회귀 확인) | `bAccepted == false`(변경 없음) |
| `GrantReward` — `Interact` 타입 완료, 보상 행 없음 | 경고 로그 발생(Dialogue와 동일 취급) |
| `GrantReward` — `Gameplay` 타입 완료, 보상 행 없음(회귀 확인) | 경고 없음 |
| `ResolveMissionProgress`(기존, 미변경) — `Interact` 타입 미션에 매칭 태그 1회 | `bCompletedNow == true`, 인덱스 +1 |

**컴포넌트/액터 레벨은 Automation 커버 불가**(기존과 동일 이유 — `HasAuthority()`가 액터 컨텍스트 필요). PIE 확인 대상:
- `ASpyInteractableObject::RequestInteract` — 최초 1회 진행도 반영, 이후 재상호작용 시 `bConsumed` 가드로 무시
- 근접 시 프롬프트에 `InteractVerb` 텍스트가 올바르게 표시되는지(NPC의 "대화하기"와 다른 문구인지)
- 서버 거리 재검증(`IsPawnInRange`) — 클라 신고와 실제 오버랩 불일치 시 거부
- NPC 근처에서 오브젝트도 동시에 범위 안일 때 F키가 NPC를 우선하는지(§5-2)

**인게임 확인**: 1인 PIE — `DA_SpyMissionConfig`에 임시로 `Interact` 타입 미션 1개 추가 → 오브젝트 배치 → 근접 시 "조사하기" 프롬프트 표시 확인 → F 상호작용 → HUD 진행도 갱신 확인 → 재상호작용 시 프롬프트 사라짐(소진) 확인.
