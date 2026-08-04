# 미션 목표 바닥 길 안내 (Ground Path Navigation) — 설계

## 1. 배경 / 동기

기존 `docs/design/hud-mana-compass-skillbar.md`, `docs/design/npc-mission-dialogue.md` 는 "미션 목표까지의 방향/거리 안내"를 명시적으로 YAGNI 로 보류해 뒀다. 이번 작업은 그 보류 결정을 뒤집고, 미션 목표까지 **바닥에 그려지는 연속 글로우 라인**(Fable 스타일 골드 트레일)으로 안내하는 기능을 신설한다.

## 2. 범위

**포함**:
- 미션별 목표 월드 좌표 데이터 저작 (`Mission_TargetLocation` 관계 테이블).
- 미션 수락 시점을 알리는 신규 델리게이트(`OnMissionAccepted`) 추가.
- NavMesh 기반 경로 계산 + 바닥 스플라인 메시 라인 렌더링(로컬 클라이언트 전용 연출).
- 미션 수락~완료 동안 상시 표시, 주기적(기본 0.75초) 경로 재계산.

**제외 (이번 스코프 아님)**:
- NPC/목표지점 액터를 코드로 자동 탐색하는 위치 레지스트리 — 좌표는 레벨 디자이너가 `Mission_TargetLocation` 에 수동 입력한다. NPC 가 이동/리스폰되어 좌표와 실제 위치가 어긋나는 경우의 자동 보정은 다루지 않는다.
- 목표 지점이 정의되지 않은 Gameplay 미션(적 처치 N회 등)의 길 안내 — 기존처럼 HUD 텍스트만 표시.
- 다중 동시 활성 미션에 대한 다중 경로 표시 — `FSpyMissionState.MissionIndex` 가 가리키는 단일 활성 미션만 대상.
- 화면 가장자리 클램프형 방향 마커, 미니맵, 3D 공중 마커 등 다른 안내 형태 — 바닥 라인 하나로 확정.
- 서버/타 플레이어 동기화 — 순수 로컨트롤 클라이언트 연출이며 레플리케이트하지 않는다.

## 3. 데이터 — `Mission_TargetLocation` 관계 테이블 (신규)

```
FSpyMission_TargetLocationRow : public FTableRowBase
├── MissionId       (int32)   — FSpyMissionRow.MissionId 를 가리키는 FK
└── TargetLocation  (FVector) — 레벨 디자이너가 수동 입력하는 목표 월드 좌표
```

- **선택적 관계**(cpp-style §14-1) — 목표 지점이 있는 미션만 로우가 존재한다. 로우가 없으면 길 안내를 표시하지 않는다(sentinel 값 사용 금지).
- Dialogue/Interact 타입 미션도 기존 `NPCId` 조회 대신 이 좌표를 그대로 목적지로 사용한다. `NPCId` 는 기존 역할(HUD 이름 힌트 텍스트 `ResolveNPCNameHintText`)을 그대로 유지 — 이번 변경과 무관.
- `DA_SpyMissionConfig` 아래 `DT_Mission_TargetLocation` 으로 기존 DataTable 파이프라인에 편입, `SpyDataEditorTool` Config 탭에서 편집.

## 4. 델리게이트 — `USpyMissionComponent::OnMissionAccepted` (신규)

현재 `bAccepted` 가 false→true 로 바뀌는 두 지점 모두 범용 `OnMissionProgressChanged` 만 브로드캐스트한다 (`SpyMissionComponent.cpp`):

- `AcceptCurrentMission()` — 플레이어가 명시적으로 미션을 수락(Gameplay 타입, NPC 오퍼 카드 경로).
- `ProcessProgress()` 내부 — 새로 진입한 미션이 Dialogue/Interact 타입이면 자동 수락.

이 두 지점에 신규 `FSpyMission_Accepted` 델리게이트(`OnMissionAccepted(UActorComponent* Component, int32 MissionIndex)`) 브로드캐스트를 추가한다. 내비게이션 컴포넌트는 "진행값이 바뀔 때마다"가 아니라 "새 미션이 수락됐을 때"만 경로를 새로 잡아야 하므로, 기존 범용 델리게이트를 재사용하지 않고 전용 신호를 둔다.

## 5. 런타임 컴포넌트 — `USpyNavigationComponent` (신규 ManagerComponent)

- `USpyCharacterAssetData` 컴포넌트 목록에 등록해 기존 런타임 주입 패턴을 따른다(unreal-infra §2) — `BeginPlay` 하드코딩 금지.
- **로컬 컨트롤 클라이언트 전용**: 소유 폰이 `IsLocallyControlled()` 가 아니면 아무 것도 하지 않는다. 데디케이티드 서버에는 렌더링이 없으므로 서버에서는 동작하지 않는다. 서버 권한 로직이 필요 없는 순수 연출 컴포넌트.
- `USpyMissionComponent` 의 `OnMissionAccepted` / `OnMissionCompleted` / `OnAllMissionsCompleted` 델리게이트를 InitState 초기화(`InitAbilityActorInfo` 이후) 시점에 1회 구독.
- **시작**: `OnMissionAccepted` 수신 → 해당 `MissionIndex` 의 `MissionId` 로 `Mission_TargetLocation` 조회 → 로우가 있으면 목표 좌표 확정, 주기 타이머 시작.
- **갱신**: 고정 주기(`EditDefaultsOnly float UpdateIntervalSeconds`, 기본 0.75초)로 `UNavigationSystemV1::FindPathToLocationSynchronously(World, 소유자 현재 위치, TargetLocation)` 재계산 → 경로점 배열 갱신.
- **정지**: `OnMissionCompleted`(현재 활성 인덱스가 완료됐을 때) 또는 `OnAllMissionsCompleted` 수신 시 타이머 해제 + 스플라인 메시 정리. 다음 미션이 `Mission_TargetLocation` 로우가 없으면 정지 상태 유지.

## 6. 렌더링 — 스플라인 메시 라인

- `USplineComponent` 1개(컴포넌트 소유) + `USplineMeshComponent` 세그먼트 풀. 매 갱신마다 destroy/spawn 하지 않고 기존 세그먼트를 재사용(개수 변화분만 추가/제거)해 GC 압박을 줄인다.
- 글로우 머티리얼(인터페이스)은 `USpyAssetManager` 이름 룩업으로 참조한다 — 하드코딩 에셋 경로 금지(plugin-skassetcore.md §2). 실제 머티리얼 에셋 제작은 이번 스펙 범위 밖(아트 작업).

## 7. 테스트 가능 범위

NavMesh 통합 동작 자체는 Automation 환경에서 검증하기 어렵다. `SpyHUDMath` 선례(순수 함수 분리 → 유닛 테스트)를 따라 아래를 분리 가능한 순수 로직으로 만들고 테스트한다:
- "경로점(`TArray<FVector>`) → 스플라인 세그먼트 트랜스폼 배열" 변환 함수.
- `MissionId` → `Mission_TargetLocation` 로우 조회 결과(있음/없음) 판정 로직.
- `OnMissionAccepted` 신규 브로드캐스트 지점 회귀 테스트(수락 시 정확히 1회 발화, 진행값만 바뀔 때는 미발화).

## 8. 열린 질문 (구현 단계에서 확정)

- `USpyNavigationComponent` 가 `USpyMissionComponent` 를 구체 클래스로 직접 참조할지, 인터페이스로 분리할지는 기존 HUD 코드의 직접 참조 선례를 따를지 gameplay-programmer 단계에서 재확인한다.
