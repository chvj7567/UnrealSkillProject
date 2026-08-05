# 미션 목표 바닥 길 안내 (Ground Path Navigation) — 도메인 기획 (톤 · UX · 데이터 배정)

> **이 문서의 범위**: [spec](../superpowers/specs/2026-08-04-mission-ground-navigation-design.md) 과 [plan](../superpowers/plans/2026-08-04-mission-ground-navigation.md) 이 확정한 **구조(데이터 스키마 · 컴포넌트 · 순수 함수 슬롯) 위에 얹을 수치와 UX 감각**만 결정한다. 클래스·파일·함수 시그니처는 plan 이 이미 확정했고 이 문서는 다시 제안하지 않는다 — 단, plan 에 없어서 구현 시 드롭될 위험이 있는 항목은 §7-5 에 "plan 개정 필요"로 명시한다(hud-mana-compass-skillbar.md §7-0 선례와 동일 처리).
>
> **이 문서가 뒤집는 이전 보류 결정**: [npc-mission-dialogue.md](npc-mission-dialogue.md) §7 은 "HUD 상 '다음 NPC 위치' 마커·미니맵 표시"를 "spec 비목표 범위 밖, §3-3-7 의 `Report` 문구 핸드오프로 대체"로 명시 제외했고, [hud-mana-compass-skillbar.md](hud-mana-compass-skillbar.md) §5·§8 은 "미션 웨이포인트·거리·미니맵·목표까지 화살표"를 spec §6 비목표로 명시 제외했다. 이번 기능은 그 두 보류를 **바닥 글로우 라인이라는 세 번째 형태**로 뒤집는다 — 화면 가장자리 마커나 미니맵이 아니라 월드 공간 바닥 라인이라는 점에서 두 문서가 다뤘던 형태와 겹치지 않으므로, 두 문서의 다른 결정(나침반 §5, 스킬바 §6 등)은 이 문서가 건드리지 않는다. `Report` 문구 핸드오프(§3-3-7)는 **대체되는 것이 아니라 병행**된다 — 텍스트 안내(다음 담당자 이름)와 시각 안내(바닥 라인)가 서로 다른 채널이라 상호 보강이다.

---

## §0 개정 이력 — 좌표 소스 전환 (2026-08-05)

**Mission_TargetLocation DataTable 수동 좌표 입력(§5 원안, 커밋 `818cad0`까지 구현·리뷰·테스트 통과 완료) 을 폐기하고, 레벨에 배치한 액터를 런타임에 자동 추적하는 방식으로 전환한다.** spec §2/§10 이 "NPC/목표지점 액터 자동 탐색·위치 레지스트리 없음"으로 명시 제외했던 결정을 이 개정이 뒤집는다 — 이 문서 도입부가 이미 npc-mission-dialogue.md/hud-mana-compass-skillbar.md 의 YAGNI 보류 2건을 뒤집은 전례가 있고, 이번이 세 번째다.

**배경**: 실사용 결과 레벨 디자이너가 좌표를 손으로 타이핑/동기화해야 하는 워크플로가 불편했다 — 레벨에서 액터를 눈으로 배치하고 그게 그대로 목표가 되길 원한다.

**바뀌는 것과 안 바뀌는 것**: 좌표의 **소스**만 바뀐다(DataTable 로우 → 레벨 배치 액터 self-registration). 톤·두께·애니메이션(§3), 시작 오프셋·도착 임계값·히스테리시스(§4), 갱신 주기(§6), 클라이언트 트리거 안전성(§2)은 좌표가 어디서 오든 `TargetLocation: FVector` 하나만 소비하므로 **전부 무관·무변경**이다. 바뀌는 절은 §1(일부 사실)·§5(전면 개정)·§7(구현 요청사항 추가)·§8(에디터 조건 갱신)·§10(제외 항목 조정)뿐이다.

**옛 설계는 재현하지 않는다**: 아래 §5 는 새 아키텍처만 서술한다. `Mission_TargetLocation` DataTable 의 스키마·저작 절차는 git 이력(커밋 `818cad0`)에 남아 있다 — 이 문서에 다시 옮겨 적지 않는다(중복 지시가 남으면 폐기된 방식을 구현하는 사고가 난다).

**(2026-08-05 추가 — 세션 시작 내비게이션 문제, 부트스트랩 미션이 아니라 §5-8 규칙으로 해소)** 세션 시작부터 플레이어가 첫 NPC(레이븐)와 접촉하기 전까지 "현재 미션이 있어도 아직 아무도 수락하지 않았다"는 상태라 이 문서의 원래 트리거(`OnMissionAccepted`)만으로는 안내가 시작되지 않는 닭-달걀 문제가 있었다. 한때 npc-mission-dialogue.md에 신규 부트스트랩 `MissionId`("레이븐과 접선")를 삽입해 세션 시작부터 자동 수락 상태를 만드는 방식으로 풀려 했으나 **그 접근은 폐기됐다.** 대신 이 문서 §5-8이 "현재 미션이 미수락이면 담당 NPC로, 수락됐으면 기존 로직으로" 안내하는 규칙을 신설해 같은 문제를 미션 데이터 변경 없이 해소한다 — npc-mission-dialogue.md의 미션 체인은 12행 원본 그대로다.

**(2026-08-05 추가 — Kill/Level/Combo 마커 배치, §5-1 원칙 3항 폐기)** **사용자가 "장소 무관 `Gameplay` 미션(Kill/Level/Combo)은 대상이 없다"는 §5-1 원칙 3항을 뒤집었다.** 코디네이터가 unreal-mcp 로 DevMap 전투 구역(봇 스폰 지점 인근, X≈1380-2600·Y≈-1400~-2600) 근처 서로 다른 진입점에 `ASpyMissionTargetPoint`(Vault/Climb/GrappleHook 용으로 이미 존재하던 마커 클래스, 신규 코드 없음) 3개를 추가 배치했다 — `Event.Mission.Kill`(1700,-1700,96), `Event.Mission.Level`(2000,-1400,96), `Event.Mission.Combo`(1380,-2020,96). §8 조건 3("마커 좌표는 구역 진입점이지 특정 오브젝트가 아니다")과 동일 원칙을 따랐다. 레지스트리의 `MatchTag` 조회 로직은 애초에 태그 종류를 가리지 않는 범용 로직이라(§5-4) 이 확장에 신규 아키텍처·코드 요구사항이 없다 — Gameplay 미션 6종(Vault/Climb/GrappleHook/Kill/Level/Combo) 전부가 이제 §5-1 2항 하나로 통합된다. 바뀌는 절: §5-1·§5-2·§5-3·§5-5·§5-6·§7-6·§8·§9·§10.

---

## § 헤더

- **목표** — 미션 수락 시점부터 완료까지, NavMesh 로 계산한 경로를 바닥에 시안 글로우 라인으로 표시하는 로컬 클라이언트 전용 연출의 톤·UX 수치(색상·두께·시작/소멸 임계값·갱신 주기)와, 어느 미션에 목표 좌표를 채울지의 원칙을 확정한다.
- **검증 가설** — 바닥 라인이 (1) 기존 프로젝트 UI 톤(시안 계열)과 이질감 없이 읽히고, (2) 목표 도착 시점에 뚝 끊기거나 발밑에 뜬 조각으로 보이지 않으며, (3) 대화형(NPC 보고) 미션과 파쿠르형(구역 진입) 미션 양쪽에서 "지금 어디로 가야 하는가"가 산수 없이 읽히는가.
- **현재 단계 범위 적합성** — `project.md` 의 `stage`/`stage_goal` 과 컨셉서 §2·§4·§5 는 **(사용자 확정 대기)** 다. mission-system.md·npc-mission-dialogue.md·hud-mana-compass-skillbar.md 의 선례를 따라, **사용자와의 브레인스토밍·계획 수립이 끝난 spec(`2026-08-04-mission-ground-navigation-design.md`)의 목표·비목표를 이 문서의 범위 경계로 삼는다.** spec §2 "제외" 항목(NPC 위치 자동 탐색·다중 미션 동시 표시·미니맵/공중 마커·서버 동기화)은 이 문서에서도 다루지 않는다.
- **핵심 메커니즘** — `USpyNavigationComponent`(로컬 컨트롤 클라이언트 전용, 서버/타 플레이어에 레플리케이트하지 않는 순수 연출)가 미션 수락/완료 신호를 구독해 NavMesh 경로를 주기 재계산하고 `USplineMeshComponent` 풀로 바닥 라인을 렌더링한다. **"로컬 전용 렌더링"과 "로컬에서 트리거가 실제로 오는가"는 별개 문제다** — §2 가 이 둘의 간극을 다룬다. 나머지(§3~§6)는 순수 로컬 반응성(입력이 아니라 "미션 상태 변화 → 연출 반응" 체감)의 문제이며, 이 기능 자체는 서버 판정을 기다리는 입력 경로가 없으므로 네트워크 페이싱 축은 적용되지 않는다.

---

## §1 확정 사실 — 입력으로만 쓰는 값 (spec/plan, 재확정 안 함)

| 사실 | 값 | 출처 |
|---|---|---|
| ~~`Mission_TargetLocation` 선택적 관계 테이블~~ — **§0 개정으로 폐기.** 좌표 소스는 레벨 배치 액터(§5) | — | §0 |
| `USpyNavigationComponent` 는 로컬 컨트롤 클라이언트 전용, 서버/데디케이티드에서 렌더링 없음 | — | spec §5 |
| 갱신 주기 `UpdateIntervalSeconds = 0.75f`(`EditDefaultsOnly`) — §6 이 승인·확정, 실제 구현 완료 확인 | 0.75초 | spec §5, plan Task 4 Step 1, 코드 실측(`SpyNavigationComponent.h:68`) |
| 렌더링은 `USplineComponent` 1개 + `USplineMeshComponent` 세그먼트 풀 재사용(destroy/spawn 반복 없음) — 구현 완료 확인 | — | spec §6, 코드 실측(`SpyNavigationComponent.cpp`) |
| 글로우 머티리얼은 `USpyAssetManager` 이름 룩업(`"NavPathGlow"`), 실제 머티리얼 에셋 제작은 스펙 범위 밖(아트 작업) | — | spec §6, plan Task 5 Step 5 |
| ~~`USpyMissionComponent::GetMissionTargetLocation(int32)` 패스스루~~ — **§0 개정으로 제거 대상.** §5-4·§7-6 참조 | — | §0 |
| `USpyNavigationComponent` 는 `USpyMissionComponent` 를 인터페이스가 아니라 구체 클래스로 직접 참조 (기존 `USpyMainHUD` 선례) — 신규 레지스트리 서브시스템도 동일 원칙(직접 참조, §5-4) | — | plan Global Constraints, spec §8 |
| **미션 스키마 SoT — `FSpyMissionRow`(`SpyMissionConfig.h`)** — `MissionId`(1-based, 12행)·`MissionType`(`Gameplay`/`Dialogue`)·`NPCId`(직접 필드, sentinel `NoNPCId=9999`)·`MatchTag`(`FGameplayTag`) | — | `SkillProject/Source/SkillProject/Data/SpyMissionConfig.h:24-63` (코드 실측) |
| `ASpyNPCCharacter` 는 이미 `int32 NPCId`(`EditDefaultsOnly`) 를 갖고 `BeginPlay` 에서 `CacheNPCData()` 를 1회 호출 — 레지스트리 자기등록 훅 지점(§5-2) | — | `SpyNPCCharacter.h:52-53`, `.cpp:24-32` (코드 실측) |
| `ASpyInteractableObject` 는 `FGameplayTag MissionEventTag`(`EditAnywhere`) 를 이미 갖고 `FSpyMissionRow.MatchTag` 와 매칭한다 — NPCId 는 없음 | — | `SpyInteractableObject.h:64-66` (코드 실측) |

**⚠ 스키마 정정**: npc-mission-dialogue.md §4 는 `NPCId` 를 별도 `MissionCommunication` 관계 테이블로 모델링했으나, 코드 실측 결과 현재 `FSpyMissionRow` 는 `NPCId` 를 **직접 필드로** 갖고 있다(2026-08-03 이후 리팩터). 이 문서는 npc-mission-dialogue.md 의 5테이블 모델이 아니라 **`SpyMissionConfig.h` 실물을 스키마 SoT로 삼는다** — §5 의 `MissionId`↔`NPCId` 매핑도 이 실측 기준이다.

---

## §2 ⚠ 트리거 신호의 클라이언트 안전성 — 이 기능이 통째로 죽을 수 있는 단일 실패점

**결론 먼저: plan Task 2 Step 4 가 정한 대로 `OnMissionAccepted` 를 `AcceptCurrentMission()`/`ProcessProgress()` 안에서만 브로드캐스트하면, 데디케이티드 서버 + 원격 클라이언트 구성에서 `USpyNavigationComponent` 는 영원히 트리거되지 않는다.** 이 문서가 §3~§9 에서 정하는 색상·두께·임계값은 전부 이 신호가 클라이언트에 실제로 도착한다는 전제 위에 있으므로, 이 절이 이 문서에서 가장 먼저 확정돼야 하는 항목이다.

### 2-1. 코드 실측 — 무엇이 클라이언트에서 이미 안전하고 무엇이 아닌가

`SpyMissionComponent.cpp:91-100` 의 `OnRep_MissionState()`:

```cpp
void USpyMissionComponent::OnRep_MissionState()
{
    OnMissionProgressChanged.Broadcast(this, MissionState.MissionIndex, MissionState.Count, GetTargetCount());

    if (IsAllCompleted())
        OnAllMissionsCompleted.Broadcast(this);
}
```

`MissionState` 는 `ReplicatedUsing = OnRep_MissionState`(`COND_OwnerOnly`)다 — 즉 **`OnMissionProgressChanged` 와 `OnAllMissionsCompleted` 는 원격 클라이언트에서도 정확히 발화한다.** 반면 `AcceptCurrentMission()`(`:188-216`)과 `ProcessProgress()`(`:153-186`) 안의 `OnMissionCompleted.Broadcast(...)` 호출, 그리고 plan Task 2 가 추가하려는 `OnMissionAccepted.Broadcast(...)` 호출은 **`HasAuthority()` 가 참인 경로에서만 실행되는 일반 멀티캐스트 델리게이트 호출**이다 — RPC 도 레플리케이션도 아니므로, **서버 프로세스에서 `Broadcast()` 를 호출한 그 UObject 인스턴스에서만 리스너가 실행된다.**

