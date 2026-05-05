# DevMap 리뉴얼 — Design Spec

Date: 2026-05-05
Target file: `SkillProject/Content/Spy/Maps/DevMap.umap`
Author: chvj7567 + Claude

---

## 1. 배경 및 목적

### 1.1 현 상태

DevMap은 점진적으로 추가된 액터들이 한 평지 위에 흩어져 있는 상태:

- Floor 1개 (-1490, 0)
- Shape_Cube1/2/5/6 — 파쿠르용으로 추정되는 큐브 4개가 일렬로 배치 (높이가 모두 동일해 Vault만 검증 가능)
- PlayerStart 1개 (-3790, 0, 92)
- TargetPoint 2개 — AI 스폰 위치
- GrappleAnchor 3개 (-3200~-3400, z=450~500) — 거리·높이가 거의 같아 다양한 케이스 시연 불가
- 라이팅·스카이박스·NavMeshBoundsVolume

### 1.2 문제점

README에서 셀링포인트로 내세운 시스템들을 DevMap에서 시연·검증할 수 없음:

| 시스템 | DevMap 부족분 |
|---|---|
| 파쿠르 (Vault / WallClimb / HangUp) | 다양한 높이·두께의 장애물 부재 → 분기 조건 검증 불가 |
| 그래플링 훅 | 앵커가 좁은 영역에 몰려있음 → 거리·각도 다양성 시연 불가 |
| 콤보 | 콤보 더미가 없음 |
| 패링 | 정면 공격 AI가 없음 |
| AI 전투 (EQS CircleStrafe) | AI가 회피·전략적 위치 선정을 시연할 공간 없음 |
| 팀 시스템 | 아군·적군 동시 배치 케이스 없음 |
| SpawnBot | 동적 스폰 검증 영역 없음 |

### 1.3 리뉴얼 목적

**시스템별 시연 + 디버그 가능한 통합 테스트 맵**으로 재구성.
플레이어가 중앙 허브에서 시작해 7개의 기능별 구역으로 자유롭게 이동하며, 각 구역에서 단일 시스템을 독립적으로 검증할 수 있도록 한다.

### 1.4 비목표

- 시각적 폴리싱(머티리얼·라이팅·포스트프로세스 디테일) — 검증/시연 목적이므로 기본 큐브 + 머티리얼만 사용
- 게임플레이 모드(승패 조건·스코어) — DevMap은 디버그용
- 신규 C++ 코드 작성 — 기존 액터·컴포넌트만 사용해 맵 구성만 변경
- 별도 BP 신규 작성 — 기존 캐릭터·AI BP 재활용

---

## 2. 전체 레이아웃

플레이어가 중앙 허브에서 시작해 8방향으로 7개 구역에 접근할 수 있는 방사형 구조.

**좌표축 규약** (UE 표준):
- `+X` = Forward (다이어그램 위쪽으로 그림)
- `+Y` = Right (다이어그램 오른쪽으로 그림)
- 모든 좌표는 cm 단위
- 각 zone은 약 2000×2000 cm, zone 간격 약 3000cm
- 전체 맵 약 8000×8000 cm

```
                       +X (3500)
                          │
    [G] SpawnBot     [A] 파쿠르      [B] 그래플링
       (3000,-3000)    (3000,0)       (3000,3000)
            │              │              │
            └──────────────┼──────────────┘
                           │
  -Y ◄── [E] 패링 ── [H] 중앙 허브 ── [C] AI 전투 ──► +Y
       (-3000,0)         (0,0)          (0,3500)
                           │
            ┌──────────────┼──────────────┐
            │              │              │
    [D] 콤보 더미    (-X 방향 비움)   [F] 팀 시스템
       (-3000,-3000)                  (-3000,3000)
                          │
                       -X (-3500)
```

**바닥 처리**: 각 zone마다 별도 Floor StaticMeshActor 배치. zone 사이는 비워둠(공중 분리). 플레이어는 텔레포트 또는 비행으로 이동(테스트용 디버그 콘솔 사용 가정).

---

## 3. 각 구역 상세 구성

### 3.1 [H] 중앙 허브 — (0, 0)

**목적**: PlayerStart + 각 구역 안내.

| 액터 | 위치 (X, Y, Z) | 메시/속성 |
|---|---|---|
| Floor (10×10m) | (0, 0, 0) | `/Engine/BasicShapes/Cube`, Scale (10, 10, 0.1) |
| PlayerStart | (0, 0, 92) | — |
| 라벨 큐브 ×7 | 허브 가장자리, 각 zone 방향 | 작은 큐브 (Scale 1×1×3), 색별 머티리얼 |

