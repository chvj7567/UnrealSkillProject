# 미션 타겟 네비 숨김 트리거 영역 — 도메인 기획 (Box Extent · 사용 가이드 · UX 검수)

> **이 문서의 범위**: [spec](../superpowers/specs/2026-08-10-nav-hide-trigger-volume-design.md) 과 [plan](../superpowers/plans/2026-08-10-nav-hide-trigger-volume.md) 이 확정한 **구조**(레지스트리 API, `ISpyMissionTargetHideVolume` 인터페이스, 3개 액터의 컴포넌트 배선, 콜리전 프로파일 재사용 방침)는 재설계하지 않는다. 이 문서는 plan 이 placeholder 로 남긴 **기본 Box Extent 수치**와, 그 수치 위에 얹는 **디자이너 사용 가이드 · UX 검수 기준**만 결정한다.
>
> **인터페이스 이름 정정**: spec §4 초안은 `IMissionTargetHideVolume`(프로젝트 접두사 없음)로 썼으나, 이후 확정된 plan Task 2 Step 1 은 `ISpyMissionTargetHideVolume`(`System/CommonInterface.System.h`, cpp-style §1 프로젝트 접두사 규칙 반영)로 정정했다. **이 문서는 plan 의 이름을 SoT 로 채택하고 전체 본문에서 `ISpyMissionTargetHideVolume` 만 사용한다** — `IMissionTargetHideVolume` 표기는 이 문서에 등장하지 않는다.
>
> **선례**: [mission-ground-navigation.md](mission-ground-navigation.md) 가 같은 컴포넌트(`USpyNavigationComponent`)의 색상·두께·임계값을 정한 직전 문서다 — 이 문서가 재확정하는 `ArrivalHideDistanceCm`(300cm)/`ArrivalReshowDistanceCm`(400cm)/`StartOffsetDistanceCm`(100cm)는 그 문서 §4-3 이 이미 확정한 값을 입력으로만 쓴다(재도출하지 않음).
>
> **기본값 후속 정정(2026-08-10)**: 이 문서·spec·plan 원안은 `bEnableHideTrigger` 기본값을 `false`(opt-in, 하위 호환 우선)로 확정했었다. PIE 검증에서 사용자가 기본 비활성 상태를 실제 버그로 오인해 즉시 `true`(opt-out)로 뒤집을 것을 요청 — gameplay-programmer 가 3개 액터 헤더 기본값과 테스트를 반전, DevMap 기존 배치 12개(마커 6·NPC 6)도 명시적으로 켰다. 이 문서 §2-4/§3-4/§4-3 등 본문의 수치·하한·UX 판단 자체는 기본값과 무관하게 그대로 유효하다 — 바뀐 건 "opt-in이냐 opt-out이냐"뿐이다.

---

## § 헤더

- **목표** — 미션 타겟 3개 액터(`ASpyMissionTargetPoint`/`ASpyNPCCharacter`/`ASpyInteractableObject`)에 추가되는 선택적 `HideTriggerVolume`(`UBoxComponent`)의 기본 Extent 수치를 정하고, 디자이너가 "언제 트리거를 켜야 하는가"와 "박스 트리거의 즉시 숨김/표시가 체감상 자연스러운가"를 판단할 기준을 제공한다.
- **검증 가설** — (1) 기본 Extent 가 기존 거리 히스테리시스(300/400cm)와 체감 크기가 어긋나지 않는가, (2) 트리거 사용 기준이 "복도·여러 층"처럼 막연한 형태 설명이 아니라 실제 폴백 메커니즘(NavMesh 경로 길이)의 한계에 근거하는가, (3) 히스테리시스 없는 즉시 전환이 박스 경계 근처에서 깜빡임(edge flicker)을 유발하지 않는가.
- **현재 단계 범위 적합성** — `project.md` 의 `stage`/`stage_goal` 과 컨셉서 §4 는 `(사용자 확정 대기)`다. [mission-ground-navigation.md](mission-ground-navigation.md) §헤더가 세운 선례를 그대로 따른다 — **사용자와의 브레인스토밍·계획 수립이 끝난 spec/plan 의 목표·비목표를 이 문서의 범위 경계로 삼는다.** spec §2 "제외" 항목(Box 외 형태, 트리거 내 추가 연출, 서버 레플리케이션, `InteractionSphere` 와의 통합, 기존 히스테리시스 수치 변경)은 이 문서도 다루지 않는다.
- **핵심 메커니즘** — 트리거 활성(현재 기본값, 위 정정 참조) 타겟은 오버랩 진입/이탈로 네비 라인을 거리 계산 없이 즉시 숨기고/표시한다(spec §6). 트리거 비활성(opt-out) 타겟은 `USpyNavigationComponent` 가 이미 쓰는 NavMesh 누적 경로 길이 히스테리시스로 완전히 폴백한다. 이 문서는 그 트리거 박스의 **크기**와 **언제 켤지**만 정한다.