리슨 서버 1인 PIE 에서는 호스트의 폰·클라이언트 역할이 같은 프로세스의 같은 오브젝트라 우연히 정상 동작한다. 하지만 이 프로젝트의 확정 아키텍처는 **데디케이티드 서버 멀티플레이어**(`project.md` one_liner)다 — 원격 클라이언트의 `USpyMissionComponent` 는 서버 오브젝트와 다른 UObject 인스턴스(레플리케이트 프록시)이고, `AcceptCurrentMission()`/`ProcessProgress()` 는 그 프록시에서 전혀 실행되지 않는다(서버 권한 가드로 조기 반환). **결과: 원격 클라이언트에서 `OnMissionAccepted`/`OnMissionCompleted` 는 한 번도 발화하지 않는다.**

### 2-2. 무증상 실패 시나리오

| 구성 | 증상 |
|---|---|
| 1인 PIE(리슨 서버=클라이언트 동일 프로세스) | 정상 동작 — QA 에서 관측되지 않는다 |
| 2인 PIE(호스트) | 호스트 화면은 정상, **원격 클라이언트 화면에서 바닥 라인이 영구적으로 나타나지 않는다** |
| 데디케이티드 서버 + 클라이언트(실 배포 형태) | **모든 플레이어 화면에서 바닥 라인이 영구적으로 나타나지 않는다** — 서버엔 렌더링이 없고(§1), 클라이언트는 트리거를 못 받는다 |

이는 mission-system.md §4-4 전제 ③(서버에서 레벨 신호가 잘못된 경로로 발신돼 영영 도착하지 않던 결함)과 같은 계열의 문제이고, hud-mana-compass-skillbar.md §7-1(`MaxMana` 미설정 시 전 스킬 영구 발동 불가)과 같은 무게의 **단일 실패점**이다. memory `project_engine_no_server_target` 이 이미 지적하듯 이 프로젝트는 데디케이티드 서버 구성으로 거의 검증되지 않으므로, 이 결함은 고쳐지지 않으면 **다음 데디케이티드 검증 시점까지 발견되지 않을 가능성이 높다.**

### 2-3. 요구 수정 — `OnRep_MissionState` 로 트리거를 이전한다

**`OnMissionAccepted`(신규)와 `OnMissionCompleted`(기존)는 `AcceptCurrentMission()`/`ProcessProgress()` 안에서의 브로드캐스트만으로 끝내지 않고, `OnRep_MissionState()` 에서도 상태 변화를 판정해 동일하게 발화해야 한다.** 정확히는 —

- `OnRep_MissionState` 가 레플리케이트 **직전 값**을 파라미터로 받도록 확장한다(UE 표준 패턴: `OnRep_MissionState(FSpyMissionState OldMissionState)` — 프로퍼티 자체는 이미 새 값으로 갱신된 상태에서 이전 값을 별도로 받는다).
- **⚠ 발화 순서 고정 — 완료 판정을 수락 판정보다 먼저 실행한다.** 서버측 `ProcessProgress()`(`SpyMissionComponent.cpp:153-186`)가 이미 이 순서다 — `OnMissionCompleted.Broadcast(...)`(라인 168, 상태 갱신 **전**) 다음에 상태를 갱신하고, 그 다음에야 새 미션의 자동 수락 판정이 일어난다(라인 174-182). `OnRep_MissionState` 도 이 순서를 그대로 따라야 한다 — 뒤바꾸면(수락을 먼저 발화하면) `USpyNavigationComponent::HandleMissionAccepted` 가 새 경로를 막 시작한 직후 `HandleMissionCompleted`(인덱스를 보지 않고 무조건 `StopPath()` 호출, plan Task 4 확정 설계)가 그 경로를 곧바로 지워버린다 — Dialogue 미션 완료와 동시에 다음 Dialogue 미션이 자동 수락되는 모든 전이(§5-2 "채움" 확정 6행: `MissionId` 2/4/6/8/10/12)에서 매번 재현된다.
  1. **완료 판정 (먼저)**: `OldMissionState.MissionIndex != MissionState.MissionIndex` 이면 `OnMissionCompleted.Broadcast(this, OldMissionState.MissionIndex)` — 완료된 인덱스는 이전 값이다.
  2. **수락 판정 (다음)**: `OldMissionState.bAccepted == false && MissionState.bAccepted == true` (명시적 수락) **또는** `OldMissionState.MissionIndex != MissionState.MissionIndex && MissionState.bAccepted == true` (새 미션 진입과 동시 자동 수락, Dialogue 타입) 이면 `OnMissionAccepted.Broadcast(this, MissionState.MissionIndex)`.
- 델리게이트 브로드캐스트는 동기 호출이므로, 이 순서만 지키면 `StopPath()` 가 먼저 실행되고 그 다음 `StartPathTo()` 가 실행돼 최종 상태가 "새 미션 경로 활성"으로 올바르게 끝난다. 재접속(late-join, 서버가 이미 여러 전이를 거친 뒤 클라이언트가 처음 레플리케이션을 받는 경우)도 같은 순서로 동일하게 처리된다 — 병합된 스냅샷이라도 완료 판정이 먼저 실행되므로 결과가 달라지지 않는다.
- 이미 존재하는 `OnMissionProgressChanged`/`OnAllMissionsCompleted` 처리와 나란히 같은 함수 안에 두되, **완료 → 수락 → 기존 두 처리** 순서를 지킨다.

**⚠ 이 판정은 서버의 매 트랜지션과 1:1 로 대응하지 않는다 — 그래도 내비게이션 목적에는 충분하다.** `MissionState` 는 이벤트 스트림이 아니라 **레플리케이트 시점의 스냅샷**이다. 한 번의 레플리케이션 윈도우 안에서 서버가 미션을 두 번 이상 연속 전진시키면(`SpyMissionComponent.cpp:143-150` 의 `PendingEvents` 드레인 루프, 또는 "보상 XP → 레벨업 → 레벨 신호 → `AddProgress`" 재진입 — 둘 다 이 코드베이스에 실재하는 경로) 클라이언트는 중간 상태를 보지 못하고 최종 상태만 받는다. 예: 서버가 미션 7 완료 → 8 진입(자동 수락) → 8 완료(보상이 유발한 재진입) → 9 진입을 한 윈도우에서 처리하면, 서버는 `OnMissionCompleted` 를 (7), (8) 두 번 발화하지만 클라이언트 diff 는 `OldMissionState.MissionIndex=7 → MissionState.MissionIndex=9` 한 번만 관측해 `OnMissionCompleted(7)` 한 번만 발화한다.
**내비게이션은 이 손실에 영향받지 않는다** — `HandleMissionCompleted` 는 완료된 인덱스를 쓰지 않고 무조건 `StopPath()` 만 호출하고, `HandleMissionAccepted` 가 필요로 하는 것은 "현재 인덱스 + 현재 수락 여부"뿐이며 이는 병합된 최종 상태에서도 정확하다. 요구되는 것은 **매 완료 이벤트 단위의 정합이 아니라 최종 상태의 정합**이다 — gameplay-programmer 는 서버와 클라이언트의 발화 횟수를 맞추는 시퀀스 번호 같은 추가 장치를 만들 필요가 없다.
**리슨 서버 호스트에서 중복 발화도 없다** — `OnRep_MissionState` 는 권한(authority) 인스턴스에서 실행되지 않으므로, 호스트는 `AcceptCurrentMission()`/`ProcessProgress()` 안의 서버 경로 브로드캐스트만 받고 `OnRep` 경로는 타지 않는다.

**이 수정은 이 문서의 새 요구가 아니라 plan Task 2 의 개정 항목이다** — §7-5 에서 "plan 개정 필요"로 다시 명시한다. **이 수정 없이 이 문서의 나머지 결정(§3~§9)은 실물 배포 구성에서 관측 불가능하다.**

### 2-4. 이 절이 §2 외 나머지 절과 갖는 관계

§3(색상)~§9(플레이테스트 메트릭)는 전부 "트리거가 온다"는 전제 위에서만 의미가 있다. 구현 순서상 **§2-3 수정이 plan Task 2 안에서 가장 먼저 끝나야 하고**, Task 4(`USpyNavigationComponent` 상태 머신) 의 수동/자동 테스트도 **2인 이상 PIE(호스트+클라이언트)** 로 재현해야 이 결함이 실제로 잡혔는지 확인된다 — 1인 PIE 만으로는 §2-2 표의 "정상 동작"만 보고 결함을 통과시킬 수 있다.

**§5-8(2026-08-05)과의 관계**: §5-8은 `USpyNavigationComponent` 가 구독하는 델리게이트를 `OnMissionAccepted`/`OnMissionCompleted`/`OnAllMissionsCompleted` 3개에서 `OnMissionProgressChanged` 1개로 좁힌다. 이 절이 확립한 안전성 분석은 그대로 유효하다 — `OnMissionProgressChanged` 는 애초에 `OnRep_MissionState` 안에서 무조건 발화해 온 델리게이트라(§2-1), §2-3의 개정 없이도 처음부터 원격 클라이언트에서 안전했다.

---

## §3 시각적 톤 — 색상 · 두께 · 애니메이션 (spec 미확정 영역)

spec §1 은 배경 설명에서 "Fable 스타일 골드 트레일"이라는 비유를 쓰지만, spec §6(렌더링) 자체는 "글로우 머티리얼"이라고만 정하고 색상·두께·애니메이션은 확정하지 않는다. 이 절이 그 갭을 채운다.

### 3-1. 색상 — 시안, spec 의 "골드" 비유를 명시적으로 기각

| 안 | 색상 | 근거 | 판정 |
|---|---|---|---|
| A. 시안(프로젝트 기존 팔레트) | **`rgba(0.373, 0.816, 0.851, 1.0)`** | `WBP_Dialogue`/`WBP_MissionOffer` 에서 이미 쓰는 실측값(`.claude/.active-sessions.md` 세션 기록 — 다크 패널 `rgba(0.063,0.082,0.102,0.90)` 과 짝을 이루는 시안, 코너브래킷 알파 0.35). session-browser.md §4-2-4 의 `#5AC8D8`(RGB 0.353/0.784/0.847) 와 **근접하지만 동일하지 않다** — 이 문서는 실제로 화면에 렌더되는 위젯 실측값(0.373/0.816/0.851)을 채택한다 | **채택** — 대화창·미션 카드·나침반·마나바까지 이미 시안이 이 프로젝트의 "스파이 테크 HUD" 정체성이다(§3-1-1). 새 액센트 색을 추가하지 않고 기존 톤에 편입 |
| B. 골드/warm 트레일(spec §1 비유 그대로) | 앰버 계열 | Fable 류 게임의 관습적 퀘스트 트레일 색 | 기각 — 이 프로젝트에 "보물/퀘스트 안내"라는 인게임 모티프가 없고, 골드를 도입하면 기존 시안 정체성과 **두 번째 액센트 색**이 공존해 톤이 갈린다. spec §1 문구는 "연속 글로우 라인"이라는 **형태**의 비유였지 색상 확정이 아니다 |
| C. 중립 백색/은색 홀로그램 | 흰색 계열 | 일부 AR/HUD 연출에서 쓰는 무채색 가이드 | 기각 — 액션 스킬 아이콘의 흰색 키힌트(hud 문서 §6-1)와 겹쳐 "이것도 UI 텍스트인가" 하는 혼동 유발. 월드 공간 요소는 UI 요소와 색으로 구분돼야 한다 |

**결정: A(시안 `rgba(0.373, 0.816, 0.851, 1.0)`) 채택. spec §1 의 "골드" 비유는 이 문서가 명시적으로 기각한다** — 형태(글로우 라인)만 유지하고 색은 프로젝트 기존 팔레트로 통일한다.

**3-1-1. 시안이 이미 이 프로젝트의 HUD 언어인 근거**: `WBP_Dialogue`/`WBP_MissionOffer`(대화·미션 카드) 뿐 아니라 hud-mana-compass-skillbar.md §6-1 이 마나바·코스트 숫자를 "청색(마나바와 동색)"으로 확정했고, session-browser.md §4-2-4 는 로고 타이포 자체가 시안이다. 바닥 라인에 같은 계열을 쓰면 "이 게임의 정보 안내는 전부 시안"이라는 일관된 학습이 성립한다.

### 3-2. 두께

| 안 | 폭(스플라인 중심 기준 좌우) | 판정 |
|---|---|---|
| A. 4cm | 전체 폭 8cm | 기각 — 이동 중·원거리에서 시인성이 떨어진다 |
| **B. 10cm(권장)** | **전체 폭 10cm(중심에서 좌우 5cm)** | **채택 — 좁은 리본형으로 지형·파쿠르 오브젝트 실루엣을 가리지 않으면서 바닥에서 뚜렷이 도드라지는 최소 폭** |
| C. 30cm | 전체 폭 30cm | 기각 — "길을 표시하는 선"이 아니라 "통행로/장벽"처럼 보여 파쿠르 존의 작은 지형 디테일을 가린다 |

**결정: 전체 폭 10cm.** 실제 스태틱 메시/머티리얼 제작(아트 작업, spec §6 범위 밖)은 이 치수를 기준으로 삼는다.

### 3-3. 애니메이션 톤 (머티리얼 제작 가이드 — 코드 아님)

- **UV 패닝**: 플레이어 → 목표 방향으로 흐르는 텍스처 패닝. "지금 어느 방향으로 가야 하는가"가 라인의 정적인 형태뿐 아니라 흐름 방향으로도 읽히게 한다.
- **펄스**: 낮은 주기의 은은한 밝기 맥동(브레싱) — 라인이 "죽은 장식"이 아니라 "활성 안내"임을 느리게 반복해서 알린다. 정확한 패닝 속도·펄스 주기(초 단위 셰이더 상수)는 머티리얼 제작 단계(아트, spec §6 범위 밖)에서 결정한다 — 이 문서는 방향성·느낌만 지정한다.

---

