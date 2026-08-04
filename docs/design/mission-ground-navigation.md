# 미션 목표 바닥 길 안내 (Ground Path Navigation) — 도메인 기획 (톤 · UX · 데이터 배정)

> **이 문서의 범위**: [spec](../superpowers/specs/2026-08-04-mission-ground-navigation-design.md) 과 [plan](../superpowers/plans/2026-08-04-mission-ground-navigation.md) 이 확정한 **구조(데이터 스키마 · 컴포넌트 · 순수 함수 슬롯) 위에 얹을 수치와 UX 감각**만 결정한다. 클래스·파일·함수 시그니처는 plan 이 이미 확정했고 이 문서는 다시 제안하지 않는다 — 단, plan 에 없어서 구현 시 드롭될 위험이 있는 항목은 §7-5 에 "plan 개정 필요"로 명시한다(hud-mana-compass-skillbar.md §7-0 선례와 동일 처리).
>
> **이 문서가 뒤집는 이전 보류 결정**: [npc-mission-dialogue.md](npc-mission-dialogue.md) §7 은 "HUD 상 '다음 NPC 위치' 마커·미니맵 표시"를 "spec 비목표 범위 밖, §3-3-7 의 `Report` 문구 핸드오프로 대체"로 명시 제외했고, [hud-mana-compass-skillbar.md](hud-mana-compass-skillbar.md) §5·§8 은 "미션 웨이포인트·거리·미니맵·목표까지 화살표"를 spec §6 비목표로 명시 제외했다. 이번 기능은 그 두 보류를 **바닥 글로우 라인이라는 세 번째 형태**로 뒤집는다 — 화면 가장자리 마커나 미니맵이 아니라 월드 공간 바닥 라인이라는 점에서 두 문서가 다뤘던 형태와 겹치지 않으므로, 두 문서의 다른 결정(나침반 §5, 스킬바 §6 등)은 이 문서가 건드리지 않는다. `Report` 문구 핸드오프(§3-3-7)는 **대체되는 것이 아니라 병행**된다 — 텍스트 안내(다음 담당자 이름)와 시각 안내(바닥 라인)가 서로 다른 채널이라 상호 보강이다.

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
| `Mission_TargetLocation` 은 선택적 관계 테이블(`MissionId`→`TargetLocation`), 로우 없으면 길 안내 없음(HUD 텍스트만) | — | spec §2·§3 |
| `USpyNavigationComponent` 는 로컬 컨트롤 클라이언트 전용, 서버/데디케이티드에서 렌더링 없음 | — | spec §5 |
| 갱신 주기 초안 `UpdateIntervalSeconds = 0.75f` (`EditDefaultsOnly`) | 0.75초 | spec §5, plan Task 4 Step 1 |
| 렌더링은 `USplineComponent` 1개 + `USplineMeshComponent` 세그먼트 풀 재사용(destroy/spawn 반복 없음) | — | spec §6 |
| 글로우 머티리얼은 `USpyAssetManager` 이름 룩업(`"NavPathGlow"`), 실제 머티리얼 에셋 제작은 스펙 범위 밖(아트 작업) | — | spec §6, plan Task 5 Step 5 |
| `USpyMissionComponent::GetMissionTargetLocation(int32)` 이 `USpyMissionConfig::GetMissionTargetLocation` 을 패스스루 | — | plan Task 2 |
| `USpyNavigationComponent` 는 `USpyMissionComponent` 를 인터페이스가 아니라 구체 클래스로 직접 참조 (기존 `USpyMainHUD` 선례) | — | plan Global Constraints, spec §8 |
| **미션 스키마 SoT — `FSpyMissionRow`(`SpyMissionConfig.h`)** — `MissionId`(1-based, 12행)·`MissionType`(`Gameplay`/`Dialogue`)·`NPCId`(직접 필드, sentinel `NoNPCId=9999`) | — | `SkillProject/Source/SkillProject/Data/SpyMissionConfig.h:24-63` (코드 실측) |

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