---

## §1 확정 사실 — 입력으로만 쓰는 값

| 값 | 수치 | 출처 |
|---|---|---|
| `ArrivalHideDistanceCm`(폴백 경로의 숨김 임계, NavMesh 누적 경로 길이 기준) | 300cm | `SpyNavigationComponent.h:132`(코드 실측), mission-ground-navigation.md §4-3 |
| `ArrivalReshowDistanceCm`(폴백 경로의 재표시 임계) | 400cm | `SpyNavigationComponent.h:135`(코드 실측), 위 동일 |
| `StartOffsetDistanceCm`(라인 시작점 오프셋) | 100cm | `SpyNavigationComponent.h:129`(코드 실측) |
| `ASpyNPCCharacter::InteractionSphere` 반경 | 300cm | plan Task 3 Step 4 (`InteractionSphere->SetSphereRadius(300.f)`) |
| `ASpyInteractableObject::InteractionRadius`(기본값) | 300cm | plan Task 4 Step 3 (`float InteractionRadius = 300.f`) |
| 플레이어 캡슐 — 반경 / 절반 높이 | 42cm / 96cm(전고 ≈192cm) | `SpyCharacterConfig.h:16,19`(코드 실측) |
| DevMap 분기문 틈 폭(실측, 참고용) | 250cm | 메모리 `reference_devmap_coordinates.md` |
| plan 의 현재 placeholder Extent(Task 2/3/4 공통) | `FVector(200.f, 200.f, 100.f)` | plan Task 2 Step 5 / Task 3 Step 4 / Task 4 Step 4 — **이 문서가 대체한다** |

**⚠ 폴백 메커니즘 정정 — "거리"가 아니라 "NavMesh 누적 경로 길이"다.** `ArrivalHideDistanceCm`/`ArrivalReshowDistanceCm` 는 플레이어-타겟 직선 거리가 아니라 `RemainingPathLength`(NavMesh 로 계산한 우회 포함 누적 경로 길이, mission-ground-navigation.md §4-3)를 기준으로 판정한다. 즉 **벽 뒤·다른 층에 있는 목표는 폴백 경로가 이미 올바르게 "아직 멀다"로 처리한다.** 이 사실이 §3 의 트리거 사용 기준을 결정한다.

---

## §2 기본 Box Extent 결정

### 2-1. 판단 기준 — 히스테리시스가 없다는 것이 유일한 제약이다

폴백 경로(거리 히스테리시스)는 300~400cm 구간에서 상태를 유지하는 100cm 폭의 데드존이 있어, 그 구간 안에서 플레이어가 미세하게 움직여도 깜빡이지 않는다. **트리거 박스에는 이런 데드존이 없다** — 오버랩 진입/이탈이 그대로 숨김/표시로 직결된다(spec §6). 따라서 기본 Extent 를 정하는 유일한 실질적 제약은: **박스 경계면이 플레이어가 실제로 멈춰 서는 지점과 겹치지 않아야 한다.**

플레이어가 타겟 근처에서 멈춰 서는 지점은 이미 존재한다 — NPC/Interactable 의 상호작용 판정 반경(`InteractionSphere`/`InteractionRadius`, 둘 다 300cm)이다. 대화를 진행하거나 F 프롬프트를 보는 동안 플레이어는 이 반경 부근에서 미세하게 좌우로 움직인다(스트레이프·재조준). 박스 경계가 이 반경과 겹치면, 대화 한 번 하는 동안에도 라인이 깜빡일 수 있다.

### 2-2. 대안 비교

기준점은 X/Y 축(수평) half-extent다. 아래는 각 후보의 **축상(면) 거리**로 판정한다 — 모서리 대각선 거리가 아니라 축 방향 최소 거리로 판정해야 한다(예: half-extent 200 은 축상 200cm 지점에서 이미 경계를 벗어나지만 모서리는 283cm — 축상 거리가 더 이르게 실패한다).