## §4 발밑 시야 가림 방지 + 도착 임계값 — 합성 규칙

과제에서 요구한 두 UX 결정(시작점 오프셋, 도착 임계 거리)은 **독립적으로 정하면 서로 충돌한다.** 이 절은 그 충돌을 확인하고 하나의 합성 규칙으로 해소한다.

### 4-1. 개별 값 후보

**시작 오프셋** — 3인칭 오버숄더 카메라 구도에서는 발밑 바로 아래 라인이 캐릭터 모델·카메라 근접 평면에 가려 잘 안 보이고, 화면 하단에 걸쳐 시야를 가릴 수 있다. 라인 렌더링을 플레이어 위치가 아니라 경로를 따라 일정 거리 진행한 지점부터 시작한다.

| 안 | 오프셋 | 판정 |
|---|---|---|
| A. 0cm(발밑부터) | 시야 가림 방지 없음 | 기각 — 원과제 |
| **B. 100cm(권장)** | | **채택 — 발밑 클리핑을 피하면서 "캐릭터로부터 계속 이어지는" 연결감은 유지하는 최소값** |
| C. 300cm | | 기각 — 라인이 캐릭터로부터 눈에 띄게 분리돼 "내 라인"이 아니라 "저 앞의 표시"처럼 읽힌다 |

**도착 임계 거리** — 목표에 매우 근접하면 NavMesh 경로 길이가 0에 가까워져 스플라인 세그먼트가 극단적으로 짧아지거나(글로우 텍스처가 압축/왜곡돼 보이는 렌더링 아티팩트) 매 갱신마다 깜빡일 수 있다. 이 거리 미만이면 라인을 아예 숨긴다.

| 안 | 임계 거리 | 근거 | 판정 |
|---|---|---|---|
| A. 100cm | | 임계값이 너무 가까워 세그먼트 왜곡 구간(§4 서두)이 이미 시작된 뒤에야 숨겨진다 | 기각 |
| **B. 300cm(권장)** | | **npc-mission-dialogue.md §5-1 이 확정한 NPC 상호작용 판정 반경(300cm)과 동일값.** Dialogue 타입(보고) 미션은 이 거리에서 이미 "F 대화하기" 프롬프트가 뜨므로, 그 지점부터 라인을 숨기면 프롬프트 UI 로 자연스럽게 주의가 전환된다 | **채택** |
| C. 500cm | | 상당한 거리가 남았는데도 안내가 사라져 마지막 접근 구간에서 방향을 잃는다 | 기각 |

**⚠ 300cm 는 대화형(NPC 보고) 미션에만 독립적으로 도출된 값이다.** §5 가 마커 액터를 배치하는 파쿠르 구역 미션 3종(넘기/벽타기/그래플)에는 대응하는 상호작용 판정 반경이 없다 — 이 값을 재사용하는 건 "하나의 통일된 규칙을 유지한다"는 단순성 근거이지 독립 도출이 아니다. 구역 미션에서 너무 빠르거나 늦게 사라지면 §9 M-nav-2 로 개별 조정한다.

### 4-2. 두 값을 합성하지 않으면 생기는 문제

시작 오프셋(100cm)과 도착 임계(300cm)를 각각 독립적으로 적용하면: 남은 경로 길이가 320cm 로 줄어든 시점에 라인은 "플레이어로부터 100cm 지점 ~ 320cm 지점"의 **220cm 짜리 조각**으로 렌더링된다 — 발밑에서 시작하지 않고 공중에 떠 있는 듯한 조각으로 보이다가, 다음 갱신에서 300cm 미만으로 떨어지는 순간 **통째로 사라진다.** 이 "발밑에서 분리된 조각 → 뚝 끊김"이 바로 과제가 지적한 "도착 시 어색해 보이는" 상황이다.

### 4-3. 합성 규칙 (단일 상태 머신)

세 상수만 쓴다. 세 번째는 앞의 둘로부터 **유도된 값**이지, 독립적으로 고른 값이 아니다.

```
ArrivalHideDistanceCm   = 300   (§4-1 B — NPC 상호작용 반경 재사용)
StartOffsetDistanceCm   = 100   (§4-1 B)
ArrivalReshowDistanceCm = ArrivalHideDistanceCm + StartOffsetDistanceCm = 400   (검산: 300 + 100 = 400)
```

매 `RecomputePath` 갱신마다(§6 의 0.75초 주기), NavMesh 경로점 배열의 **누적 길이**(직선 거리 아님 — 우회가 필요한 경로에서 직선 거리를 쓰면 벽 뒤에 있는데도 라인이 일찍 사라지는 오류가 생긴다)를 `RemainingPathLength` 로 계산하고, 표시 상태를 아래 순서로 판정한다:

1. **가시성 히스테리시스** (깜빡임 방지) — 이전 프레임에 라인이 **숨겨져** 있었다면 `RemainingPathLength >= ArrivalReshowDistanceCm(400)` 일 때만 다시 보인다. 이전 프레임에 **보이고** 있었다면 `RemainingPathLength < ArrivalHideDistanceCm(300)` 일 때만 숨겨진다. 300~400cm 구간(폭 100cm)에서는 직전 상태를 유지한다.
2. **시작 오프셋 축소** — 라인이 보이는 상태일 때, `RemainingPathLength >= ArrivalHideDistanceCm + StartOffsetDistanceCm(400)` 이면 시작점을 `StartOffsetDistanceCm(100)` 만큼 트리밍한다(§4-1 의도 그대로). 그 미만이면 오프셋을 0 으로 낮춰 **플레이어 실제 위치부터** 렌더링한다 — 라인이 목표를 향해 점진적으로 줄어들다 300cm 아래에서 자연스럽게 사라지도록, "발밑에서 분리된 조각"이 생기지 않게 한다.

**⚠ 콜드 스타트(경로를 막 시작하는 첫 프레임)는 "이전 프레임에 보이고 있었다"로 취급한다.** 규칙 1은 "이전 프레임의 가시성"을 전제하는데, 미션을 막 수락해 경로를 처음 만드는 프레임에는 지킬 이전 상태가 없다 — 이걸 임의로 "이전에 숨겨져 있었다"로 두면 목표까지 남은 거리가 300~400cm 구간(또는 그 미만)인 채로 미션이 시작될 때 `RemainingPathLength >= 400` 을 만족하지 못해 **그 미션이 끝날 때까지 라인이 한 번도 보이지 않는** 결함이 생긴다(플레이어가 우연히 400cm 밖으로 멀어졌다 돌아와야 처음 표시됨 — §4-2 가 없애려던 "발밑 분리 조각"보다 나쁜 "아예 안 보임"). 콜드 스타트를 "이전에 보이고 있었다"로 시드하면 규칙 1의 두 번째 문장(`< 300` 일 때만 숨김)이 즉시 적용돼, 시작 시점에 이미 300cm 미만이 아닌 한 첫 프레임부터 정상 표시된다. 구현상으로는 `StartPathTo()` 호출 시점에 가시성 상태를 `true` 로 초기화한 뒤 첫 `RecomputePath` 의 히스테리시스 판정을 그 상태 위에서 그대로 돌리면 된다 — 콜드 스타트 전용 별도 분기를 만들 필요는 없다.

이 두 규칙이 같은 경계값(400)을 공유하므로 상수가 늘어나지 않는다 — "오프셋이 꺼지는 지점"과 "다시 보이는 지점"이 동일 지점이라는 것 자체가 §4-2 문제의 해소다.

**한계(허용, 2가지)**:
1. `MaxWalkSpeed = 500cm/s`(§6, **걷기 기준** — 더 빠른 이동 수단이 있다면 더 자주 발생 가능) 로 목표를 향해 직진하면 0.75초에 375cm 를 이동하므로(§6), 300~400cm 폭 100cm 구간을 한 틱에 건너뛸 수 있다 — 이 경우 "발밑부터 서서히 줄어드는" 중간 프레임이 생략되고 오프셋 트리밍 상태에서 바로 숨김으로 전환된다. 상태 판정 자체는 깨지지 않으므로(순간 전환일 뿐 진동이 아니다) 결함으로 보지 않는다 — §9 M-nav-2 로 체감을 관측한다.
2. **재접근(재표시) 순간에는 §4-2 가 없애려던 "발밑에서 분리된 조각"이 규칙 2 항 그대로 한 번 더 나타난다** — 라인이 숨겨진 상태에서 `RemainingPathLength` 가 400 을 넘어 다시 보이는 그 프레임은 이미 `>= 400` 조건을 만족하므로 규칙 2 가 곧바로 시작 오프셋(100cm)을 활성화하고, 근접 단(0→100cm)이 한 틱 안에 스냅한다. **최초 접근(§4-2 가 다루는 "도착" 방향)에서는 발생하지 않는다** — 그쪽은 오프셋이 먼저 꺼지고 나중에 숨겨지는 순서라 매끄럽다. 발생 방향은 "라인 재표시(이탈 후 재접근)" 한 경우로 한정되며, 그 빈도(같은 목표 주변을 배회하며 300~400cm 경계를 넘나드는 플레이)가 낮다고 보고 이번 범위에서는 별도 상수를 추가하지 않는다(YAGNI) — §9 M-nav-3 가 이 방향도 함께 관측한다.

---

## §5 목표 위치 데이터 소스 — 레벨 배치 액터 자동 추적 (2026-08-05 개정)

§0 이 요약한 대로, 좌표의 **소스**가 DataTable 수동 입력에서 **레벨에 실제로 배치된 액터**로 바뀐다. 어느 `MissionId` 가 목표 지점을 갖는가(§5-1 원칙)는 §0 개정 시점 기준 9채움/3비움이었으나, **2026-08-05 Kill/Level/Combo 마커 배치로 12채움/0비움으로 갱신됐다**(§5-1 참조) — §0 이 다루는 "좌표 소스 전환" 자체는 이 변화와 무관하며, 그 12개의 좌표를 "누가, 어떻게" 제공하느냐만 바뀐다.

### 5-1. 원칙 (2026-08-05 재개정 — Kill/Level/Combo 마커 배치로 Gameplay 6종 전부 대상 보유)

1. **`MissionType == Dialogue`(NPC 보고) 미션은 전부 대상이 있다.** 담당 NPC(그 미션 로우의 `NPCId`)가 이미 레벨에 배치돼 있고(npc-mission-dialogue.md §4), 그 NPC 의 월드 위치가 곧 목표 지점이다 — **신규 데이터 저작이 필요 없다.**
2. **`MissionType == Gameplay` 미션 6종(Vault/Climb/GrappleHook/Kill/Level/Combo) 전부 전용 마커 액터를 갖는다.** Vault/Climb/GrappleHook 3종은 §5-3 이 정한 "구역 결부" 이유로 이미 전용 마커가 필요했고, **Kill/Level/Combo 3종도 2026-08-05 사용자 결정으로 전투 구역(봇 스폰 지점 인근) 서로 다른 진입점에 `ASpyMissionTargetPoint`(§5-4, 신규 코드 없이 기존 마커 클래스 재사용)를 배치해 이 원칙에 편입됐다** — 배치 원칙은 §8 조건 1·2·3 참조.
3. ~~장소 무관 `Gameplay` 미션(Kill/Level/Combo)은 대상이 없다~~ — **2026-08-05 폐기, 위 2항으로 통합됐다.** 이 3종에 대응하는 마커 액터가 더 이상 부재하지 않으므로, §14-1 선택적 관계 원칙("로우 없음 → 관계 없음")은 이제 이 3종에는 적용되지 않는다 — 이 3종도 Vault/Climb/GrappleHook 과 동일하게 "로우(마커) 있음 → 관계 있음"으로 성립한다.
4. **`ESpyMissionType::Interact` 는 대상이 오브젝트일 수 있어 별도 취급이 필요하다** — §5-3 참조. 현재 12행 데이터에 `Interact` 행은 없다.

### 5-2. `MissionType` 이 1차 게이트다 — `NPCId` 는 그 분기 안에서의 유효성 확인일 뿐, 분기 판정 기준이 아니다

**⚠ 이 항목이 이번 개정의 핵심 함정이다.** `FSpyMissionRow.NPCId` 가 채워져 있다는 사실 **자체**를 "이 미션에 목표 지점이 있다"의 판정 기준으로 쓰면 안 된다. Kill(1)/Level(3)/Combo(5) 는 담당 NPC(레이븐/팰컨/바이퍼 — 그 미션을 Offer 하는 NPC)의 `NPCId` 를 갖고 있을 가능성이 높다(`ResolveNPCNameHintText` 가 `bAccepted == false` 인 모든 미션에 대해 "NPC 찾아가세요" 힌트를 표시하므로 — `SpyMainHUD.cpp:225-228`, 이 힌트가 Gameplay 타입에도 뜨려면 그 타입도 `NPCId` 를 가져야 한다. **다만 라이브 `DT_SpyMission` 값을 직접 조회하지는 않았으므로 확정 사실이 아니라 강한 정황이다 — gameplay-programmer 구현 시 1회 확인.**).

이 정황이 맞든 틀리든 결론은 같다: **`Entry->MissionType == ESpyMissionType::Dialogue` 로만 분기 판정한다는 규칙은 "수락된 미션의 목표"에 한정된 규칙이다.** `NPCId` 값의 존재/부재로 "목표 지점 있음"을 판정하면 Kill/Level/Combo 도 (자신들의 `NPCId` 가 우연히 채워져 있다는 이유만으로) "그 NPC 에게 되돌아가라"는 **잘못된** 라인을 얻게 된다 — 이 3종의 수락 후 정확한 목표는 §5-1 2항(2026-08-05 재개정)이 정한 전용 마커(`MatchTag` 키 공간)이지 담당 NPC(`NPCId` 키 공간)가 아니다. `NPCId` 는 **`MissionType == Dialogue` 로 이미 분기를 탄 다음**, `Entry->NPCId != NoNPCId` 를 그 분기 **안에서의 유효성 확인**(§5-5)으로만 쓴다 — 분기 자체를 결정하지 않는다. **단, §5-2-1 이 이 규칙 앞에 미수락 게이트를 하나 더 추가한다 — 아래 참조.**

### 5-2-1. 미수락 시 게이트 확장 — `MissionType` 무관, 담당 `NPCId` 로 항상 안내 (2026-08-05 개정)