**⚠ 300cm 는 대화형(NPC 보고) 미션에만 독립적으로 도출된 값이다.** §5 에서 좌표를 채우는 파쿠르 구역 미션(넘기/벽타기/그래플, 3개 행)에는 대응하는 상호작용 판정 반경이 없다 — 이 값을 재사용하는 건 "하나의 통일된 규칙을 유지한다"는 단순성 근거이지 독립 도출이 아니다. 구역 미션에서 너무 빠르거나 늦게 사라지면 §9 M-nav-2 로 개별 조정한다.

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

## §5 `Mission_TargetLocation` 데이터 저작 가이드 — 어느 미션에 채우는가

spec §3 은 "선택적 관계 — 목표 지점이 있는 미션만 로우가 존재"까지만 정하고 어느 `MissionId` 에 채울지는 정하지 않는다. §1 의 스키마 정정(직접 `NPCId` 필드)을 반영해, 현재 12행 미션 스키마(`SpyMissionConfig.h` 실측, npc-mission-dialogue.md §3-6 값 계승) 기준으로 확정한다.

### 5-1. 원칙

1. **`MissionType == Dialogue`(NPC 보고) 미션은 전부 채운다.** "누구에게 가서 보고할지"가 항상 명확한 단일 좌표(그 NPC 의 월드 위치)로 존재하기 때문이다 — spec §3 이 이미 "Dialogue/Interact 타입도 이 좌표를 그대로 목적지로 사용한다"고 확정했다. **`ESpyMissionType::Interact` 도 동일 취급**한다(`ProcessProgress` 가 `Dialogue || Interact` 를 같은 분기로 처리 — `SpyMissionComponent.cpp:179-180`) — 현재 12행 데이터에는 `Interact` 타입 행이 없어 §5-2 표에 실제 사례는 없지만, 향후 `Interact` 미션이 추가되면 이 원칙을 그대로 적용한다.
2. **`MissionType == Gameplay` 미션 중 특정 구역에 결부된 것(파쿠르 이동 스킬)만 채운다.** Vault/Climb/GrappleHook 는 mission-system.md §2-4 가 이미 "전투 구역/파쿠르 레인/그래플 타워"라는 구역으로 나눠 배치했다 — 그 구역의 **진입점 좌표 하나**를 대표값으로 채운다.
3. **장소 무관 `Gameplay` 미션은 비운다.** Kill(적 아무나 처치)·Level(전투 자체로 누적)·Combo(대상 불요, mission-system.md §1-3)는 "여기로 가라"고 가리킬 단일 지점이 없다 — spec §2 의 제외 항목("목표 지점이 정의되지 않은 Gameplay 미션")이 정확히 이 셋을 가리킨다. 로우 자체를 만들지 않는다(§14-1 선택적 관계 원칙 — sentinel 로 채우지 않는다).

### 5-2. 확정 표 (`MissionId` 1~12, `SpyMissionConfig.h`/npc-mission-dialogue.md §3-6 스키마 기준)

| `MissionId` | `MissionType` | `DisplayName` | `Mission_TargetLocation` | 좌표 |
|---|---|---|---|---|
| 1 | Gameplay | 적 1명 처치 | **비움** | — 장소 무관(§5-1-3) |
| 2 | Dialogue | 레이븐에게 보고하십시오 | **채움** | 레이븐(`NPCId 1`) 월드 위치 |
| 3 | Gameplay | 레벨 3 달성 | **비움** | — 장소 무관 |
| 4 | Dialogue | 팰컨에게 보고하십시오 | **채움** | 팰컨(`NPCId 2`) 월드 위치 |
| 5 | Gameplay | 콤보 4회 연결 | **비움** | — 장소 무관(대상 불요) |
| 6 | Dialogue | 바이퍼에게 보고하십시오 | **채움** | 바이퍼(`NPCId 3`) 월드 위치 |
| 7 | Gameplay | 장애물 넘기 5회 | **채움** | 파쿠르 레인(Vault 구역) 진입점 |
| 8 | Dialogue | 스패로우에게 보고하십시오 | **채움** | 스패로우(`NPCId 4`) 월드 위치 |
| 9 | Gameplay | 벽 타기 3회 | **채움** | 파쿠르 레인(Climb 구역) 진입점 |
| 10 | Dialogue | 울프에게 보고하십시오 | **채움** | 울프(`NPCId 5`) 월드 위치 |
| 11 | Gameplay | 그래플링 3회 | **채움** | 그래플 타워 진입점 |
| 12 | Dialogue | 폭스에게 보고하십시오 | **채움** | 폭스(`NPCId 6`) 월드 위치 |