| 안 | X/Y half-extent | 축상 경계 거리 | 판정 |
|---|---|---|---|
| A. 기존 placeholder | 200cm | 200cm | **기각** — 상호작용 반경(300cm)보다 안쪽이다. 플레이어가 정상적으로 F 상호작용을 시작하는 280cm 지점은 이 박스 **바깥**이라, 폴백이었다면 이미 숨겼을 상황(경로 길이 280 < 300)인데 트리거를 켜면 오히려 라인이 계속 보인다 — **트리거를 켜는 것이 안 켜는 것보다 UX 가 나빠지는 역효과.** 단순히 "작다"가 아니라 폴백보다 결과가 후퇴하는 구체적 실패다. |
| B. 250cm | 250cm | 250cm | 기각 — 상호작용 반경(300cm) **안쪽**이라 여전히 위 문제가 재현된다. |
| C. 300cm(`ArrivalHideDistanceCm` 그대로) | 300cm | 300cm | 기각 — 경계가 상호작용 반경과 **정확히 겹친다.** 대화 중 미세한 좌우 이동이 300cm 선을 그대로 넘나들어 §2-1 이 지목한 깜빡임이 가장 발생하기 쉬운 값이다. |
| **D. 400cm(`ArrivalReshowDistanceCm` 그대로) — 채택** | **400cm** | **400cm** | **채택 — 상호작용 반경(300cm)보다 100cm 바깥.** 이 100cm 여유는 임의값이 아니라 폴백 경로가 이미 쓰고 있는 데드존 폭(300→400cm)과 동일하다 — "상호작용 반경 + 기존 데드존 폭"으로 그대로 유도된다: `400 = 300(InteractionSphere/InteractionRadius) + 100(기존 히스테리시스 데드존 폭)`. |

**결정: X/Y half-extent = 400cm.** 근거를 다시 정리하면 — 트리거는 데드존이 없으므로, 폴백이 갖던 데드존과 같은 폭의 여유를 상호작용 반경 바깥에 확보해야 같은 수준의 깜빡임 내성을 얻는다. 400cm 는 이 여유를 기존 두 상수(`InteractionSphere`/`InteractionRadius` = 300, 히스테리시스 데드존 폭 = 100)로부터 그대로 유도한 값이지 새 매직 넘버가 아니다.

### 2-3. Z(수직) Extent

**결정: Z half-extent = 100cm (기존 placeholder 값 유지).**

검산 — 3개 타겟 액터 모두 `HideTriggerVolume` 이 액터 루트(피벗)에 부착되고, 이 프로젝트의 캐릭터/마커 배치 관례상 피벗은 지면 기준 캡슐 절반 높이(96cm) 부근에 온다(DevMap NPC/마커 배치 Z=96, 메모리 `reference_devmap_coordinates.md`). Z half-extent 100cm 이면 박스는 월드 기준 대략 `96 - 100 = -4cm` ~ `96 + 100 = 196cm` 를 덮는다 — 플레이어 전고(≈192cm, 캡슐 반높이 96cm×2)를 하단 4cm·상단 4cm 여유로 완전히 포함한다. (다만 오버랩 판정 자체는 캡슐과 박스가 조금이라도 겹치면 발동하므로, 이 계산은 "완전 포함"을 요구하는 물리 조건이 아니라 "여러 층을 인위적으로 덮으려 하지 않을 만큼 보수적인가"를 확인하는 안전 검산이다 — §3-3 의 "수직 분리는 트리거로 풀지 않는다"는 원칙과 짝을 이룬다.)

### 2-4. 최종 기본값 — 3개 클래스 공통

```
HideTriggerVolume->SetBoxExtent(FVector(400.f, 400.f, 100.f));
```

`ASpyMissionTargetPoint`(plan Task 2 Step 5) / `ASpyNPCCharacter`(Task 3 Step 4) / `ASpyInteractableObject`(Task 4 Step 4) 세 생성자 모두 이 값으로 통일한다 — plan 이 세 곳 모두 동일 placeholder(`200,200,100`)를 썼던 것과 같은 이유로, 세 타겟 타입에 규칙 차이를 두지 않는다(§2-1~§2-3 의 근거가 타입 무관 — 상호작용 반경 300cm 는 NPC/Interactable 공통값이고, `ASpyMissionTargetPoint` 는 상호작용 반경이 없지만 대칭성·일관된 디자이너 경험을 위해 동일 기본값을 쓴다).