**배경 — 세션 시작 닭-달걀 문제.** §5-2의 규칙(§5-1~§5-7 원안)은 **수락된 미션**의 목표를 어떻게 찾는가만 다룬다. `HandleMissionAccepted`(§5-5)가 유일한 트리거였으므로, **미션이 아직 미수락인 동안은 안내할 대상이 아예 없었다** — 세션 시작 시점(`MissionIndex 1`, 미수락)부터 플레이어가 첫 NPC(레이븐)와 접촉하기 전까지 내비게이션이 안내를 시작하지 못하는 구조적 공백이었다. npc-mission-dialogue.md는 한때 이 공백을 신규 부트스트랩 미션(세션 시작부터 자동 수락된 `MissionId`)으로 메우려 했으나 폐기됐다 — 대신 이 절이 **내비게이션 쪽에서** 규칙으로 해소한다.

**신규 규칙**: 현재 미션이 **미수락**이면 `MissionType` 과 무관하게 그 미션의 담당 `NPCId` 로 안내한다. 현재 미션이 **수락됨**이면 §5-2 원안 그대로(`Dialogue`→`NPCId`, 그 외→`MatchTag`) 적용한다.

```
IsCurrentAccepted() == false:
    NPCId 키 공간 사용 (MissionType 무관)
IsCurrentAccepted() == true:
    §5-2 원안 그대로 (Dialogue → NPCId, 그 외 → MatchTag)
```

**왜 미수락 상태에 `MissionType` 을 더 이상 게이트로 쓰지 않는가**: 미수락 상태에서 플레이어가 찾아가야 할 곳은 항상 "그 미션을 **제시하는** NPC" 하나뿐이다 — `Gameplay` 미션도 예외가 아니다. 처치·레벨·콤보·넘기·벽타기·그래플 6종 전부 담당 NPC 의 Offer 카드로만 시작되므로(npc-mission-dialogue.md §2), 미수락 구간의 목적지는 언제나 "그 미션을 Offer 하는 NPC" 다. `MatchTag` 레지스트리(§5-4의 Gameplay/Interact 키 공간)는 **수락 후 실제 수행 위치**(예: 파쿠르 레인 안쪽 마커)를 가리키므로 미수락 상태에는 애초에 맞지 않는 목적지다 — 미수락 상태에서 `MatchTag` 로 조회하면 아직 시작하지도 않은 미션의 "수행 위치"로 플레이어를 보내는 오류가 된다.

**`NoNPCId`(9999) 처리 — 현재 데이터에서는 도달 불가, 방어적으로만 남긴다**: 이론상 `Entry->NPCId == NoNPCId` 인 미수락 미션은 대상 없음으로 조용히 처리한다(§5-6 재시도 없이 즉시 포기, cpp-style §14-1 선택적 관계 원칙의 "로우 없음 → 관계 없음" 취급과 동일한 처리). 그러나 **이 분기는 라이브 12행 데이터에서 도달하지 않는다** — 아래 §8 조건이 12행 전부(`Gameplay`·`Dialogue` 구분 없이) `NPCId` 값을 요구하도록 강화됐기 때문이다(§5-2 이전에는 `Dialogue` 6종만 `NPCId` 가 구조적으로 필요했고 `Gameplay` 6종은 "정황상 채워져 있을 가능성"에 불과했다 — §5-2 원문 참조). 도달 불가한 분기를 근거 없이 설계하지 않는다는 npc-mission-dialogue.md §3-3의 "도달 가능성" 원칙과 같은 이유로, 이 분기는 방어 코드로만 존재하고 관측 대상이 아니다.

### 5-3. 왜 Gameplay 구역 미션(7/9/11)은 담당 NPC 의 위치를 재사용할 수 없는가 — 유도 가치 논증

Vault(7)/Climb(9)/GrappleHook(11) 를 Offer 하는 NPC(스패로우/울프/폭스)는 npc-mission-dialogue.md §4-5 가 각각 "파쿠르 레인 초입"·"파쿠르 레인 안쪽"·"그래플 타워 초입"에 배치하도록 이미 설계했다 — 즉 그 NPC 의 위치가 §5-1 원칙 2항이 원래 원했던 "구역 진입점"과 실무적으로 겹칠 수 있다. 그럼에도 **그 NPC 의 위치를 그대로 재사용하지 않는다.** 이유는 데이터 유무가 아니라 **유도 가치가 0 이기 때문**이다 — 플레이어는 Vault 미션을 Offer 카드로 수락하는 그 순간 이미 스패로우 바로 앞에 서 있다. 목표가 스패로우의 좌표라면 수락 즉시 `RemainingPathLength` 가 §4 의 300cm 임계 미만이라 **라인이 뜨자마자(또는 아예) 사라진다** — 정작 플레이어가 실제로 찾아가야 하는 곳(파쿠르 레인 안쪽의 Vault 오브젝트 군집)까지는 아무 안내도 없다. 이 기능이 Gameplay 구역 미션에 존재하는 이유 자체가 무의미해진다.

**대응**: 신규 마커 액터 `ASpyMissionTargetPoint`(§5-4)를 NPC 위치보다 구역 안쪽(실제 Vault/Climb 오브젝트 군집·그래플 타워 앵커 밀집 지역에 가까운 지점)에 별도로 배치한다. 이 마커도 §5-1 원칙 2항이 이미 정한 "구역 진입점이지 특정 오브젝트가 아니다"라는 성질은 그대로 유지한다 — mission-system.md §1-2 결론대로 Vault/Climb 는 임의 지오메트리에 반복 반응하고 GrappleHook 앵커는 19개 중 아무거나 유효하므로(§1-2-1), 마커는 "이 방향으로 더 들어가라"는 안내일 뿐 "이 오브젝트가 정답"이 아니다. §4 의 300cm 임계 이내로 접근하면 라인이 사라지고 그 뒤는 플레이어가 자유롭게 찾는다 — 이 성질은 이번 개정으로 바뀌지 않는다.

**(2026-08-05 추가)** 같은 마커 클래스와 같은 배치 원칙(구역/전투구역 진입점, 특정 오브젝트 아님)으로 Kill(1)/Level(3)/Combo(5) 에도 전투 구역(봇 스폰 지점 인근) 진입점 마커가 사용자 결정으로 추가됐다(§5-1 2항, §8 조건 1·2). 이 3종은 원래 "장소 무관"이라 이 절의 "유도 가치 0" 논증(NPC 위치가 틀린 목적지가 되는 이유) 대상은 아니었지만, 마커 좌표 원칙 자체는 6종 전부 동일하게 적용된다.

**`Interact` 타입도 같은 이유로 NPCId 브랜치를 타면 안 된다** — `ASpyInteractableObject`(interactable-object-mission.md 도메인, `ASpyNPCCharacter` 와 무관한 별개 클래스)는 `NPCId` 자체가 없다(§1 실측). Interact 미션이 오브젝트를 대상으로 한다면 NPCId 조회는 애초에 성립하지 않는다 — §5-4 가 `Interact` 를 Dialogue 가 아니라 **`MatchTag` 조회 분기**(Gameplay 와 같은 경로)로 보낸다.

### 5-3-1. 이 논증은 Dialogue 분기(§5-1 원칙 1)에는 구조적으로 적용되지 않는다 — 단, 예외 하나는 남는다 (design-reviewer 1차 MAJOR 대응)

**왜 적용되지 않는가 — 범주가 다르다.** §5-3 이 Gameplay 구역 미션의 NPC-위치 재사용을 기각한 이유는 "라인이 짧다"가 아니라 **그 위치가 애초에 틀린 목적지였기 때문**이다(Vault 미션의 진짜 목표는 "레인 안쪽 오브젝트 군집"이지 "스패로우가 서 있는 자리"가 아니다) — 그래서 구조적 해법(신규 마커, 다른 데이터 소스)이 필요했다. Dialogue 미션은 다르다: "그 NPC 에게 가서 보고하라"가 미션의 정의 그 자체이므로, **NPC 의 좌표는 언제나 정확한 목적지다.** 수락 시점에 우연히 그 NPC 와 가까웠다고 해도 그건 "틀린 곳을 가리킨다"가 아니라 "이미 도착에 가깝다"는 뜻이고, §4 의 도착 임계값(300cm)은 정확히 이런 경우—더 안내할 필요가 없을 만큼 가깝다—를 위해 설계된 장치다(§4-1 B, 이미 §4 에서 확정). 즉 Dialogue 에는 "재사용할 수 없는 틀린 목표"라는 §5-3 의 전제 자체가 성립하지 않고, 대체할 다른 목적지도 없다(그 NPC 외에 "보고할 곳"은 없다) — 신규 마커로 해결할 문제가 아니다.

**단, 남는 예외 — "정확한 목표인데 라인이 그 홉(hop)에서 한 번도 안 보일 수 있다."** 목적지가 틀리진 않지만, 자동 수락 순간 이미 300cm 이내라면 그 미션 구간 동안 라인이 아예 표시되지 않는다(§4-3 콜드 스타트 조항도 이 경우를 구제하지 못한다 — 그 조항은 시작 거리가 400cm 이상일 때의 오프셋 처리를 다루지, 300cm 미만 시작 자체는 규칙1이 의도대로 "숨김"을 내린 정상 결과다). 이는 버그가 아니라 §4 의 설계 의도(가까우면 안내 불필요)가 그대로 실현된 결과지만, **그 근접이 우연이 아니라 npc-mission-dialogue.md 자체의 NPC 배치 설계에서 구조적으로 유도되는 쌍**이 있다면 "이 미션만 유독 라인이 안 뜬다"는 체감 불일치가 생길 수 있다 — 아래 두 쌍을 검토한다(레벨 실측 좌표는 확인하지 않았다 — 배치 **서술**에서 도출한 구조적 개연성이며, 실제 300cm 이내 여부는 §9 M-nav-6 로 관측한다).

| 쌍 | 확신도 | 근거 |
|---|---|---|
| **울프(`NPCId 5`, 미션10) — Climb(미션9)** | **높음** | npc-mission-dialogue.md §4-5 가 울프를 "파쿠르 레인 안쪽, **Climb 오브젝트 근처**"로 명시 배치했고, Climb GA 는 그 오브젝트 위치에서 활성화된다(mission-system.md §1-2 라인트레이스 판정) — NPC 배치 서술 자체가 두 지점의 근접을 의도하고 있다 |
| 바이퍼(`NPCId 3`, 미션6) — 콤보(미션5) | **낮음** | 콤보는 고정 완료 지점이 없다(mission-system.md §1-3, 대상 불요·전투 구역 어디서나 연결 가능) — 바이퍼 인근에서 끝난다는 구조적 보장이 없다. 스패로우(Vault, "초입")·폭스(GrappleHook, "타워 초입")도 각자의 구역이 넓고 다중 오브젝트에 흩어져 있어 유사한 구조적 결합이 없다 |

**결정: 마커 같은 구조적 해법을 추가하지 않는다(위 표가 이미 그 이유다 — 목적지가 틀리지 않았다), 대신 확신도가 높은 울프-미션10 쌍을 §9 M-nav-6 관측 대상으로 등록한다.** 관측 결과 실제로 위화감이 있으면 그때 조정한다(예: 이 특정 미션에 한해 도착 임계값을 낮추는 등 — 지금 선제적으로 설계하지 않는다, YAGNI).

### 5-4. 레지스트리 아키텍처

**핵심 결정 — 두 개의 서로 다른 키 공간을 쓴다. 하나로 합치지 않는다.**

| 분기 | 키 | 이유 |
|---|---|---|
| `MissionType == Dialogue` | `NPCId`(`int32`) | Dialogue 6행의 `MatchTag` 는 전부 공용 `Event.Mission.Report`(중복) 라 구분자가 못 된다 — 반면 `NPCId` 는 행마다 고유하다 |
| `MissionType == Gameplay`(구역형) 또는 `Interact` | `MatchTag`(`FGameplayTag`) | Gameplay 6종은 전부 leaf 태그로 고유하다(mission-system.md §6-1). `ASpyInteractableObject` 가 이미 갖고 있는 `MissionEventTag` 를 그대로 재사용할 수 있어(§1 실측) **신규 필드 없이** Interact 확장이 가능하다. `MissionId` 정수 대신 태그를 키로 쓰면, 이 프로젝트가 이미 두 번 겪은 MissionId 재번호 이력(0-based→1-based 등, `.claude/.active-sessions.md` 세션 421~433)에도 마커가 흔들리지 않는다 |

**구성 요소 3개 (신규)**:

1. **`USpyMissionTargetRegistrySubsystem : public UWorldSubsystem`** — 순수 로컬 조회 서비스. API 형태(정확한 시그니처는 gameplay-programmer 판단):
   - `RegisterNPCLocation(int32 NPCId, AActor* InActor)` / `UnregisterNPCLocation(...)` / `FindNPCLocation(int32 NPCId, FVector& OutLocation) const`
   - `RegisterMissionTargetLocation(FGameplayTag InTag, AActor* InActor)` / `UnregisterMissionTargetLocation(...)` / `FindMissionTargetLocation(FGameplayTag InTag, FVector& OutLocation) const`
   - **액터 자체(약참조)를 저장하고, 조회 시점에 `GetActorLocation()` 을 읽는다 — `BeginPlay` 시점의 좌표를 스냅샷으로 캡처해 저장하지 않는다.** 저장 형태가 액터 참조이므로 에디터에서 액터를 재배치하면 다음 플레이 세션에 자동 반영된다(§10 참고). 복잡도는 스냅샷 캐싱과 동일하지만, 나중에 틱 단위 실시간 추적이 필요해져도 조회 지점만 바꾸면 된다 — 스냅샷 방식은 이 확장 경로 자체가 막힌다.
   - **서버 레플리케이션 불필요** — 호스트든 원격 클라이언트든 같은 레벨 에셋을 로드하므로 액터 배치가 이미 동일하다. 각 프로세스(서버 포함)가 로컬로 자기 월드의 액터만 자기등록하면 그 자체로 모든 프로세스의 조회 결과가 일치한다. `USpyNavigationComponent` 는 로컬 컨트롤 클라이언트에서만 조회하므로(§1) 서버가 실제로 조회하는 일은 없지만, 등록 자체를 서버에서 막을 이유도 없다 — 조건 분기를 추가하지 않는다(YAGNI).
   - `TActorIterator`/`GetAllActorsOfClass` 류의 매 프레임 탐색이 아니다 — 각 액터가 **자기 `BeginPlay` 에서 1회** 자기등록하는 패턴이라 cpp-style §8 "런타임 액터 탐색 금지"가 권장하는 대안("참조는 스폰/등록 시점에 넘겨받는다") 그대로다.