**검산**: 12행 중 채움 9(Dialogue 6 + Vault·Climb·GrappleHook 3) / 비움 3(Kill·Level·Combo) — §5-1 원칙 3항과 정확히 일치.

### 5-3. "구역 진입점"은 특정 오브젝트가 아니다 — 데이터 저작 시 주의

7·9·11 번의 좌표는 **그 미션을 채울 수 있는 여러 유효 오브젝트 중 "정답 하나"를 가리키는 게 아니다.** mission-system.md §1-2 결론대로 Vault/Climb 는 조건을 만족하는 임의 지오메트리에 반복 반응하고, GrappleHook 는 앵커 19개 중 아무거나 유효하다(mission-system.md §1-2-1). 좌표는 **"이 구역으로 가라"는 진입 안내**이며, §4 의 300cm 임계 이내로 접근하면 라인이 사라지고 그 뒤로는 플레이어가 구역 안에서 자유롭게 오브젝트를 찾아 반복 수행한다. 데이터 저작 시 "정확히 이 오브젝트를 가리켜야 하나?"로 고민할 필요가 없다는 뜻을 에디터 작업자에게 명확히 전달한다(§8 에디터 데이터 조건에 재명시).

7·9·11 번 좌표가 반드시 같은 구역의 8·10·12 번(해당 NPC) 좌표와 동일할 필요는 없다 — 같은 구역이라 실무적으로 인접할 뿐, 독립적으로 저작한다.

### 5-4. 정확한 좌표값

이 문서는 좌표의 **원칙**만 정한다. 실제 `FVector` 값은 npc-mission-dialogue.md §4-6 과 동일한 이유로 이 문서가 정하지 않는다 — NPC 6종의 정확한 트랜스폼은 "맵 MCP 재조회로 확인 필요"이고, 파쿠르 구역 진입점도 마찬가지로 레벨의 실제 배치를 반영해야 한다. plan Task 5 Step 6(수동 PIE 검증)와 Task 1 의 데이터 저작 단계에서 MCP 로 확정한다.

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

**신규 필드 없음.** `Mission_TargetLocation`(`FSpyMission_TargetLocationRow` — `MissionId`/`TargetLocation`)은 plan Task 1 이 이미 정의했다. 이 문서는 §5 의 값 배정 원칙만 추가한다.

### 7-4. GA · GE 명세

**신규 GA·GE 없음.**

### 7-5. ⛔ plan 개정 필요 항목 (게이트 판단 사항 아님)

hud-mana-compass-skillbar.md §7-0 과 동일하게, 아래는 task 구동 실행에서 매핑이 없으면 조용히 드롭될 수 있는 항목이다 — **구현 착수 전 plan 개정(Task 2·Task 4·Task 5 확장)이 선행돼야 한다.**