---

## §3 디자이너 사용 가이드 — 트리거를 언제 켜는가

### 3-1. "복도·여러 층"은 그 자체로 트리거를 켤 이유가 아니다

§1 이 정정한 대로, 폴백은 **직선 거리가 아니라 NavMesh 누적 경로 길이**를 쓴다. 목표가 다른 층·벽 뒤에 있으면 경로 길이가 이미 커서 폴백이 정확히 "아직 멀다"로 처리한다 — 복도를 돌아가야 하거나 계단을 올라야 한다는 사실 자체는 폴백이 이미 반영하는 정보이지, 트리거가 추가로 필요한 이유가 아니다.

### 3-2. 트리거를 켜야 하는 경우 — 근거 있는 기준 둘

| 기준 | 설명 | 이 프로젝트의 사례 |
|---|---|---|
| **A. 진입 자체가 숨김 신호인 넓은 단일 진입 공간** | 목표가 방/구역 안쪽 깊숙이 있어서, "그 방에 들어온 순간"이 곧 도착 신호여야 하는데 방 안쪽까지의 경로 길이가 300cm 를 훨씬 초과해 폴백이 입장 즉시 숨기지 못하는 경우. 방에 들어와 있는데도 라인이 한동안 계속 보이는 것이 어색하다면 이 기준에 해당한다. | DevMap 분기 룸(사격장·평가장·격투장·장애물코스·클라이밍벽) 처럼 문으로 구획된 방 — 문을 통과하는 순간 숨기고 싶다면 트리거 후보. |
| **B. NavMesh 경로 자체가 정의되지 않거나 신뢰 불가능한 타겟** | 도보로 도달할 수 없어 NavMesh 가 경로를 만들 수 없는 타겟은 경로 길이 히스테리시스가 애초에 계산될 수 없다 — 폴백이 "항상 멀다" 또는 정의되지 않은 값을 낼 수 있다. 이 경우 트리거가 유일한 "도착" 판정 수단이다. | **현재 DevMap 에는 해당 사례가 없음.** `GrappleHook` 마커(`ASpyMissionTargetPoint`)는 Z=96·문 안쪽(분기 룸 내부)에 있어 도보로 도달 가능하다 — 메모리 `reference_devmap_coordinates.md` 가 미션 마커 6종(Kill/Level/Combo/Vault/WallClimb/GrappleHook) 전부를 동일하게 "Z=96, 문 안쪽, 지상 접근 가능"으로 기술하고, mission-ground-navigation.md §5-1/§9(M-nav-2) 도 GrappleHook 을 Vault/Climb 와 동일한 구역 미션으로 취급해 별도의 NavMesh 미도달 처리를 두지 않는다. `ExitPlatform` 은 같은 메모리가 "미션 시스템에 세션 종료 트리거가 없는 순수 연출 요소"로 명시하는 비-미션 액터라, 애초에 `HideTriggerVolume`/`bEnableHideTrigger` 가 존재하는 3개 타입(§헤더)에 속하지 않는다 — 기준 B 의 사례가 될 수 없다. 향후 그래플 전용(도보 경로가 없는) 미션 목표가 추가되면 그때 기준 B 의 구체 사례로 재검토한다. |

**⚠ A 기준 대상은 §2 기본값(400cm)을 그대로 켜면 오히려 역효과가 날 수 있다 — 마커-문 간격을 확인해야 한다.** 메모리 `reference_devmap_coordinates.md` 실측값 기준, 분기 룸 1(사격장)의 문은 Y=1600, `Kill` 마커는 Y=1800 — **마커가 문에서 방 안쪽으로 200cm 지점**에 있다(±900 보정 전후로 이 간격 자체는 불변). `ASpyMissionTargetPoint` 에 §2 기본값(half-extent 400cm)을 그대로 쓰면 박스 경계가 마커에서 `400 - 200 = 200cm` 만큼 문 **바깥**(복도 쪽)까지 번진다 — 문을 통과하기도 전에 라인이 숨는, A 기준이 노리는 것과 정반대 결과다. **A 기준으로 트리거를 켜는 모든 마커는 배치 시 마커-문 간격을 확인하고, 400cm 보다 좁으면 박스를 축소하거나 문 쪽으로 치우치게 중심을 옮겨 경계가 문 위치에 오도록 재배치한다(§3-4).** 이 수치는 문서 작성 시점 실측 기록 기준이며 라이브 맵 재조회로 재확인이 필요하다.