2. **`ASpyMissionTargetPoint : public AActor`(신규)** — Gameplay 미션 6종(Vault/Climb/GrappleHook/Kill/Level/Combo) 전용 경량 마커(2026-08-05 확장 — 최초 3개에서 Kill/Level/Combo 3개 추가).
   - `UPROPERTY(EditAnywhere) FGameplayTag TargetMissionTag;` — 해당 미션의 `MatchTag` 와 정확히 일치시켜 배치(§8).
   - `BeginPlay`: `RegisterMissionTargetLocation(TargetMissionTag, this)`. `EndPlay`: `UnregisterMissionTargetLocation(...)`.
   - 에디터 가시성을 위해 `UBillboardComponent`(에디터 전용) 부착을 권장하되 필수는 아니다(런타임 렌더링 없음, 순수 위치 마커).

3. **기존 클래스 확장 (신규 필드 없음)**:
   - `ASpyNPCCharacter::BeginPlay()` — 기존 `CacheNPCData()` 호출 근처에 `RegisterNPCLocation(NPCId, this)` 추가. **`EndPlay` 오버라이드가 현재 없다 — 신규로 추가해 `UnregisterNPCLocation(...)` 호출.**
   - `ASpyInteractableObject::BeginPlay()` — `RegisterMissionTargetLocation(MissionEventTag, this)` 추가(기존 `MissionEventTag` 필드 재사용, §5-3). `EndPlay` 도 동일 패턴으로 해제(현재 `EndPlay` 오버라이드 유무는 gameplay-programmer 구현 시 확인).

### 5-5. `USpyNavigationComponent` 목표 조회 로직 개정 (2026-08-05: 미수락 게이트 통합, §5-8도 참조)

기존(DataTable) 로직 `BoundMissionComponent->GetMissionTargetLocation(MissionIndex)` 한 줄을 아래 흐름으로 교체한다. **이 흐름은 §5-8이 정하는 단일 재계산 함수(`RecalculateTarget` 류) 안에서 실행되며, 더 이상 `HandleMissionAccepted` 하나만의 책임이 아니다** — 호출 시점이 "수락 이벤트"에서 "미션 진행 상태가 바뀔 때마다"로 넓어졌기 때문이다(§5-8):

```
Entry = BoundMissionComponent->GetMissionEntry(MissionIndex)   //# 기존 공개 API, 신규 아님
Entry 없음 → StopPath(), 종료

UseNPCIdBranch = (BoundMissionComponent->IsCurrentAccepted() == false) OR (Entry->MissionType == Dialogue)
    //# 미수락이면 MissionType 무관 NPCId 우선(§5-2-1), 수락됐으면 §5-2 원안 그대로

UseNPCIdBranch:
    Entry->NPCId != NoNPCId 이면 Registry->FindNPCLocation(Entry->NPCId, Target) 시도
그 외(수락된 Gameplay·Interact만 해당):
    Entry->MatchTag 유효하면 Registry->FindMissionTargetLocation(Entry->MatchTag, Target) 시도

조회 성공 → StartPathTo(Target)
조회 실패 → §5-6 재시도로 이동
```

**수락된 Kill/Level/Combo(1/3/5)도 2026-08-05부터 `ASpyMissionTargetPoint` 가 해당 `MatchTag`(`Event.Mission.Kill`/`Level`/`Combo`)로 등록돼 있어 조회가 성공한다 — Vault/Climb/GrappleHook(7/9/11)과 완전히 동일한 경로다.** 별도의 예외 목록(하드코딩된 MissionId 제외 집합)을 두지 않는다 — 매직 넘버(cpp-style §15)이기도 하고, Gameplay 6종은 §5-1 2항(2026-08-05 재개정)이 정한 대로 전부 동일하게 `MatchTag` 조회를 거치기 때문이다. **미수락 상태의 Kill/Level/Combo(1/3/5)는 여전히 `UseNPCIdBranch`가 참이 되어 담당 NPC로 안내된다(§5-2-1)** — "미수락이면 항상 담당 NPC, 수락되면 전용 마커"라는 흐름은 Gameplay 6종 전부 동일하다.

### 5-6. 타이밍 리스크 — 재시도가 필요한가

**결론: 필요하다 — 이 프로젝트의 실제 위험도는 낮지만(레벨이 스트리밍 없이 상시 로드돼 모든 액터의 `BeginPlay` 가 플레이어 입력 가능 이전에 끝난다), `SpyNavigationComponent` 자신이 이미 정확히 같은 문제(컨트롤러/PlayerState 아직 없음)를 `BindRetryTimerHandle` + 0.2초 반복 타이머로 방어하고 있다 — 같은 클래스 안에 정합 없는 두 가지 방식(하나는 방어, 하나는 무방비)을 둘 이유가 없다.**

- 조회 실패 시 **새 타이머**(`BindRetryTimerHandle` 과는 별개 — 하나는 BeginPlay 시점 컴포넌트 바인딩용, 하나는 목표 좌표 조회용으로 발생 시점이 다르다)로 0.2초 간격 최대 약 10회(≈2초) 재시도한다.
- 2초 안에 계속 실패하면: **`NPCId` 키 공간으로 조회한 경우(§5-2-1의 미수락 분기 + §5-2의 수락된 `Dialogue` 분기, 둘 다 §5-5의 `UseNPCIdBranch`) 경고 로그 1회**(`bWarnedMissingConfig` 와 동일 패턴) — NPC 는 항상 존재해야 하므로 이 실패는 데이터 버그(NPC 미배치·`NPCId` 오기)를 뜻한다. **`MatchTag` 키 공간으로 조회한 경우(수락된 Gameplay/Interact) 로그 없이 조용히 포기** — 2026-08-05 Kill/Level/Combo 마커 배치로 Gameplay 6종(Vault/Climb/GrappleHook/Kill/Level/Combo) 전부가 마커를 가지므로, 지금은 이 조회 실패가 사실상 항상 마커 배치 누락(§8 조건 1)을 뜻한다. 그래도 경고 로그를 신설하지 않는 이유는 향후 `Interact` 로우가 추가될 때(현재 0행, §5-1 4항)까지 이 조회 지점을 그대로 재사용하기 위함이다 — 실패 감지는 §8 데이터 조건 체크리스트 + §9 M-nav 플레이테스트로 잡는다. **경고 유무를 가르는 기준이 §5-2 원안(`MissionType` 기준)에서 §5-5 의 `UseNPCIdBranch`(키 공간 기준)로 바뀐 것**뿐 — 미수락 상태의 Kill/Level/Combo 도 이제 `NPCId` 로 조회되므로 실패 시 경고가 뜬다(§5-2-1이 만든 새 경로).
- 새 목표 조회를 시작할 때는 그 전에 대기 중이던 재시도 타이머를 먼저 clear 한다(§5-8이 정하는 재계산 함수의 첫 동작) — 낡은 재시도가 새로 바뀐 미션 상태에 잘못된 좌표를 주입하는 경합을 막는다. `StopPath()` 도 동일하게 이 타이머를 clear 한다(완전 정지 시).

### 5-7. 테스트 가능성 — World 의존성 회귀 방지 (신규 요구사항)

**⚠ 이 항목을 §7-6 으로 다시 명시한다 — 여기서만 언급하고 넘어가면 구현 시 조용히 누락될 위험이 크다.**

`SpyNavigationComponentTests.cpp` 의 기존 상태 머신 테스트(수락→경로 활성, 완료→비활성)는 `NewObject<AActor>(GetTransientPackage())` 로 World 없이 액터를 만든다. 기존 `HandleMissionAccepted` 는 `BoundMissionComponent`(DataAsset 기반) 만 읽어 World 가 필요 없었지만, 개정 후에는 `GetWorld()->GetSubsystem<USpyMissionTargetRegistrySubsystem>()` 를 거쳐야 하므로 **이 픽스처들이 전부 깨진다**(World 없음 → Subsystem 없음 → 조회 불가). `BindMissionComponent()` 가 Controller/PlayerState 체인을 우회해 테스트가 `USpyMissionComponent` 를 직접 주입하게 해주는 것과 **같은 목적의 주입 지점**이 좌표 조회 단계에도 있어야 한다 — 정확한 형태(오버라이드 가능한 protected 메서드, 함수 델리게이트 주입 등)는 gameplay-programmer 판단이지만, 이 장치 없이는 기존 테스트 커버리지 전체가 실 World 없이는 회귀 불가능해진다는 사실만은 이 문서가 못박는다.

### 5-8. 트리거 메커니즘 개정 — 델리게이트 3개를 1개로 통합 (2026-08-05, design-reviewer가 특히 볼 판단)

**문제**: §5-2-1이 "미수락 상태에서도 항상 목표가 있다(담당 NPC)"는 새 전제를 추가하면서, 기존 트리거 모델(`OnMissionAccepted`가 경로 시작을 트리거, `OnMissionCompleted`가 **무조건** `StopPath()` 를 호출)이 이 전제와 충돌할 위험이 생겼다. `Dialogue` 미션 완료(예: 레이븐 보고) 직후 인덱스가 다음 `Gameplay` 미션(미수락)으로 넘어가는 전이 6건(`MissionId` 2/4/6/8/10/12, 재번호 없음, §3-4)마다 매번 이 상호작용이 발생한다:

1. `OnMissionCompleted` 발화 → `HandleMissionCompleted` → 무조건 `StopPath()`
2. **같은 `OnRep`/서버 호출 안에서** `OnMissionProgressChanged` 도 무조건 발화(코디네이터 코드 확인 — `ProcessProgress` 217줄·`AcceptCurrentMission` 238줄·`OnRep_MissionState` 114줄, 조건 없이 매번)한다. §5-2-1이 이 델리게이트도 구독하게 만들면, 방금 `StopPath()` 로 끈 경로를 곧바로 "다음 미션은 미수락 → 담당 NPC로" 재계산해 다시 켠다.

정지와 재시작이 같은 동기 호출 체인 안에서 연달아 일어난다 — 눈에 보이는 깜빡임은 아니지만(렌더링은 다음 Tick), `StopPath()` 가 내부 상태(가시성 플래그·스플라인·재시도 타이머)를 정지 형태로 리셋했다가 곧바로 재계산이 그걸 다시 세우는 **불필요한 이중 작업**이고, §4-3 콜드 스타트 조항이 "이전 프레임에 보이고 있었다"로 시드하는 로직과 맞물리면 매 완료 전이마다 그 시드 경로를 다시 타게 돼 로직이 산개된다.

**대안 비교**:

| 안 | 설명 | trade-off |
|---|---|---|
| **A. 현행 유지 + `OnMissionProgressChanged` 병행 구독** | `HandleMissionAccepted`/`HandleMissionCompleted`(무조건 `StopPath()`) 를 그대로 두고, `OnMissionProgressChanged` 구독을 추가해 "미수락 상태 재타겟팅"만 새로 처리 | 3개 델리게이트 + 신규 1개로 로직이 4갈래로 흩어진다. 매 완료 전이마다 위에서 서술한 정지→재시작 이중 작업이 항상 발생 |
| **B. 단일 재계산 함수로 통합, `OnMissionProgressChanged` 하나만 구독 (권장)** | `HandleMissionAccepted`/`HandleMissionCompleted`/`OnAllMissionsCompleted` 구독을 제거하고, `OnMissionProgressChanged` 하나가 유일한 트리거가 된다. 핸들러(`RecalculateTarget` 류)는 매번 §5-5의 통합 로직을 처음부터 다시 실행 — "지금 `MissionIndex`/`IsCurrentAccepted()` 가 무엇인가"만 보고 목표를 다시 계산한다. `Entry` 가 없으면(전체 완료) §5-5가 이미 `StopPath()` 로 귀결되므로 별도 처리 불필요 | 정지 없이 이전 목표에서 다음 목표로 직접 전환 — 이중 작업 없음. 로직이 함수 하나에 모여 §5-5 하나만 보면 전체 동작을 알 수 있다. 단, 매 `Count` 변화(진행도 tick)에도 이 델리게이트가 발화하므로 **불필요한 재계산을 걸러야 한다**(아래 가드) |
| C. `HandleMissionCompleted` 조건부화 | `StopPath()` 를 무조건 호출하지 않고, "다음 인덱스가 있으면 그 인덱스로 즉시 재계산, 없으면 `StopPath()`" 로 바꿔 `HandleMissionCompleted` 안에 §5-5 로직을 인라인 | 결과적으로 안 B와 동일한 로직을 `OnMissionCompleted`/`OnMissionAccepted`/`OnMissionProgressChanged` 세 곳에 나눠 중복 구현하는 것과 같다 — 세 델리게이트의 발화 순서·중복 호출 가능성을 계속 신경 써야 하는 안 A의 산개 문제가 그대로 남는다 |

**결정: B.** 근거:

1. **순서 보장이 이미 성립해 있다** — §2-3이 이미 `OnRep_MissionState` 안에서 "완료 판정 → 수락 판정 → 기존 두 처리(`OnMissionProgressChanged`/`OnAllMissionsCompleted`)" 순서를 확정했다. 즉 `OnMissionProgressChanged` 가 발화하는 시점에는 **완료·수락 상태 전이가 이미 전부 `MissionState` 에 반영된 뒤**다 — 재계산 함수가 그 최종 상태(`MissionIndex`+`IsCurrentAccepted()`) 만 읽으면 되므로, 매 이벤트 단위의 순서를 델리게이트 3개에 걸쳐 따로 추적할 필요가 없다(§2-3의 "요구되는 것은 매 완료 이벤트 단위의 정합이 아니라 최종 상태의 정합" 논증과 정확히 같은 구조).
2. `OnAllMissionsCompleted` 도 별도 처리가 필요 없다 — 전체 완료 시 `MissionIndex` 가 마지막 미션을 넘어서고, `GetMissionEntry(MissionIndex)` 가 `nullptr` 을 반환해 §5-5가 그 자리에서 `StopPath()` 로 귀결한다. `OnRep_MissionState` 는 `OnMissionProgressChanged` 를 항상 `OnAllMissionsCompleted` 보다 먼저(또는 함께) 발화하므로(§2-1 코드 스니펫) 이 델리게이트 없이도 놓치는 경우가 없다.
3. §2 가 확립한 클라이언트 안전성(§2-1~§2-3)은 `OnMissionProgressChanged` 자체에도 그대로 적용된다 — 이 델리게이트는 애초에 `ReplicatedUsing`(`OnRep_MissionState`) 안에서 무조건 발화해 왔으므로(§2-1), 세 델리게이트 중 **가장 먼저부터 안전했던** 신호다. 안 B 는 결과적으로 §2-3이 고생해서 안전하게 만든 두 델리게이트(`OnMissionAccepted`/`OnMissionCompleted`) 대신, 처음부터 안전했던 델리게이트 하나에 의존을 몰아준다.

**과잉 재계산 가드(안 B 채택에 따른 필수 조건)**: `OnMissionProgressChanged` 는 `Count` 만 바뀌어도(예: Vault 3/5 → 4/5) 발화한다. 재계산 함수가 그때마다 목표를 다시 조회하고 `StartPathTo()` 를 다시 부르면 낭비다(조회 자체는 가벼운 레지스트리 조회이지만, `StartPathTo` 가 내부 상태를 리셋하면 §4-3 가시성 히스테리시스가 매번 콜드 스타트를 다시 타는 부작용이 생긴다). **재계산 함수는 `(MissionIndex, bAccepted)` 튜플을 캐싱해 이전 호출과 동일하면 조회·`StartPathTo` 를 스킵한다** — `Count` 변화만으로는 목표가 절대 바뀌지 않으므로 이 가드로 완전히 걸러진다.

**바인드 시점 pull(필수, 새 요구 아님 — 기존 패턴 재사용)**: `BindMissionComponent()` 완료 직후, 재계산 함수를 **1회 명시 호출**한다 — 바인드 이전에 이미 어떤 상태였는지는 `OnMissionProgressChanged` 이벤트로 전달되지 않으므로(그 델리게이트는 향후 변화만 알려준다), 바인드 시점에 "지금 상태가 무엇인가"를 직접 읽어야 한다. `USpyMainHUD::RefreshMission()` 과 동일한 pull 패턴(§5-6 재시도 타이머 리스트와는 별개 목적)이다.

---

## §6 갱신 주기 0.75초 — 밸런스 판단

spec 이 제시한 기본값(`UpdateIntervalSeconds = 0.75f`)을 **승인**한다. 근거:

- `MaxWalkSpeed`(500, `SpyCharacterConfig.h:32`·`SpyCharacter.cpp:55` 코드 기본값 실측 — 라이브 `DataAsset` 인스턴스 값까지는 확인하지 않았다) 기준, 0.75초 동안 플레이어가 **걷기** 로 이동 가능한 최대 거리 = `500 × 0.75 = 375cm`. 스프린트 등 더 빠른 이동 수단이 있는지는 이 문서에서 확인하지 않았다 — 있다면 이 값보다 더 자주 §4-3 "한계(허용)" 절의 순간 전환이 발생할 수 있으나, 상태 판정 자체는 깨지지 않는다.
- **더 짧은 주기(예 0.5초)를 기각하는 이유**: `FindPathToLocationSynchronously` 는 **동기(synchronous) 호출**이라 게임 스레드를 블로킹한다. 호출 빈도를 높이면(0.5초 = 초당 2회) 패키지 빌드의 큰 NavMesh 에서 프레임 히치 위험이 커진다. 0.75초(초당 약 1.33회)는 plan Task 4 의 `TryBindMissionComponent` 계열 재시도 타이머(0.2초, 가벼운 바인딩 재시도)보다 확연히 느린 주기다 — 이쪽은 무거운 NavMesh 질의라 더 보수적인 주기가 맞다.
- **더 긴 주기(예 1.5초)를 기각하는 이유**: 1.5초 동안 최대 이동 거리 750cm — 코너를 돌아 시야 밖 경로로 이동할 때 라인이 눈에 띄게 "뒤처져" 보이는 구간이 길어진다.

**결정 메트릭(M-nav-1, §9)**: 파쿠르 레인처럼 코너가 많은 구간에서 0.75초 지연이 "라인이 실제 이동 경로와 눈에 띄게 어긋난다"는 인상을 주는지 관측한다. 어긋남이 관측되면 0.5초로 낮추되, 그 경우 히치 유무를 함께 확인한다(YAGNI: 적응형/가변 주기는 도입하지 않는다 — 단일 상수로 충분).

---

## §7 구현 요청사항 (gameplay-programmer 용)

### 7-1. Gameplay Tag

**신규 태그 없음.** 이 문서는 태그를 추가하지 않는다.

### 7-2. C++ 인터페이스

**신규 인터페이스 없음.** plan Global Constraints 가 확정한 대로 `USpyNavigationComponent` → `USpyMissionComponent` 직접 참조를 유지한다. 단 §2-3 이 요구하는 `OnRep_MissionState` 시그니처 확장(이전 값 파라미터)은 인터페이스가 아니라 기존 함수의 개정이다 — §7-5 참조.

### 7-3. DataAsset 스키마

**§0 개정 이후: DataAsset 신규 필드 없음.** 좌표는 더 이상 DataAsset/DataTable 이 아니라 레벨 액터에서 온다(§5). 제거 대상 스키마는 §7-6 참조.

### 7-4. GA · GE 명세

**신규 GA·GE 없음.**

### 7-5. ⛔ plan 개정 필요 항목 — 구현 완료 확인 (역사적 기록으로 보존)

**아래 6항목은 전부 구현 완료가 코드로 확인됐다**(`SpyNavigationComponent.h/.cpp` 실측 — `StartOffsetDistanceCm`/`ArrivalHideDistanceCm`/`ArrivalReshowDistanceCm`/`GroundZOffsetCm` 필드, `HideVisual()`, `SpyNavPathMath::EvaluateHysteresisVisibility`/`ComputePathLength`/`TrimLeadingDistance` 전부 실재). 더 이상 "개정 필요"가 아니지만, §2·§4 결정의 근거 기록으로 표는 그대로 남긴다. **신규 작업 항목은 §7-6.**

hud-mana-compass-skillbar.md §7-0 과 동일하게, 아래는 task 구동 실행에서 매핑이 없으면 조용히 드롭될 수 있는 항목이었다 — **구현 착수 전 plan 개정(Task 2·Task 4·Task 5 확장)이 선행돼야 했다.**

| # | 항목 | 대상 Task | 성격 | 미개정 시 결과 |
|---|---|---|---|---|
| 1 | `OnRep_MissionState` 확장 — 이전 값 파라미터 추가 + `OnMissionAccepted`/`OnMissionCompleted` 를 클라이언트에서도 상태-diff 로 판정해 브로드캐스트(§2-3 규칙 그대로) | plan Task 2 | **코드** | 데디케이티드 서버/원격 클라이언트에서 `USpyNavigationComponent` 영구 미작동(§2, 단일 실패점) — 1인 PIE 에서는 증상 없음 |
| 2 | `USpyNavigationComponent` 에 `StartOffsetDistanceCm`(100.f)·`ArrivalHideDistanceCm`(300.f)·`ArrivalReshowDistanceCm`(400.f)·`GroundZOffsetCm`(3.f) 4개 `EditDefaultsOnly float` 필드 추가 | plan Task 5 | 코드 | §4 합성 규칙을 표현할 자리가 없어 발밑 분리 조각·뚝 끊김 현상이 그대로 남는다 |
| 3 | 가시성 상태 판정 로직(§4-3 상태 머신) — `RecomputePath`/`ApplyPathPoints` 확장. **입력→출력이 순수 함수로 분리 가능**(경로점 배열 + 누적 길이 + 3개 임계값(`StartOffsetDistanceCm`/`ArrivalHideDistanceCm`/`ArrivalReshowDistanceCm`) → 트리밍된 경로점 배열 + 가시성 bool — `GroundZOffsetCm` 는 이 판정과 무관하게 항목 4 에서 별도로 적용한다)하므로, Task 3 의 `SpyNavPathMath` 모듈(순수 함수 전용, 테스트 가능)에 이 판정을 추가하는 것이 spec §7 의 "테스트 가능 범위" 원칙과 정확히 들어맞는다. 정확한 함수명·시그니처는 gameplay-programmer 판단 | plan Task 3 확장 | 코드 | 렌더 경로에 직접 박히면 §7 이 요구하는 자동화 테스트 커버리지(회귀 고정)를 못 만든다 |
| 4 | `GroundZOffsetCm`(3.f) 을 `ApplyPathPoints` 에서 각 경로점 Z 에 더해 지면 z-fighting 방지 | plan Task 5 | 코드 | 바닥 데칼/지형과 스플라인 메시가 겹쳐 플리커링 발생 가능 |
| 5 | `NavPathGlow` 머티리얼(아트, 범위 밖) 제작 시 §3 톤 가이드(색상 `rgba(0.373, 0.816, 0.851, 1.0)`, 전체 폭 10cm, 플레이어→목표 방향 UV 패닝 + 저주기 펄스) 전달 | 아트 작업(범위 밖) | 에셋 | 엔진 기본 머티리얼로 표시돼(plan Task 5 Step 5 주석) 톤이 이 프로젝트 정체성과 무관해진다 |
| 6 | `RecomputePath` 가 `NavPath == nullptr \|\| NavPath->IsValid() == false` 로 조기 반환할 때, 스플라인·세그먼트 풀을 그대로 두지 않고 **숨긴다**(§4-3 의 "숨김" 상태로 전이) | plan Task 5 | 코드 | plan 원안(Task 5 Step 5 `RecomputePath`)은 조기 반환 시 이전에 그려진 라인을 아무 처리 없이 그대로 둔다 — `TargetLocation` 이 NavMesh 밖(예: 벽 안쪽 10cm)이면 **경로 질의가 매번 실패하고 라인은 마지막으로 성공했던 낡은 경로를 가리킨 채 영구히 멈춘다.** §2 와 같은 계열의 무증상 실패이며, §8 조건 3·4(NPC 좌표 정확성·재배치 갱신)가 어긋났을 때 실제로 트리거되는 경로다 |

- 항목 1 은 **이 표에서 가장 중요했다** — 나머지 전부가 이 수정 없이는 실물 배포에서 관측조차 안 된다(§2).
- 항목 6 은 §8(구판) 조건 3·4 가 완벽히 지켜지지 않았을 때의 **완충 장치**였다 — 좌표가 틀려도 최소한 "라인이 이상한 곳에 멈춰 있다"가 아니라 "라인이 없다"로 실패하게 만든다. **§0 개정 이후에도 이 장치 자체는 그대로 유효하다** — `TargetLocation` 이 NavMesh 밖이면(마커 액터를 벽 안쪽에 배치하는 등) 여전히 같은 방식으로 실패해야 한다.

### 7-6. 신규 구현 요청사항 — 레벨 배치 액터 자동 추적 (§0/§5 개정)

**신규 클래스**:

| 클래스 | 위치(참고) | 핵심 API/필드 |
|---|---|---|
| `USpyMissionTargetRegistrySubsystem : public UWorldSubsystem` | 신규(폴더는 기존 Mission/Navigation 인프라가 있는 `System/` 과 일관되게 gameplay-programmer 판단) | `Register/UnregisterNPCLocation(int32, AActor*)`, `FindNPCLocation(int32, FVector&) const`, `Register/UnregisterMissionTargetLocation(FGameplayTag, AActor*)`, `FindMissionTargetLocation(FGameplayTag, FVector&) const` — 약참조 저장, 조회 시점에 `GetActorLocation()` (§5-4) |
| `ASpyMissionTargetPoint : public AActor`(신규) | `NPC/` 또는 신규 `Navigation/`(gameplay-programmer 판단) | `UPROPERTY(EditAnywhere) FGameplayTag TargetMissionTag;` — `BeginPlay`/`EndPlay` 에서 자기등록/해제(§5-4) |

**기존 클래스 확장(신규 필드 없음, §5-4)**:
- `ASpyNPCCharacter.h/.cpp` — `BeginPlay` 에 `RegisterNPCLocation(NPCId, this)` 추가. **`EndPlay` 오버라이드 신규 추가** 필요(`UnregisterNPCLocation(...)`) — 현재 이 클래스에 `EndPlay` 오버라이드가 없음(코드 실측).
- `ASpyInteractableObject.h/.cpp` — `BeginPlay` 에 `RegisterMissionTargetLocation(MissionEventTag, this)` 추가. **`EndPlay` 오버라이드가 현재 없다**(코드 실측, `ASpyNPCCharacter` 와 동일 상황) — 신규 추가해 `UnregisterMissionTargetLocation(...)` 호출.
- **소진(consume) 처리와의 상호작용(1문장, 설계는 범위 밖)**: `ASpyInteractableObject` 는 상호작용 시 `bConsumed = true` + 콜리전 비활성화로 스스로 소진된다(`SpyInteractableObject.h:72-73`) — 소진된 오브젝트가 레지스트리에 계속 등록돼 있으면, 같은 `MatchTag` 를 쓰는 **미래의** 다른 미션이 죽은 오브젝트를 가리키게 될 수 있다. 현재 12행 데이터에 `Interact` 행이 없어 오늘 당장 깨지는 경로는 아니다 — 소진 시점에 `UnregisterMissionTargetLocation` 을 함께 호출할지는 `Interact` 미션이 실제로 추가될 때 별도 결정한다(이번 범위에서 설계하지 않는다).
- `SpyNavigationComponent.h/.cpp` — 조회 로직 전면 교체(§5-5), 신규 재시도 타이머 필드(`BindRetryTimerHandle` 과 별개, §5-6), 새 목표 조회 시작 시 그 타이머 clear 추가, **테스트 주입 지점 추가(§5-7, 필수)**.