라벨 큐브는 `+X` 방향이면 `[A] 파쿠르` 위치를 가리키도록 zone과 동일한 색의 마커.

### 3.2 [A] 파쿠르 코스 — (3000, 0)

**목적**: Vault / WallClimb / HangUp 분기 조건 + Depth 검출 검증.

zone 중심 (3000, 0). zone 내부에서 +X 방향으로 점진적으로 어려워지는 코스.

| 단계 | 위치 (X, Y, Z) | 크기 (cm) | 검증 GA |
|---|---|---|---|
| Vault 1 | (2300, 0, 40) | 100×400×80 | `GA_Vault` (낮은 장애물) |
| Vault 2 | (2600, 0, 50) | 100×400×100 | `GA_Vault` |
| Vault 3 | (2900, 0, 60) | 100×400×120 | `GA_Vault` (분기 임계 근처) |
| WallClimb 1 | (3300, 0, 125) | 100×400×250 | `GA_WallClimb` |
| WallClimb 2 | (3700, 0, 175) | 100×400×350 | `GA_WallClimb` |
| WallClimb 3 | (4100, 0, 225) | 100×400×450 | `GA_WallClimb` |
| HangUp 1 | (4600, 0, 250) | 100×400×500 | `GA_WallClimb` → `GA_HangUp` 자동 전환 |
| HangUp 2 | (5100, 0, 300) | 100×400×600 | `GA_HangUp` (상단 엣지) |
| Depth 50 | (3000, -800, 25) | 50×400×50 | 두께 50 → Depth 산출 검증 |
| Depth 200 | (3000, -1300, 25) | 200×400×50 | 두께 200 |
| Depth 400 | (3000, -1800, 25) | 400×400×50 | 두께 400 (Depth 임계 검증) |
| Floor | (3700, 0, 0) | 60×40×0.1 | zone 바닥 |

### 3.3 [B] 그래플링 타워 — (3000, 3000)

**목적**: GrappleAnchor 거리·각도·높이 분포에 따른 viewport 스캔 + 발사 검증.

zone 중심 (3000, 3000). 다양한 높이의 기둥 + 앵커.

| 액터 | 위치 (X, Y, Z) | 메시 | 비고 |
|---|---|---|---|
| Pillar 1 | (3500, 2500, 200) | Cube (Scale 0.5×0.5×4) | 높이 400 |
| Pillar 2 | (3800, 3000, 300) | Cube (Scale 0.5×0.5×6) | 높이 600 |
| Pillar 3 | (3500, 3500, 450) | Cube (Scale 0.5×0.5×9) | 높이 900 |
| Pillar 4 | (2700, 3000, 600) | Cube (Scale 0.5×0.5×12) | 높이 1200 |
| GrappleAnchor_Near_L | (2200, 2700, 400) | 작은 큐브 | 거리 ~800 (가까움, 좌측) |
| GrappleAnchor_Near_R | (2200, 3300, 400) | 작은 큐브 | 거리 ~800 (가까움, 우측) |
| GrappleAnchor_Mid_L | (3500, 2200, 600) | 작은 큐브 | 거리 ~1500 (중간, 좌측 위) |
| GrappleAnchor_Mid_R | (3500, 3800, 600) | 작은 큐브 | 거리 ~1500 (중간, 우측 위) |
| GrappleAnchor_Far_Up | (3800, 3000, 1200) | 작은 큐브 | 거리 ~2500 (위쪽 멀리) |
| GrappleAnchor_Far_Side | (4300, 3000, 800) | 작은 큐브 | 거리 ~1500 (정면 멀리) |
| GrappleAnchor_HighL | (2700, 2300, 1200) | 작은 큐브 | 좌측 위 |
| GrappleAnchor_HighR | (2700, 3700, 1200) | 작은 큐브 | 우측 위 |
| Floor | (3000, 3000, 0) | Cube (20×20×0.1) | zone 바닥 |

총 GrappleAnchor 8개, 거리 분포 800/1500/2500cm × 좌·우·위 각도.

### 3.4 [C] AI 전투 아레나 — (0, 3500)

**목적**: EQS CircleStrafe + BTTask_ActivateAbility 시연.

zone 중심 (0, 3500). 원형 평지 + 중앙 장애물.