### 3-3. 트리거를 켜지 말아야 하는 경우

- **일반적인 접근형 타겟**(NPC 보고, 평범한 상호작용 오브젝트, 파쿠르 구역 진입 마커 등 — §3-2 의 A/B 어느 쪽에도 해당하지 않는 나머지 전부) — 기존 거리 히스테리시스로 이미 충분하다. 트리거를 켜면 배치·유지보수 비용(Box 위치/크기를 레벨 형상에 맞춰 계속 조정)만 늘고 §2-1 이 지목한 깜빡임 위험을 새로 떠안는다.
- **"여러 층 분리"를 이유로 켜지 않는다.** 트리거 박스는 층을 따라가지 않는 고정 로컬 볼륨이다 — Z 를 인위적으로 넓혀 "위아래 층을 한 박스로 덮으려" 하면, 그 아래층을 그냥 지나가는 플레이어에게도 오작동(엉뚱하게 라인이 숨음)한다. 수직 분리가 필요한 타겟은 **트리거를 끄고 경로 길이 폴백에 맡긴다** — §1 이 확인한 대로 폴백은 이미 층 간 경로 길이를 올바르게 반영한다. 이는 알려진 한계로 남기고(§5), 이 문서에서 새 메커니즘(예: Z 슬라이스, 여러 개 박스)을 설계하지 않는다.

### 3-4. 배치 원칙

박스 경계는 플레이어가 **의도를 갖고 통과하는 지점**(문턱, 계단 위, 구역 입구)에 두고, 플레이어가 **서성이는 지점**(상호작용 반경, 벽에 붙어 대기하는 지점)과 겹치지 않게 한다.

**축소 하한은 타입별로 다르다 — 상호작용 반경의 유무가 기준이다(§2-1).**

| 타입 | 축소 하한(half-extent) | 근거 |
|---|---|---|
| `ASpyNPCCharacter` / `ASpyInteractableObject` | **300cm** | §2-2 가 채택한 400cm 은 상호작용 반경(300cm) + 데드존 여유(100cm)다. 300cm 미만으로 줄이면 경계가 상호작용 반경 **안쪽**으로 들어와 §2-1 의 깜빡임(대화 중 미세 이동)이 재현된다. |
| `ASpyMissionTargetPoint` | **75cm**(전체 폭 150cm) | 이 타입은 상호작용 반경 자체가 없다(§2-4) — 300cm 하한의 근거가 성립하지 않는다. 적용되는 유일한 제약은 §4-3 의 물리적 최소 폭(캡슐 반경 42cm 대비 클리어런스)뿐이다. §3-2 A 기준으로 마커-문 간격이 400cm 보다 좁을 때(위 경고 참조) 이 하한까지 축소·재배치할 수 있다. |

Extent 를 위 표의 하한 미만으로는 줄이지 않는다 — NPC/Interactable 은 300cm, `ASpyMissionTargetPoint` 는 75cm 가 각각의 바닥이다.

---

## §4 UX 검수 기준

### 4-1. 검수 절차 — 반드시 이 순서로

1. **PIE 에서 실제로 오버랩이 발생하는지 먼저 확인한다.** `HideTriggerVolume` 은 `NoCollision` 으로 생성되고 `bEnableHideTrigger == true` 일 때만 `BeginPlay` 에서 `QueryOnly` 로 전환된다(spec §5). 콜리전 프로파일·오브젝트 타입 설정이 어긋나면 오버랩 자체가 발생하지 않아 "체크박스를 안 켠 것"과 똑같이 아무 일도 일어나지 않는다 — 이 침묵 실패는 mission-ground-navigation.md §2 가 지적한 "델리게이트가 원격 클라이언트에서 발화하지 않는" 문제와 같은 계열이다(원인은 다르지만 증상이 "아무 반응 없음"으로 동일해 발견이 늦어진다). **느낌을 판단하기 전에 오버랩 로그/디버그로 진입·이탈이 실제로 찍히는지부터 확인한다.**
2. 진입/이탈 시 라인이 즉시 반응하는지 확인한다(§4-2).
3. 박스 경계 근처에서 좌우로 배회하며 깜빡임을 관측한다(§4-3).
4. 방/구역에 들어간 직후 체감이 자연스러운지 확인한다(§3-2 A 기준 타겟).