**§5-8 개정 — 트리거 델리게이트 통합(2026-08-05, 신규)**:
- `BindMissionComponent()` 의 델리게이트 구독을 `OnMissionAccepted`/`OnMissionCompleted`/`OnAllMissionsCompleted` 3개에서 **`OnMissionProgressChanged` 1개**로 교체한다.
- `HandleMissionAccepted`/`HandleMissionCompleted` 두 함수를 단일 재계산 함수(가칭 `RecalculateTarget`, 정확한 이름은 gameplay-programmer 판단)로 통합한다 — §5-5의 통합 조회 로직을 그 안에서 실행한다.
- **과잉 재계산 가드**: 재계산 함수 진입 시 `(MissionIndex, IsCurrentAccepted())` 튜플을 캐싱된 이전 값과 비교해, 동일하면 조회·`StartPathTo()` 를 스킵하고 즉시 반환한다(§5-8) — `Count` 변화만으로 발화하는 `OnMissionProgressChanged` 호출을 걸러내기 위한 필수 가드.
- **바인드 시점 pull(필수)**: `BindMissionComponent()` 완료 직후 재계산 함수를 1회 명시 호출한다(§5-8, `SpyMainHUD::RefreshMission()` 과 동일 패턴).
- 기존 `HandleMissionAccepted`/`HandleMissionCompleted` 함수·그 델리게이트 바인딩 코드는 제거 대상이다.

**Gameplay Tag**: 신규 태그 없음 — `TargetMissionTag`/`MissionEventTag` 는 기존 `FSpyMissionRow.MatchTag` 값(`Skill.Move.Vault` 등)을 그대로 담는 필드일 뿐 새 태그 정의가 아니다.

**제거(dead code, §0)**:
- `SpyMissionConfig.h/.cpp` — `FSpyMission_TargetLocationRow` 구조체, `MissionTargetLocationTable` 필드, `GetMissionTargetLocation(int32)` 전체 삭제.
- `SpyMissionComponent.h/.cpp` — `GetMissionTargetLocation(int32)` 패스스루 전체 삭제(`GetMissionEntry` 는 이미 있으므로 대체 불필요, §5-5).

**테스트 영향 범위(작성은 test-engineer 몫 — 여기서는 범위만 명시)**: 위 제거로 아래 파일의 관련 테스트가 전부 깨진다(약 15개 지점, `FSpyMission_TargetLocationRow`/`MissionTargetLocationTable`/`GetMissionTargetLocation` grep 기준) — `System/Tests/SpyMissionTests.cpp`, `System/Tests/SpyMissionComponentTests.cpp`, `ManagerComponent/Tests/SpyNavigationComponentTests.cpp`. §5-7 의 World 의존성 회귀도 `SpyNavigationComponentTests.cpp` 전체 재작성 없이는 해소되지 않는다.

**에셋 정리(코드 아님, 메인이 unreal-mcp 로 처리)**: `DT_Mission_TargetLocation` DataTable(9행, 실측 좌표로 이미 입력됨) 과 `DA_SpyMissionConfig.MissionTargetLocationTable` 연결은 코드에서 필드를 제거하면 참조가 저절로 끊긴다 — **`DT_Mission_TargetLocation` 에셋 자체의 삭제 여부는 이 문서가 권고한다: 삭제 권장.** 고아 상태로 남기면 향후 기여자가 "이 DataTable 은 왜 있지?"로 혼동할 위험이 실질 이득(보존 가치)보다 크다 — 원 스키마는 git 이력(`818cad0`)에 남아 있어 완전히 사라지지 않는다.

---

## §8 에디터 데이터 조건 (2026-08-05 개정)

| # | 조건 | 어기면 |
|---|---|---|
| 1 | `ASpyMissionTargetPoint`(또는 그 BP 서브클래스)를 **정확히 6개** 배치한다 — Gameplay 미션 6종(Vault/Climb/GrappleHook/Kill/Level/Combo) 마다 1개(§5-3 "구역 안쪽/구역 진입점, 특정 오브젝트가 아닌" 지점, §5-1 2항, 2026-08-05 개정 — 최초 3개에서 Kill/Level/Combo 3개 추가) | 마커를 빠뜨리면 §5-6 재시도가 조용히 실패해 그 미션 내내 라인이 안 뜬다(Dialogue 와 달리 경고 로그도 없다 — §5-6) |
| 2 | 각 `ASpyMissionTargetPoint.TargetMissionTag` 는 대응 미션의 `FSpyMissionRow.MatchTag` 와 **정확히 일치**(leaf 태그, 부모 태그 아님 — mission-system.md §6-1)해야 한다: Vault→`Skill.Move.Vault`, Climb→`Skill.Move.Climb`, GrappleHook→`Skill.Move.GrappleHook`, Kill→`Event.Mission.Kill`, Level→`Event.Mission.Level`, Combo→`Event.Mission.Combo`(2026-08-05 개정 — 뒤 3개 추가) | 태그 오기(부모 태그 지정, 오타)는 컴파일도 런타임 에러도 없이 조용히 조회가 실패한다 — §5-6 과 동일한 무증상 실패 |
| 3 | 마커 좌표는 **구역 진입점**이지 특정 오브젝트가 아니다(§5-3) — "정확한 오브젝트 하나"를 가리키려 하지 않는다 | 특정 오브젝트에 못 박으면, 그 오브젝트가 아닌 다른(똑같이 유효한) 오브젝트로 완료했을 때 라인이 계속 남아 있던 지점을 가리켜 혼란을 준다 |
| 4 | Dialogue 미션(2/4/6/8/10/12)의 목표 좌표는 **신규 데이터 저작이 필요 없다** — 기존 NPC 6종(레이븐~폭스)이 이미 정확한 위치에 배치돼 있고, 코드가 그 위치를 그대로 읽는다(§5-4). **레벨 디자이너가 할 일이 없다는 것이 확인 대상이다** — 데이터 조건이 아니라 검증 항목 |
| 5 | **§7-6 완료 후, NPC 를 재배치해도 좌표가 별도 갱신 없이 다음 플레이 세션에 자동 반영되는지 PIE 로 확인한다.** 구판(§8 구판 조건 4)이 "known limitation"으로 인정했던 문제의 해소 여부 검증 — §10 참고 | 자동 반영이 안 되면 §5-4 의 "액터 참조 저장, 조회 시점에 위치 읽기" 설계가 실제로는 스냅샷처럼 동작한다는 뜻 — 구현 재확인 필요 |
| 6 | §7-5 항목 1(`OnRep_MissionState` 개정)이 반영된 빌드로 **2인 이상 PIE**(호스트+원격 클라이언트)에서 검증한다 — 1인 PIE 만으로는 §2 결함이 통과된다(§0 개정과 무관하게 계속 유효) | 데디케이티드 배포 후 발견 — 재현·디버깅 비용이 훨씬 크다 |
| 7 | **(2026-08-05 신규, §5-2-1)** `MissionTable` 12행 **전부**(`Gameplay` 6종 포함, `Dialogue` 6종뿐 아니라) `NPCId` 가 유효한 값(그 미션을 Offer 하는 담당 NPC)으로 채워져 있어야 한다 | 미수락 상태의 재타겟팅(§5-2-1)이 `MissionType` 과 무관하게 `NPCId` 를 쓴다 — 어느 한 행이라도 `NoNPCId`(9999)면 그 미션이 미수락인 동안 안내가 조용히 사라진다(§5-2-1의 "도달 불가" 전제 자체가 깨진다). npc-mission-dialogue.md §3-6·§2 표의 "담당 `MissionId`" 값과 반드시 일치해야 한다 |

---

## §9 결정 메트릭 (플레이테스트)

| ID | 메트릭 | 목표 범위 | 어긋나면 조정할 값 |
|---|---|---|---|
| M-nav-1 | 코너가 있는 파쿠르 레인에서 0.75초 갱신 지연이 "라인이 실제 경로와 어긋난다"는 인상을 주는가 | 인상 없음 | `UpdateIntervalSeconds` 0.75 → 0.5(§6, 히치 유무 함께 확인) |
| M-nav-2 | Gameplay 6종(Vault/Climb/GrappleHook/Kill/Level/Combo, 2026-08-05 Kill/Level/Combo 추가) 구역/전투구역 진입점에서 300cm(`ArrivalHideDistanceCm`) 임계가 너무 이르게/늦게 사라지는가(§4-1 재사용 캐비어) | 위화감 없음 | 구역 미션 전용 임계값으로 분리(현재는 YAGNI로 전역 상수 하나만 사용) |
| M-nav-3 | (a) **최초 도착 방향**: §4-3 상태 머신 적용 후 목표 도착 시점에 "발밑에서 분리된 조각"이나 "뚝 끊김"이 관측되는가. (b) **재접근 방향**: §4-3 "한계(허용)" 2항의 재표시 스텁(재접근 시 오프셋이 곧바로 스냅)이 실제 플레이에서 거슬릴 만큼 관측되는가 | (a) 관측되지 않음(§4-2 문제 해소가 목표) — (b) 는 애초에 발생을 인정한 한계이므로 "빈도가 낮고 거슬리지 않음"이 목표 | (a) 어긋나면 `StartOffsetDistanceCm`/`ArrivalHideDistanceCm` 재산출(§4-2 문제로 회귀했다는 뜻). (b) 거슬리면 그때 가서 재표시 임계값을 400 보다 크게 분리(§4-3 한계 2항의 YAGNI 재검토) |
| M-nav-4 | 시안 라인(`rgba(0.373,0.816,0.851,1.0)`)이 지형/조명 조건에 따라 잘 안 보이는 구간이 있는가 | 전 구간에서 시인 | 폭 10cm → 14cm 상향, 또는 이미시브 강도 상향(아트 작업) |
| M-nav-5 | §2-3 수정 후 2인 PIE(원격 클라이언트)에서 라인이 정상 표시·소멸되는가 — **단일 실패점 회귀 확인** | 항상 정상 | 회귀 시 §2-3 diff 판정 로직 재검토(최우선 처리, 나머지 메트릭보다 선행) |
| M-nav-6 | **울프-미션10 근접 쌍**(§5-3-1, design-reviewer 1차 MAJOR) — Climb(미션9) 완료 후 미션10(울프 보고) 자동 수락 시점에 라인이 아예 안 뜨는가, 뜬다면 그것이 위화감을 주는가 | 라인이 안 뜨더라도 위화감 없음(플레이어가 이미 울프를 찾아 미션9 를 수락했으므로 재안내가 불필요하다는 것이 설계 의도, §5-3-1) | 위화감이 관측되면(예: "이 미션만 안내가 없다"는 혼란) 이 미션에 한해 `ArrivalHideDistanceCm` 를 낮추는 미션별 예외를 검토 — 지금은 전역 상수 하나로 시작(YAGNI, §5-3-1) |
| M-nav-7 | **(2026-08-05 신규, §5-2-1)** 세션 시작(첫 미션 미수락) 시점부터 레이븐에게 라인이 정상 표시되는가 — 이 규칙이 애초에 풀려던 세션 시작 닭-달걀 문제의 직접 확인 | 세션 시작과 동시에 라인 표시 | 표시되지 않으면 §5-8의 바인드 시점 pull 이 빠졌거나 §5-2-1의 `UseNPCIdBranch` 판정이 잘못 구현된 것 — 최우선 처리(이 기능 전체의 존재 이유) |
| M-nav-8 | **(2026-08-05 신규, §5-8)** `Dialogue` 미션 완료 직후(다음 미션이 미수락 `Gameplay`로 전이하는 6건, `MissionId` 2/4/6/8/10/12) 라인이 끊기지 않고 다음 담당 NPC로 자연스럽게 전환되는가 | 끊김 없이 전환 | 끊김이 관측되면 §5-8 안 B 의 "정지 없이 직접 전환" 전제가 실 구현에서 깨진 것 — 재계산 함수가 여전히 `StopPath()` 를 경유하는지 확인 |

---

## §10 이번 범위에서 명시 제외

| 항목 | 사유 |
|---|---|
| ~~NPC/목표지점 액터 자동 탐색·위치 레지스트리~~ | **§0(2026-08-05) 개정으로 반전 — 더 이상 제외 항목이 아니다.** spec §2 원안의 이 제외를 이번 개정이 뒤집는다(§0). 아래 행이 대체한다 |
| **미션 진행 중 NPC/마커의 실시간(런타임) 이동 추적** | §5-4 가 명확히 하는 경계 — **에디터에서 재배치 → 다음 플레이 세션에 자동 반영은 지원**(§8 조건 5)하지만, **플레이 도중 NPC/마커가 움직이는 경우 실시간 재조회는 지원하지 않는다**(좌표는 재계산 시점 1회 캡처, §5-4). 이 프로젝트에 이동하는 NPC/마커가 없어(전부 정지형 "핸들러" 캐릭터) YAGNI로 남긴다 |
| ~~**수락된** Gameplay 미션(Kill/Level/Combo)의 길 안내~~ | **2026-08-05 재반전 — 더 이상 제외 항목이 아니다.** spec §2 원안의 이 제외를 §0 이 먼저 좌표 소스 전환으로 건드렸고(§5-2-1, "수락 이후"로 좁혀 유지), 이번 개정이 그 좁혀진 제외마저 완전히 뒤집는다 — Kill/Level/Combo 3종에도 전용 마커(§5-1 2항, §5-4)가 배치돼 Vault/Climb/GrappleHook 과 동일하게 **수락 후에도** 길 안내를 받는다(§5-5) |
| 다중 활성 미션 동시 경로 표시 | spec §2 비목표 — `MissionState.MissionIndex` 단일 활성 미션만 |
| 화면 가장자리 방향 마커·미니맵·3D 공중 마커 | spec §2 비목표 — 바닥 라인 하나로 확정. npc-mission-dialogue.md §7·hud-mana-compass-skillbar.md §5/§8 이 보류했던 **다른 형태**들은 여전히 범위 밖 |
| 서버/타 플레이어 동기화 | spec §2 비목표 — 순수 로컬 클라이언트 연출. §5-4 의 레지스트리도 서버 레플리케이션이 없는 순수 로컬 구조라 이 제외와 모순 없다 |
| 구역 미션(Vault/Climb/GrappleHook) 전용 임계값 분리 | YAGNI — 전역 상수(§4-3) 하나로 시작, M-nav-2 로 필요성만 관측 |
| 적응형/가변 갱신 주기 | YAGNI — 단일 상수(§6)로 충분, 코너 밀집 구간별 차등 주기는 도입하지 않음 |
| `NavPathGlow` 머티리얼 실제 제작 | 아트 작업, spec §6 범위 밖 — §3 은 톤 가이드만 제공 |
| ~~`Mission_TargetLocation` 실제 `FVector` 좌표값 확정~~ | **§0 개정으로 무의미** — 좌표를 코드가 아니라 레벨 배치로 결정하므로 "확정할 좌표값" 자체가 없다. 대신 §8 조건 1(마커 배치)이 그 자리를 대신한다 |