| 액터 | 위치 (X, Y, Z) | 메시/속성 |
|---|---|---|
| Floor (원형 평지) | (0, 3500, 0) | Cube (Scale 30×30×0.1), 반경 ~1500 |
| 중앙 장애물 1 | (0, 3500, 100) | Cube (Scale 2×2×2), EQS 회피 평가용 |
| 중앙 장애물 2 | (300, 3300, 100) | Cube (Scale 1.5×1.5×2) |
| AI 스폰 TargetPoint A | (-1000, 3500, 50) | 둘레 1시 방향 |
| AI 스폰 TargetPoint B | (500, 4350, 50) | 둘레 5시 방향 |
| AI 스폰 TargetPoint C | (500, 2650, 50) | 둘레 9시 방향 |

**낭떠러지 검증**: zone Floor 바깥 1500cm는 비어있으므로 EQS가 추락 위치 회피하는지 검증 가능.

### 3.5 [D] 콤보 더미 링 — (-3000, -3000)

**목적**: SpyComboAssetData 기반 GA 체인 시연(3타+).

| 액터 | 위치 (X, Y, Z) | 메시/속성 |
|---|---|---|
| Floor | (-3000, -3000, 0) | Cube (Scale 20×20×0.1) |
| 더미 중앙 | (-3000, -3000, 90) | 기존 캐릭터 BP (TargetPoint 명명, AI 비활성 또는 무한 체력) |
| 더미 12시 | (-2500, -3000, 90) | 기존 캐릭터 BP |
| 더미 4시 | (-3300, -2500, 90) | 기존 캐릭터 BP |
| 더미 8시 | (-3300, -3500, 90) | 기존 캐릭터 BP |

**더미 처리 옵션**: 기존 AI 캐릭터를 배치하되 BehaviorTree 비활성으로 정적 더미 역할. 데미지는 받되 죽지 않게 임시 GE 적용 가능(테스트 BP에서 분기).

### 3.6 [E] 패링 더미 — (-3000, 0)

**목적**: 홀드형 패링 GA + Skill_Parry_Hit 이벤트 검증.

| 액터 | 위치 (X, Y, Z) | 메시/속성 |
|---|---|---|
| Floor | (-3000, 0, 0) | Cube (Scale 15×15×0.1) |
| 정면 공격 AI 1 | (-2500, -300, 90) | 기존 AI 캐릭터, 정면 공격 BT |
| 정면 공격 AI 2 | (-2500, 300, 90) | 기존 AI 캐릭터 |

좁은 공간(1500×1500)이라 AI가 멀리 떨어지지 않고 정면 공격을 일관되게 시도.

### 3.7 [F] 팀 시스템 구역 — (-3000, 3000)

**목적**: TeamId 0(아군) + TeamId 1(적군) 동시 배치, 데미지 분기 검증.

| 액터 | 위치 (X, Y, Z) | TeamId | 비고 |
|---|---|---|---|
| Floor | (-3000, 3000, 0) | — | Cube (Scale 20×20×0.1) |
| 아군 AI 1 | (-2700, 2700, 90) | 0 | 기존 AI BP |
| 아군 AI 2 | (-2700, 3300, 90) | 0 | |
| 적군 AI 1 | (-3300, 2700, 90) | 1 | |
| 적군 AI 2 | (-3300, 3300, 90) | 1 | |

**데이터 주입**: 각 AI의 `CharacterAssetData`에서 해당 캐릭터 엔트리의 `TeamId` 필드를 0 또는 1로 설정 (또는 별도 디버그 컴포넌트로 런타임 강제).

### 3.8 [G] SpawnBot 구역 — (3000, -3000)

**목적**: SpySpawnBotManagerComponent의 동적 스폰 검증.

| 액터 | 위치 (X, Y, Z) | 비고 |
|---|---|---|
| Floor | (3000, -3000, 0) | Cube (Scale 20×20×0.1) |
| SpawnBot 트리거 | (3000, -3000, 50) | 트리거 박스 또는 스폰 매니저 호스트 액터 |
| SpawnPoint TargetPoint A | (2400, -3000, 50) | 이름에 `TargetPoint` 포함 (memory 규약) |
| SpawnPoint TargetPoint B | (3600, -3000, 50) | |
| SpawnPoint TargetPoint C | (3000, -2400, 50) | |
| SpawnPoint TargetPoint D | (3000, -3600, 50) | |
| SpawnPoint TargetPoint E | (2700, -2700, 50) | |
| SpawnPoint TargetPoint F | (3300, -3300, 50) | |