### 4-2. 즉시 반응(0.75초 지연 없음)이 체감상 자연스러운가

트리거 활성 타겟은 spec §6 이 이미 확정한 대로 델리게이트 콜백에서 즉시 `HideVisual()`/`RecomputePath()` 를 호출한다(0.75초 `RecomputePath` 주기를 기다리지 않음). 이 문서는 그 결정을 재설계하지 않되, UX 로서 타당한지 판단하면 — **타당하다.** 트리거는 "지금 이 정의된 공간 안에 있다"는 이산적(discrete) 사실을 표현하는 장치이지 연속적인 거리 변화가 아니다. 거리 히스테리시스가 0.75초 주기로 갱신되는 이유(mission-ground-navigation.md §6 — NavMesh 동기 쿼리 비용)는 트리거에는 적용되지 않는다 — 오버랩 콜백은 이미 그 프레임에 발생한 이벤트이므로 지연시킬 이유가 없다. 오히려 지연을 두면 "이미 방 안에 들어와 있는데 라인이 잠깐 더 보이는" 어색함이 §3-2 A 가 해결하려는 문제를 다시 만든다.

### 4-3. Edge flicker 완화책 (디자이너가 배치 단계에서 쓸 수 있는 것만 — 코드 변경 없음)

- **축소 하한은 §3-4 표를 그대로 따른다 — NPC/Interactable 은 half-extent 300cm, `ASpyMissionTargetPoint` 는 75cm 가 바닥이다.** 물리적 최소 폭 검산은 `ASpyMissionTargetPoint` 하한(75cm)에서만 의미가 있다 — 캡슐 반경 42cm 기준 편도 클리어런스 `75 - 42 = 33cm`, 캡슐이 경계 바로 앞뒤로 몇 cm 만 움직여도 오버랩이 뒤집히는 "면도날" 폭은 피한다. NPC/Interactable 은 이보다 훨씬 여유 있는 300cm 가 하한이므로 별도 물리 검산이 필요 없다. §3-2 A 기준으로 `ASpyMissionTargetPoint` 를 DevMap 분기문 틈(250cm)처럼 좁은 지형에 맞춰 축소할 때도 75cm 미만으로는 내리지 않는다.
- **경계를 상호작용 반경(300cm)과 겹치지 않게 배치한다(NPC/Interactable 한정)** — §2/§3-4 가 이미 정한 원칙을 반복 확인한다.
- **경계를 "의도를 갖고 통과하는" 지점에 둔다** — 문턱·계단 위 등, 서성이는 지점과 분리한다(§3-4). `ASpyMissionTargetPoint` 를 §3-2 A 기준으로 쓸 때는 이 지점이 곧 "문"이다 — §3-2 표 하단 경고(마커-문 간격 확인)와 연결된다.
- **에스컬레이션 — 위 세 가지로도 플레이테스트에서 깜빡임이 관측되면, exit 판정에 짧은 지연(디바운스)을 추가하는 방안이 있다. 이는 이번 spec/plan 이 확정한 즉시 반응 아키텍처를 바꾸는 변경이라 이 문서에서 새로 설계하지 않는다 — mission-ground-navigation.md §7-5 선례와 동일하게 "plan 개정 필요" 항목으로만 등록한다(§5-2).**

---

## §5 알려진 한계 (이번 범위에서 해결하지 않음)

1. **수직 분리(여러 층)는 트리거로 해결하지 않는다** — §3-3 참조. 필요해지면 별도 검토(예: 층별 NavMesh 필터, Z 범위 판정 추가) 대상이며 이번 기획 범위 밖이다.
2. **Edge flicker 가 플레이테스트에서 실제로 관측되고 §4-3 디자이너 레버로 해소되지 않으면, exit 디바운스 도입은 plan 개정 필요 항목이다** — 이번 spec/plan 의 "즉시 반응" 아키텍처(spec §6)를 바꾸는 결정이므로 게임 디자이너 단독으로 선제 설계하지 않는다(YAGNI, mission-ground-navigation.md §7-5 와 동일 처리 원칙).

---

## §6 플레이테스트 메트릭