---

## §11 Self-Review

- **Placeholder 잔존(5카테고리)**: 0건. 색상·두께·임계값·주기 전부 근거+대안+판정으로 확정. §0 개정 후 좌표는 "MCP 재조회로 확정"(옛 방식)이 아니라 **레벨 액터 배치 그 자체**가 좌표다(§5-4) — 별도 확정 절차가 필요 없어졌으므로 이 카테고리 위험이 오히려 줄었다.
- **스펙 커버리지**: spec §3(데이터)→§5(2026-08-05 개정판), §4(델리게이트)→§2·§7-5, §5(컴포넌트 라이프사이클)→§4·§7-5, §6(렌더링)→§3·§7-5, §7(테스트 범위)→§7-5 항목3·§5-7(신규 World 의존성 요구), §8(열린 질문·직접 참조)→§7-2·§5-4("직접 참조" 원칙을 레지스트리에도 동일 적용). §0 이 명시하는 spec §2/§10 의 반전(액터 레지스트리)도 §5 전체가 커버. 갭 0건.
- **내부 일관성**: 색상값(`rgba(0.373,0.816,0.851,1.0)`)·폭(10cm)·임계값(300/100/400)·주기(0.75초)가 §3~§9 전체에서 동일 표기 — **이 값들은 좌표 소스 전환과 무관해 §0 이후에도 변경되지 않았다(§0 명시).** 신규 도입 개념(`TargetMissionTag`/`NPCId` 두 키 공간, §5-4)도 §5~§8 전체에서 표기 일관.
- **시그니처/명명 일관성**: 기존 식별자(`USpyNavigationComponent`/`OnRep_MissionState`/`RecomputePath`/`ApplyPathPoints`/`SpyNavPathMath`) 유지. **§5-8 개정으로 `HandleMissionAccepted`/`HandleMissionCompleted`/`OnAllMissionsCompleted` 구독은 폐기 대상으로 전환됐다** — §5-8·§7-6에서만 "제거 대상"으로 등장하고, 이 문서의 새 서술(§5-5·§9)은 통합된 재계산 함수와 `OnMissionProgressChanged` 하나만 지칭한다(Grep으로 옛 두 함수명이 신규 절에 남아있지 않음을 확인). **폐기된 식별자**(`Mission_TargetLocation`/`FSpyMission_TargetLocationRow`/`GetMissionTargetLocation`)는 §1·§7-6 에서 취소선 또는 "제거 대상"으로만 등장. 신규 식별자(`USpyMissionTargetRegistrySubsystem`/`ASpyMissionTargetPoint`/`TargetMissionTag`/`RegisterNPCLocation`/`FindMissionTargetLocation`/`UseNPCIdBranch`)는 §5-4·§5-5·§7-6·§8 전체에서 동일 표기.
- **모호 표현**: "적당히/또는/재량" 0건. §5-2-1은 미수락 게이트를 "`MissionType` 무관, `NPCId` 우선"이라는 단일 규칙으로 명시했고, §5-8은 3안(현행유지+병행구독 / 단일통합 / 조건부화) 비교 후 권장안(B)과 그 근거를 명시했다 — 코디네이터가 판단을 요청한 지점이라 대안·trade-off·권장을 모두 남겼다(§5-8).
- **스코프**: 이번 라운드는 두 축 — (1) npc-mission-dialogue.md의 폐기된 Greeting 부트스트랩 접근을 전부 되돌리는 롤백(§0, 다른 문서), (2) 그 자리를 대체하는 내비게이션 규칙 신설(§5-2-1·§5-5·§5-8, 이 문서). §2/§3/§4/§6 은 이번에도 **손대지 않았다** — 이전 라운드들의 색상·두께·임계값·주기 결정과 독립적이다. 트리거 메커니즘 변경(§5-8)은 §2가 확립한 안전성 분석 위에서만 이뤄져 그 라운드의 재작업을 요구하지 않는다.
- **구현 요청사항 완전성**: Tag(없음)·인터페이스(없음)·DataAsset(제거만, §7-3·§7-6)·GA/GE(없음) 전부 명시. §7-6에 신규 클래스 2종 + 기존 클래스 확장 2종(§5-8 델리게이트 통합 포함) + 제거 대상 2종 + 테스트 영향 범위 + 에셋 정리 권고까지 전부 명시.

**Self-Review: 통과** — 검토 8라운드 반영(advisor 4·design-reviewer 2·code-reviewer 1·코디네이터 정정 1). **8차(코디네이터 정정, 2026-08-05, Kill/Level/Combo 마커 배치 — 사용자가 §5-1 원칙 3항 뒤집음)**: 사용자가 "장소 무관 Gameplay 미션(Kill/Level/Combo)은 대상 없음" 결정을 뒤집고, 코디네이터가 DevMap 전투 구역 진입점 3곳에 기존 `ASpyMissionTargetPoint` 클래스(신규 코드 없음)를 추가 배치함에 따라 문서를 실제 배치 상태와 정합시켰다 — §5-1 원칙 3항 폐기(2항으로 통합), §5-2·§5-3·§5-5·§5-6의 "Kill/Level/Combo 는 대상 없음" 전제 서술 정정, §7-6·§8 조건 1·2 를 3개→6개 마커로 확장, §9 M-nav-2 관측 범위 확장, §10 "수락된 Gameplay 미션 길 안내" 제외 항목 반전(더 이상 제외 아님). 코드·아키텍처 변경 없음(레지스트리의 `MatchTag` 조회는 원래 태그 종류 무관 범용 로직) — 문서만의 정정이라 design-reviewer 재검토는 생략한다. **7차(advisor, 2026-08-05 미수락 상태 내비게이션 규칙 신설 — Greeting 부트스트랩 폐기 대체)**: npc-mission-dialogue.md의 세션 시작 부트스트랩 미션 접근(`MissionId` 삽입, `Greeting` 상태)이 사용자 결정으로 전면 폐기되고 12행 원본으로 롤백됨에 따라, 같은 문제(세션 시작 닭-달걀)를 이 문서 쪽 내비게이션 규칙으로 재해결했다 — (a) §5-2-1을 신설해 "미수락이면 `MissionType` 무관 담당 `NPCId`로, 수락되면 §5-2 원안대로"라는 게이트를 §5-2 앞에 추가하고 `NoNPCId` 분기는 라이브 데이터상 도달 불가함을 명시, (b) §5-5의 조회 로직을 `UseNPCIdBranch` 판정으로 통합, (c) §5-8을 신설해 코디네이터가 명시 요청한 판단(완료 시 무조건 `StopPath()`를 유지할지, 재타겟팅을 즉시 이어갈지)에 대해 3안을 비교하고 "델리게이트 3개(`OnMissionAccepted`/`OnMissionCompleted`/`OnAllMissionsCompleted`)를 `OnMissionProgressChanged` 1개로 통합"을 권장안으로 채택 — 근거는 §2-3이 이미 확립한 "완료→수락 순서 보장"이 이 델리게이트 하나만 봐도 최종 상태 정합을 담보한다는 것, (d) 과잉 재계산 가드(`(MissionIndex, bAccepted)` 캐싱)와 바인드 시점 pull을 필수 조건으로 명시, (e) §8 조건 7·§9 M-nav-7·M-nav-8을 신설해 이 신규 규칙의 데이터 전제(12행 전부 `NPCId` 필수)와 UX(세션 시작 즉시 표시, 완료 전이 시 끊김 없음)를 관측 대상으로 등록. §2/§3/§4/§6의 기존 결정은 변경하지 않았다. **5차(advisor, 2026-08-05 좌표 소스 전환)**: 사용자 요청(DataTable→레벨 배치 액터 전환)에 대해 (a) `MissionType` 을 1차 게이트로 명시해 "NPCId 유무 = 목표 있음" 오판정을 사전 차단(§5-2), (b) Gameplay 구역 미션이 담당 NPC 위치를 재사용할 수 없는 이유를 "데이터 없음"이 아니라 "유도 가치 0"으로 정정 논증(§5-3), (c) `ASpyInteractableObject` 실측(NPCId 없음, `MissionEventTag` 있음, `EndPlay` 오버라이드 없음)으로 `Interact` 타입을 NPCId 브랜치에서 `MatchTag` 브랜치로 재배선해 조용한 조회 실패를 사전 차단 + 소진(consume) 상호작용을 미결 사항으로 명시, (d) 레지스트리를 좌표 스냅샷이 아니라 액터 약참조+조회 시점 `GetActorLocation()` 으로 설계해 에디터 재배치 자동 반영을 확보, (e) `SpyNavigationComponentTests.cpp` 의 World 의존성 회귀를 §5-7·§7-6 으로 선제 명시, (f) §2-3 의 완료→수락 순서와 신규 재시도 타이머의 `StopPath()` clear 가 정합됨을 §5-6 에서 명시적으로 연결. **6차(design-reviewer 1차, MAJOR)** (12) §5-3 의 "유도 가치 0" 논증이 Gameplay 에만 적용되고 구조적으로 동일한 근접 문제가 생길 수 있는 Dialogue 분기(특히 울프-미션10, Climb 오브젝트 근처로 명시 배치됨)를 검토하지 않았다는 지적 — §5-3-1 을 신설해 (i) Dialogue 는 목적지 자체가 항상 정확해(§5-3 의 "틀린 목표" 전제가 성립 안 함) 구조적 해법이 불필요함을 논증하고, (ii) 그럼에도 남는 "정확한 목표인데 그 홉에서 라인이 한 번도 안 보일 수 있다"는 예외를 인정해 배치 서술 근거로 확신도를 매겨(울프-미션10 높음, 바이퍼-미션6 낮음, 나머지 근거 없음) 마커 등 구조적 대응 없이 M-nav-6(§9) 관측 대상으로만 등록. 레벨 실측 좌표는 확인하지 않았음을 명시(리뷰어도 단정하지 않은 부분). MINOR 1건(§7-6 테스트 영향 범위 "일부"→"전체 6개")은 이미 §5-7 결론과 동일해 재작업 불필요 확인. 이전 4라운드 기록은 아래에 그대로 보존한다. **1차(advisor)** (1) `SpyMissionComponent.h/.cpp` 실측으로 클라이언트 트리거 단일 실패점을 §2 로 신설, (2) §4 시작 오프셋/도착 임계값을 독립 판단에서 합성 상태 머신(히스테리시스 + 유도값)으로 재구성, (3) 색상값을 실측 rgba 로 특정하고 session-browser.md `#5AC8D8` 과의 차이를 명시, spec "골드" 비유를 명시적으로 기각, (4) 300cm 임계값이 Dialogue 미션에만 독립 도출이고 구역 미션엔 재사용값임을 명시 + M-nav-2 로 분리. **2차(advisor)** (5) §2-3 판정이 서버 트랜지션과 1:1 대응하지 않음을 인정하고 내비게이션 목적에는 충분함을 별도 논증(허위 등가 주장 제거) + 리슨 서버 중복 발화 없음을 추가, (6) `RecomputePath` 경로 질의 실패 시 라인이 얼어붙는 무증상 실패를 §7-5 항목 6·§8 로 신설, (7) 재접근 시 재발생하는 분리 조각을 §4-3 한계 2항으로 인정 + M-nav-3 을 도착/재접근 두 방향으로 분리, (8) `ESpyMissionType::Interact` 를 §5-1 total 규칙에 포함, (9) `MaxWalkSpeed` 인용을 "코드 기본값(걷기 기준)"으로 좁히고 라이브 DataAsset 미확인을 명시, 이후 §11 자체의 항목 수 오기(5→6)·§7-5 항목3 "4개→3개 임계값" 표기 오류·§4-3 한계 1항의 "최대 속도" 표현을 §6 의 "걷기 기준" 재서술과 맞춰 수정. **3차(design-reviewer, BLOCKER)** (10) §2-3 이 나열한 수락/완료 판정 순서가 실제로는 "수락 먼저" 로 읽혀, `HandleMissionAccepted`(새 경로 시작) 직후 `HandleMissionCompleted`(무조건 `StopPath()`) 가 그 경로를 즉시 지우는 결함을 지적받음 — 서버측 `ProcessProgress()` 가 이미 "완료 → 상태 갱신 → 수락" 순서임을 근거로 §2-3 을 **완료 판정을 수락 판정보다 먼저 실행**하도록 재정렬하고, 이 순서가 Dialogue 미션 자동 전이 6건(`MissionId` 2/4/6/8/10/12) 전부와 재접속(late-join) 시나리오 모두에서 최종 상태를 올바르게 만든다는 근거를 명시. **4차(code-reviewer, 구현 후 BLOCKER)** (11) §4-3 의 히스테리시스 규칙이 "이전 프레임의 가시성"을 전제해 **콜드 스타트(경로를 막 시작하는 첫 프레임, 지킬 이전 상태가 없음)를 다루지 않는 공백**을 실 구현(`bPathVisible` 기본값 `false`)에서 발견 — 이를 방치하면 목표까지 300~400cm(또는 그 미만)인 채로 미션이 시작될 때 그 미션 동안 라인이 한 번도 안 보이는 결함이 생김을 확인하고, "콜드 스타트는 이전 프레임에 보이고 있었다로 취급(가시성 상태를 `StartPathTo()` 시점에 `true` 로 시드)"는 한 문단을 §4-3 에 추가해 규칙 1을 즉시 적용 가능하게 함.