스폰 매니저 컴포넌트는 zone 내 TargetPoint 액터들을 자동 수집하도록 기존 로직 활용.

---

## 4. NavMesh & 라이팅

### 4.1 NavMesh

기존 `NavMeshBoundsVolume`을 확장해 전체 맵(약 8000×8000 cm)을 커버하도록 변경.

- 위치: (0, 0, 0)
- BrushSize: BoxExtent 4500 × 4500 × 500

`RecastNavMesh`는 자동 빌드.

### 4.2 라이팅·스카이박스

기존 라이팅 액터(`DirectionalLight`, `SkyAtmosphere`, `SkyLight`, `ExponentialHeightFog`, `VolumetricCloud`, `SM_SkySphere`, `PostProcessVolume` ×2) 그대로 유지. 위치만 필요시 미세 조정.

### 4.3 시각 구분 (선택, YAGNI)

zone별 색깔 구분은 머티리얼 작업이 추가로 필요하므로 **1차에서는 skip**. 모두 기본 큐브 색 사용. 라벨 큐브만 위치로 식별.
시각 폴리싱은 별도 작업으로 분리.

---

## 5. 작업 흐름

1. 기존 DevMap 액터 **삭제 대상** 정리:
   - 보존: `WorldSettings1`, `Brush1`, `DirectionalLight`, `SkyAtmosphere`, `SkyLight`, `ExponentialHeightFog`, `VolumetricCloud`, `SM_SkySphere`, `DefaultPhysicsVolume0`, `GameplayDebuggerPlayerManager0`, `AbstractNavData-Default`, `RecastNavMesh-Default`, `PostProcessVolume`, `PostProcessVolume2`
   - 삭제: `Floor`, `Shape_Cube1/2/5/6`, `PlayerStart`, `TargetPoint1/2`, `GrappleAnchor_1/2/3`, `Actor` (정체 불명 빈 액터)
   - `NavMeshBoundsVolume`은 위치/크기 변경 (삭제·재생성보다 in-place 수정)
2. zone별 액터 스폰 — Unreal MCP `spawn_actor` + `set_actor_property`로 이름·위치·메시·스케일 설정
3. NavMesh 빌드 (자동)
4. 맵 저장
5. 에디터에서 PIE 실행 → 각 zone 시연 검증

---

## 6. 위험 & 미해결 이슈

| 이슈 | 대응 |
|---|---|
| 기존 더미 캐릭터 BP가 "죽지 않음" 모드를 지원하지 않으면 콤보 zone에서 즉사 | 1차에는 평범한 AI BP 사용, 별도 작업으로 무한 체력 GE 적용 |
| `TeamId` 런타임 강제 방법이 명확치 않음 | `CharacterAssetData` 데이터 분리(아군용·적군용 두 에셋)가 가장 깔끔. 본 spec 범위 밖 |
| Unreal MCP `spawn_actor`가 StaticMeshActor의 메시·스케일·머티리얼을 한 번에 설정 못 할 수 있음 | spawn 후 `set_actor_property`로 단계별 설정 |
| 기존 Brush1(BSP)이 무엇인지 불명 | 그대로 두고 검증, 문제 시 후속 정리 |
| 8000×8000 NavMesh 빌드 시간 증가 | 데스크톱 PIE에서는 무시 가능. 빌드 후 ETA 확인 |

---

## 7. 인수 기준 (Acceptance Criteria)

- [ ] PlayerStart에서 시작해 7개 zone 전부 도달 가능
- [ ] [A]에서 Vault → WallClimb → HangUp 단계가 순차로 발동
- [ ] [B]에서 viewport 중앙에 잡히는 GrappleAnchor가 거리·각도별로 다양함을 확인
- [ ] [C]에서 AI가 EQS 기반으로 좌/우 회피하면서 어빌리티 발동
- [ ] [D]에서 콤보 3타 이상이 더미 대상으로 발동
- [ ] [E]에서 정면 공격 AI에 대해 패링 윈도우 동작
- [ ] [F]에서 아군 AI에는 데미지가 들어가지 않고 적군 AI에만 들어감
- [ ] [G]에서 SpawnBot 매니저가 TargetPoint를 자동 수집해 스폰
- [ ] NavMesh가 모든 zone Floor에 표시됨