| # | 항목 | 대상 Task | 성격 | 미개정 시 결과 |
|---|---|---|---|---|
| 1 | `OnRep_MissionState` 확장 — 이전 값 파라미터 추가 + `OnMissionAccepted`/`OnMissionCompleted` 를 클라이언트에서도 상태-diff 로 판정해 브로드캐스트(§2-3 규칙 그대로) | plan Task 2 | **코드** | 데디케이티드 서버/원격 클라이언트에서 `USpyNavigationComponent` 영구 미작동(§2, 단일 실패점) — 1인 PIE 에서는 증상 없음 |
| 2 | `USpyNavigationComponent` 에 `StartOffsetDistanceCm`(100.f)·`ArrivalHideDistanceCm`(300.f)·`ArrivalReshowDistanceCm`(400.f)·`GroundZOffsetCm`(3.f) 4개 `EditDefaultsOnly float` 필드 추가 | plan Task 5 | 코드 | §4 합성 규칙을 표현할 자리가 없어 발밑 분리 조각·뚝 끊김 현상이 그대로 남는다 |
| 3 | 가시성 상태 판정 로직(§4-3 상태 머신) — `RecomputePath`/`ApplyPathPoints` 확장. **입력→출력이 순수 함수로 분리 가능**(경로점 배열 + 누적 길이 + 3개 임계값(`StartOffsetDistanceCm`/`ArrivalHideDistanceCm`/`ArrivalReshowDistanceCm`) → 트리밍된 경로점 배열 + 가시성 bool — `GroundZOffsetCm` 는 이 판정과 무관하게 항목 4 에서 별도로 적용한다)하므로, Task 3 의 `SpyNavPathMath` 모듈(순수 함수 전용, 테스트 가능)에 이 판정을 추가하는 것이 spec §7 의 "테스트 가능 범위" 원칙과 정확히 들어맞는다. 정확한 함수명·시그니처는 gameplay-programmer 판단 | plan Task 3 확장 | 코드 | 렌더 경로에 직접 박히면 §7 이 요구하는 자동화 테스트 커버리지(회귀 고정)를 못 만든다 |
| 4 | `GroundZOffsetCm`(3.f) 을 `ApplyPathPoints` 에서 각 경로점 Z 에 더해 지면 z-fighting 방지 | plan Task 5 | 코드 | 바닥 데칼/지형과 스플라인 메시가 겹쳐 플리커링 발생 가능 |
| 5 | `NavPathGlow` 머티리얼(아트, 범위 밖) 제작 시 §3 톤 가이드(색상 `rgba(0.373, 0.816, 0.851, 1.0)`, 전체 폭 10cm, 플레이어→목표 방향 UV 패닝 + 저주기 펄스) 전달 | 아트 작업(범위 밖) | 에셋 | 엔진 기본 머티리얼로 표시돼(plan Task 5 Step 5 주석) 톤이 이 프로젝트 정체성과 무관해진다 |
| 6 | `RecomputePath` 가 `NavPath == nullptr \|\| NavPath->IsValid() == false` 로 조기 반환할 때, 스플라인·세그먼트 풀을 그대로 두지 않고 **숨긴다**(§4-3 의 "숨김" 상태로 전이) | plan Task 5 | 코드 | plan 원안(Task 5 Step 5 `RecomputePath`)은 조기 반환 시 이전에 그려진 라인을 아무 처리 없이 그대로 둔다 — `TargetLocation` 이 NavMesh 밖(예: 벽 안쪽 10cm)이면 **경로 질의가 매번 실패하고 라인은 마지막으로 성공했던 낡은 경로를 가리킨 채 영구히 멈춘다.** §2 와 같은 계열의 무증상 실패이며, §8 조건 3·4(NPC 좌표 정확성·재배치 갱신)가 어긋났을 때 실제로 트리거되는 경로다 |

- 항목 1 은 **이 표에서 가장 중요하다** — 나머지 전부가 이 수정 없이는 실물 배포에서 관측조차 안 된다(§2).
- 항목 6 은 §8 조건 3·4 가 완벽히 지켜지지 않았을 때의 **완충 장치**다 — 좌표가 틀려도 최소한 "라인이 이상한 곳에 멈춰 있다"가 아니라 "라인이 없다"로 실패하게 만든다.

---

## §8 에디터 데이터 조건