mission-ground-navigation.md 가 이미 `M-nav-1`~`M-nav-6` 을 점유하고 있으므로 이 문서는 별도 네임스페이스를 쓴다.

| 메트릭 | 관측 대상 | 판단 기준 |
|---|---|---|
| **M-navtrig-1** | §2 기본 Extent(400,400,100)가 §3-2 A 기준으로 실제 배치한 방(DevMap 분기 룸 등)에서 크기가 맞는지 | 방에 들어오는 순간과 박스 경계 진입 시점 사이 체감 지연이 있으면 Extent 를 방 형상에 맞춰 축소 — §3-4/§4-3 표의 타입별 하한(NPC/Interactable half-extent 300cm, `ASpyMissionTargetPoint` half-extent 75cm) 미만으로는 줄이지 않는다 |
| **M-navtrig-2** | 박스 경계 근처 배회 시 깜빡임 빈도(§4-1 절차 3) | 1회 배치·플레이테스트 세션당 관측된 깜빡임 0회가 목표. 발생 시 §4-3 레버부터 적용, 그래도 재발하면 §5-2 로 에스컬레이션 |
| **M-navtrig-3** | 즉시 반응(§4-2)과 기존 거리 히스테리시스(0.75초 주기) 전환 사이 체감 대비 — 같은 세션에서 트리거형 타겟과 폴백형 타겟을 연달아 경험했을 때 반응 속도 차이가 "일관성 없다"는 인상을 주는지 | 위화감이 보고되면 §4-2 논증(이산적 사실 vs 연속 거리)을 재검토 대상으로 삼는다 — 다만 이번 범위에서 선제 조정은 하지 않는다 |

---

## §7 구현 요청사항 (gameplay-programmer 용)

- **Gameplay Tag**: 해당 없음 — 이 기능은 신규 게임플레이 태그를 만들지 않는다. spec §4~§6·plan Task 1~5 전부 기존 `MatchTag`/`NPCId` 키 공간(`SpyGameplayTags`/`FSpyMissionRow`)을 그대로 재사용하고, 새 태그 카테고리를 요구하지 않는다.
- **C++ 인터페이스**: 신규 설계 없음 — `ISpyMissionTargetHideVolume`(`System/CommonInterface.System.h`)는 plan Task 2 Step 1 이 이미 확정했다. 이 문서가 추가하는 것은 그 구현체 3곳의 생성자 인자뿐이다:
  - `ASpyMissionTargetPoint::ASpyMissionTargetPoint()`(plan Task 2 Step 5) — `HideTriggerVolume->SetBoxExtent(FVector(200.f, 200.f, 100.f));` 를 **`HideTriggerVolume->SetBoxExtent(FVector(400.f, 400.f, 100.f));`** 로 교체.
  - `ASpyNPCCharacter::ASpyNPCCharacter(...)`(plan Task 3 Step 4) — 동일하게 `FVector(400.f, 400.f, 100.f)` 로 교체.
  - `ASpyInteractableObject::ASpyInteractableObject()`(plan Task 4 Step 4) — 동일하게 `FVector(400.f, 400.f, 100.f)` 로 교체.
  - 그 외 plan 이 정한 콜리전 프로파일·오버랩 델리게이트 배선(Task 2~5)은 변경하지 않는다.
- **DataAsset 스키마**: 해당 없음 — `bEnableHideTrigger`/Box Extent/회전은 레벨 배치 인스턴스마다 다르게 조정돼야 하는 **인스턴스 편집 값**(`EditAnywhere`, plan 이 이미 각 액터 헤더에 선언)이지, 반복 밸런스 로우(§14 DataTable 대상)도 정적 설정(§14 DataAsset 대상)도 아니다 — 이번 기능은 신규/수정 DataAsset·DataTable 필드가 없다.
- **GA·GE 명세**: 해당 없음 — 이 기능은 GAS(GameplayAbility/GameplayEffect/AttributeSet)를 전혀 사용하지 않는다. 순수 로컬 콜리전 오버랩 + 로컬 렌더링 표시/숨김이다(spec §2 제외 항목 — 서버 권한/레플리케이션 없음).

---

## §8 Self-Review