| # | 조건 | 어기면 |
|---|---|---|
| 1 | `Mission_TargetLocation` 은 §5-2 표의 9행(`MissionId` 2/4/6/7/8/9/10/11/12)만 채우고, 3행(1/3/5)은 로우 자체를 만들지 않는다 | 장소 무관 미션(Kill/Level/Combo)에 임의 좌표를 채우면 "정답 위치가 있다"는 잘못된 기대를 주고, 실제로는 아무 데서나 완료되므로 라인이 목표에 도착해도 미션이 안 끝나는 것처럼 보인다 |
| 2 | 7·9·11 번(Vault/Climb/GrappleHook) 좌표는 **구역 진입점**이지 특정 오브젝트가 아니다(§5-3) — 데이터 저작자가 "정확한 오브젝트 하나"를 가리키려 하지 않는다 | 특정 오브젝트에 못 박으면, 그 오브젝트가 아닌 다른(똑같이 유효한) 오브젝트로 완료했을 때 라인이 계속 남아 있던 지점을 가리켜 혼란을 준다 |
| 3 | `MissionId` 2/4/6/8/10/12 의 좌표는 해당 NPC 블루프린트의 **실제 배치 좌표**와 일치시킨다(npc-mission-dialogue.md §4-6, 맵 MCP 재조회) | 어긋나면 라인이 NPC 가 서 있지 않은 지점으로 안내한다 |
| 4 | **NPC 재배치 시 `Mission_TargetLocation` 동반 갱신 필수** — `FSpyMissionRow::NPCId` 는 이름 조회용 필드일 뿐 좌표 자동 동기화 수단이 아니다. spec §2 가 "NPC 이동 시 자동 보정은 다루지 않는다"고 이미 제외했으므로, NPC 위치를 바꿀 때마다 이 좌표를 수동으로 함께 갱신해야 한다(알려진 제한, §10) | NPC 를 옮기고 좌표를 안 고치면 라인이 예전 위치로 안내하는 무증상 실패 — 대화가 시작되기 전까지는 발견되지 않는다 |
| 5 | §7-5 항목 1(`OnRep_MissionState` 개정)이 반영된 빌드로 **2인 이상 PIE**(호스트+원격 클라이언트)에서 검증한다 — 1인 PIE 만으로는 §2 결함이 통과된다 | 데디케이티드 배포 후 발견 — 재현·디버깅 비용이 훨씬 크다 |

---

## §9 결정 메트릭 (플레이테스트)

| ID | 메트릭 | 목표 범위 | 어긋나면 조정할 값 |
|---|---|---|---|
| M-nav-1 | 코너가 있는 파쿠르 레인에서 0.75초 갱신 지연이 "라인이 실제 경로와 어긋난다"는 인상을 주는가 | 인상 없음 | `UpdateIntervalSeconds` 0.75 → 0.5(§6, 히치 유무 함께 확인) |
| M-nav-2 | Vault/Climb/GrappleHook 구역 진입점에서 300cm(`ArrivalHideDistanceCm`) 임계가 너무 이르게/늦게 사라지는가(§4-1 재사용 캐비어) | 위화감 없음 | 구역 미션 전용 임계값으로 분리(현재는 YAGNI로 전역 상수 하나만 사용) |
| M-nav-3 | (a) **최초 도착 방향**: §4-3 상태 머신 적용 후 목표 도착 시점에 "발밑에서 분리된 조각"이나 "뚝 끊김"이 관측되는가. (b) **재접근 방향**: §4-3 "한계(허용)" 2항의 재표시 스텁(재접근 시 오프셋이 곧바로 스냅)이 실제 플레이에서 거슬릴 만큼 관측되는가 | (a) 관측되지 않음(§4-2 문제 해소가 목표) — (b) 는 애초에 발생을 인정한 한계이므로 "빈도가 낮고 거슬리지 않음"이 목표 | (a) 어긋나면 `StartOffsetDistanceCm`/`ArrivalHideDistanceCm` 재산출(§4-2 문제로 회귀했다는 뜻). (b) 거슬리면 그때 가서 재표시 임계값을 400 보다 크게 분리(§4-3 한계 2항의 YAGNI 재검토) |
| M-nav-4 | 시안 라인(`rgba(0.373,0.816,0.851,1.0)`)이 지형/조명 조건에 따라 잘 안 보이는 구간이 있는가 | 전 구간에서 시인 | 폭 10cm → 14cm 상향, 또는 이미시브 강도 상향(아트 작업) |
| M-nav-5 | §2-3 수정 후 2인 PIE(원격 클라이언트)에서 라인이 정상 표시·소멸되는가 — **단일 실패점 회귀 확인** | 항상 정상 | 회귀 시 §2-3 diff 판정 로직 재검토(최우선 처리, 나머지 메트릭보다 선행) |

---

## §10 이번 범위에서 명시 제외

| 항목 | 사유 |
|---|---|
| NPC/목표지점 액터 자동 탐색·위치 레지스트리 | spec §2 비목표 — 좌표는 레벨 디자이너 수동 입력 |
| NPC 이동 시 좌표 자동 보정 | spec §2 비목표 — §8 조건 4 로 수동 갱신 필요성만 명시 |
| 목표 미정의 Gameplay 미션(Kill/Level/Combo)의 길 안내 | spec §2 비목표 — 기존처럼 HUD 텍스트만 |
| 다중 활성 미션 동시 경로 표시 | spec §2 비목표 — `MissionState.MissionIndex` 단일 활성 미션만 |
| 화면 가장자리 방향 마커·미니맵·3D 공중 마커 | spec §2 비목표 — 바닥 라인 하나로 확정. npc-mission-dialogue.md §7·hud-mana-compass-skillbar.md §5/§8 이 보류했던 **다른 형태**들은 여전히 범위 밖 |
| 서버/타 플레이어 동기화 | spec §2 비목표 — 순수 로컬 클라이언트 연출 |
| 구역 미션(Vault/Climb/GrappleHook) 전용 임계값 분리 | YAGNI — 전역 상수(§4-3) 하나로 시작, M-nav-2 로 필요성만 관측 |
| 적응형/가변 갱신 주기 | YAGNI — 단일 상수(§6)로 충분, 코너 밀집 구간별 차등 주기는 도입하지 않음 |
| `NavPathGlow` 머티리얼 실제 제작 | 아트 작업, spec §6 범위 밖 — §3 은 톤 가이드만 제공 |
| `Mission_TargetLocation` 실제 `FVector` 좌표값 확정 | 데이터 저작 단계(MCP 좌표 재조회), 이 문서는 원칙만(§5) |

---

## §11 Self-Review

- **Placeholder 잔존(5카테고리)**: 0건. 색상·두께·임계값·주기 전부 근거+대안+판정으로 확정. 좌표 실값만 "데이터 저작 단계에서 MCP 재조회"로 명시 위임(§5-4, npc-mission-dialogue.md §4-6과 동일 처리이며 임의 확정 아님).
- **스펙 커버리지**: spec §3(데이터)→§5, §4(델리게이트)→§2·§7-5, §5(컴포넌트 라이프사이클)→§4·§7-5, §6(렌더링)→§3·§7-5, §7(테스트 범위)→§7-5 항목3 이 순수 함수 슬롯 재사용을 명시, §8(열린 질문·직접 참조)→§7-2. 갭 0건.
- **내부 일관성**: 색상값(`rgba(0.373,0.816,0.851,1.0)`)·폭(10cm)·임계값(300/100/400)·주기(0.75초)가 §3~§9 전체에서 동일 표기. §4-3 의 400 = 300+100 검산 명시. §6 의 375cm 계산(500×0.75)과 §4 히스테리시스 폭(100cm) 비교도 상호 참조.
- **시그니처/명명 일관성**: `USpyNavigationComponent`/`OnMissionAccepted`/`OnMissionCompleted`/`OnAllMissionsCompleted`/`OnRep_MissionState`/`RecomputePath`/`ApplyPathPoints`/`SpyNavPathMath`/`Mission_TargetLocation`/`FSpyMission_TargetLocationRow`/`GetMissionTargetLocation` 이 plan/spec 원문 표기와 전체 문서에서 글자 그대로 일치(Grep 확인). 신규 제안 필드명(`StartOffsetDistanceCm`/`ArrivalHideDistanceCm`/`ArrivalReshowDistanceCm`/`GroundZOffsetCm`)도 §4·§7-5·§8·§9 전체에서 동일 표기 유지.
- **모호 표현**: "적당히/또는/재량" 0건. §3 색상·두께는 대안 비교 후 단일 값으로 좁힘. §5 채움/비움 원칙은 규칙(Dialogue 전부/구역형 Gameplay/장소무관 제외)으로 total 커버.
- **스코프**: 단일 기능(바닥 길 안내) 범위. §2 의 `OnRep_MissionState` 수정은 범위 확장처럼 보이지만 **이 기능의 트리거 신뢰성 자체**이므로 포함이 맞다 — 이 문서가 고치지 않으면 이 문서의 나머지 전부가 무의미해진다(§2-4).
- **구현 요청사항 완전성**: Tag(없음)·인터페이스(없음, 시그니처 확장만)·DataAsset(값 원칙만, 스키마 변경 없음)·GA/GE(없음) 전부 명시. §7-5 에 plan 개정 **6항목**(트리거 클라이언트 안전성·신규 필드 4종·순수 함수 확장·Z 오프셋·머티리얼 톤 전달·경로 실패 시 숨김)을 hud 문서 §7-0 형식으로 못박음(드롭 방지).