- **Placeholder 잔존**: 0건 — 수치 결정(Extent 400/400/100)에 대안별 판정 근거를 명시했고, "플레이테스트 확인 후 결정"으로 남긴 항목(§6 메트릭)도 판단 기준을 함께 제시했다.
- **스펙 커버리지**: spec §2(범위)=§헤더/§1, spec §3(레지스트리 API)=본 기획서 범위 외(사유: `USpyMissionTargetRegistrySubsystem` API 형태는 plan Task 1 이 이미 확정한 아키텍처 결정이라 게임 디자이너 도메인이 아님), spec §4(인터페이스)=본문 표기 정정만(§ 상단), spec §5(3개 액터 Extent)=§2, spec §6(즉시 반응 아키텍처)=§4-2, spec §7(테스트 가능 범위)=본 기획서 범위 외(사유: 테스트 케이스 설계는 test-engineer 도메인), spec §8(콜리전 프로파일 열린 질문)=plan 이 이미 해소(§7 에서 "변경하지 않음"으로 명시). 갭 없음(전부 매핑 또는 명시 제외).
- **내부 일관성**: 생성자 기본 Extent(400,400,100)가 §2/§6/§7 전체에서 동일. 디자이너 축소 하한은 §3-4 가 타입별로 명시 분리(NPC/Interactable=300cm, `ASpyMissionTargetPoint`=75cm)했고 §4-3 이 그 표를 그대로 참조 — 초판에서 §3-4(300cm 하한)와 §4-3(150cm 하한)이 같은 필드에 서로 다른 바닥을 주던 모순을 이 개정에서 타입별 스코프로 해소했다. `ArrivalHideDistanceCm`(300)/`ArrivalReshowDistanceCm`(400) 인용값이 §1/§2 에서 일치.
- **시그니처/명명 일관성**: `ISpyMissionTargetHideVolume` 로 전체 통일(그렙 확인 — `IMissionTargetHideVolume` 변형 표기는 §상단 정정 설명과 본 Self-Review 문구에서만 의도적으로 등장, 그 외 0건). `HideTriggerVolume`/`bEnableHideTrigger` 필드명도 plan 표기 그대로 유지.
- **모호 표현**: "적절히"/"상황에 맞게"류 표현 없음 — §3-4/§4-3 의 배치 원칙은 타입별로 구체 수치(NPC/Interactable 300cm, `ASpyMissionTargetPoint` 75cm)로 명시하며 두 절이 같은 표를 참조해 모순이 없다.
- **스코프**: 단일 결정(Extent 기본값 + 사용 가이드 + UX 검수)이며 구조 변경이 없어 별도 구현 단위 분할 불필요.
- **구현 요청사항 완전성**: Gameplay Tag/C++ 인터페이스/DataAsset/GA·GE 4항목 모두 "해당 없음 + 사유" 또는 구체 변경사항으로 채움 — 공란 없음.

**Self-Review: 3항목 보강 후 통과** — advisor 검토 반영: (1) §3-4/§4-3 축소 하한 모순(300cm vs 150cm)을 타입별 스코프(NPC/Interactable 300cm, `ASpyMissionTargetPoint` 75cm)로 재구성, (2) §3-2 A 기준 사례(DevMap 분기 룸)에 마커-문 간격 200cm 실측 근거로 기본값 400cm 가 문 밖까지 번지는 역효과 경고 추가, (3) 스펙 커버리지에서 누락됐던 spec §3·§7 을 "범위 외(사유)"로 명시.

**Self-Review 2차 (design-reviewer BLOCKER 반영) — 2항목 보강 후 통과**: (1) §6 `M-navtrig-1` 이 존재하지 않는 단일 "150cm 하한"을 인용하던 것을 §3-4/§4-3 이 실제로 정한 타입별 하한(NPC/Interactable 300cm, `ASpyMissionTargetPoint` 75cm)으로 정정, (2) §3-2 기준 B 사례가 근거 없이 `GrappleHook`/`ExitPlatform` 을 "그래플로만 도달"로 인용하던 것을 제거 — 인용 메모리(`reference_devmap_coordinates.md`) 는 `GrappleHook` 을 포함한 미션 마커 6종 전부를 지상 도보 접근 가능으로 기술하고 `ExitPlatform` 은 애초에 미션 타겟이 아니므로 이 기능 적용 대상 3개 액터 타입(§헤더)에 속하지 않는다 — "현재 DevMap 에는 해당 사례가 없음, 향후 재검토" 로 명시 치환. 기준 B 자체(NavMesh 미도달 타겟에서 트리거가 유일한 도착 판정 수단)는 유지.