**Self-Review: 통과** — advisor 검토 2회 + design-reviewer 검토 1회 반영: **1차(advisor)** (1) `SpyMissionComponent.h/.cpp` 실측으로 클라이언트 트리거 단일 실패점을 §2 로 신설, (2) §4 시작 오프셋/도착 임계값을 독립 판단에서 합성 상태 머신(히스테리시스 + 유도값)으로 재구성, (3) 색상값을 실측 rgba 로 특정하고 session-browser.md `#5AC8D8` 과의 차이를 명시, spec "골드" 비유를 명시적으로 기각, (4) 300cm 임계값이 Dialogue 미션에만 독립 도출이고 구역 미션엔 재사용값임을 명시 + M-nav-2 로 분리. **2차(advisor)** (5) §2-3 판정이 서버 트랜지션과 1:1 대응하지 않음을 인정하고 내비게이션 목적에는 충분함을 별도 논증(허위 등가 주장 제거) + 리슨 서버 중복 발화 없음을 추가, (6) `RecomputePath` 경로 질의 실패 시 라인이 얼어붙는 무증상 실패를 §7-5 항목 6·§8 로 신설, (7) 재접근 시 재발생하는 분리 조각을 §4-3 한계 2항으로 인정 + M-nav-3 을 도착/재접근 두 방향으로 분리, (8) `ESpyMissionType::Interact` 를 §5-1 total 규칙에 포함, (9) `MaxWalkSpeed` 인용을 "코드 기본값(걷기 기준)"으로 좁히고 라이브 DataAsset 미확인을 명시, 이후 §11 자체의 항목 수 오기(5→6)·§7-5 항목3 "4개→3개 임계값" 표기 오류·§4-3 한계 1항의 "최대 속도" 표현을 §6 의 "걷기 기준" 재서술과 맞춰 수정. **3차(design-reviewer, BLOCKER)** (10) §2-3 이 나열한 수락/완료 판정 순서가 실제로는 "수락 먼저" 로 읽혀, `HandleMissionAccepted`(새 경로 시작) 직후 `HandleMissionCompleted`(무조건 `StopPath()`) 가 그 경로를 즉시 지우는 결함을 지적받음 — 서버측 `ProcessProgress()` 가 이미 "완료 → 상태 갱신 → 수락" 순서임을 근거로 §2-3 을 **완료 판정을 수락 판정보다 먼저 실행**하도록 재정렬하고, 이 순서가 Dialogue 미션 자동 전이 6건(`MissionId` 2/4/6/8/10/12) 전부와 재접속(late-join) 시나리오 모두에서 최종 상태를 올바르게 만든다는 근거를 명시. **4차(code-reviewer, 구현 후 BLOCKER)** (11) §4-3 의 히스테리시스 규칙이 "이전 프레임의 가시성"을 전제해 **콜드 스타트(경로를 막 시작하는 첫 프레임, 지킬 이전 상태가 없음)를 다루지 않는 공백**을 실 구현(`bPathVisible` 기본값 `false`)에서 발견 — 이를 방치하면 목표까지 300~400cm(또는 그 미만)인 채로 미션이 시작될 때 그 미션 동안 라인이 한 번도 안 보이는 결함이 생김을 확인하고, "콜드 스타트는 이전 프레임에 보이고 있었다로 취급(가시성 상태를 `StartPathTo()` 시점에 `true` 로 시드)"는 한 문단을 §4-3 에 추가해 규칙 1을 즉시 적용 가능하게 함.
