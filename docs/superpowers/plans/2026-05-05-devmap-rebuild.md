# DevMap 리뉴얼 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** `SkillProject/Content/Spy/Maps/DevMap.umap`을 7개 기능별 zone(파쿠르 / 그래플링 / AI 전투 / 콤보 / 패링 / 팀 / SpawnBot) + 중앙 허브 구조로 재구성해 README 셀링포인트 시스템들을 독립적으로 시연·디버그할 수 있게 한다.

**Architecture:** 단일 `.umap` 파일을 Unreal MCP(`unreal-mcp/server.py` + RemoteControl 브리지) 도구로 원격 편집. 각 Task에서 액터를 spawn / 속성 설정 → 끝에 `save_asset`로 디스크 반영. 신규 C++/BP 작성 없음, 기존 액터·BP만 재배치. NavMesh는 Python 스크립트로 BoxExtent 변경.

**Tech Stack:** Unreal Engine 5.7 에디터, Unreal MCP 도구(`spawn_actor` / `delete_actor` / `set_actor_property` / `execute_python` / `save_asset`), `/Engine/BasicShapes/Cube.Cube` 메시.

**Spec:** `docs/superpowers/specs/2026-05-05-devmap-rebuild-design.md`

**Commit policy:** 본 plan의 각 Task 끝에 Stage step이 있더라도 **사용자가 명시적으로 커밋을 요청한 경우에만 실행**. 그 전까지는 변경만 stage하고 commit은 보류.

**Worktree policy:** 메인 워크트리(`C:/Users/Tae/Desktop/MyProject/SpyProject/UnrealSkillProject`, `main` 브랜치)에서 작업. 자동 생성된 워크트리에서 작업 금지(memory: `feedback_no_claude_branch.md`).

---

## File Structure

| 파일 | 작업 | 설명 |
|---|---|---|
| `SkillProject/Content/Spy/Maps/DevMap.umap` | **수정** (액터 재구성) | 액터 삭제 + 신규 spawn + 속성 변경. 바이너리 자산이라 Edit/Diff 불가 → MCP 호출 시퀀스로 추적. |

**작성 전략:** Task별로 zone 단위 작업 + 끝에서 `save_asset` 호출. 에디터 크래시 시 손실 최소화 + git history도 task 단위로 의미 있게 분리 가능. 모든 변경은 메인 워크트리 main 브랜치에 stage.

**MCP 클래스 경로 규약:**
- StaticMeshActor: `/Script/Engine.StaticMeshActor`
- PlayerStart: `/Script/Engine.PlayerStart`
- TargetPoint: `/Script/Engine.TargetPoint`
- BP 캐릭터/기존 GrappleAnchor 액터: `/Game/Spy/Blueprints/...` (Task 0에서 실재 경로 확인)

---

## Task 0: 사전 점검 (MCP 연결 + 베이스라인 + 클래스 경로 확정)

**Files:** 없음 (정보 수집만)

- [ ] **Step 1: 메인 워크트리 git 상태 확인**

Run from main worktree:
```bash
git -C "C:/Users/Tae/Desktop/MyProject/SpyProject/UnrealSkillProject" status --short docs/superpowers/specs/
```
Expected: `A docs/superpowers/specs/2026-05-05-devmap-rebuild-design.md` (이미 stage됨).

- [ ] **Step 2: 현재 DevMap 액터 베이스라인 캡처**

Call: `mcp__unreal-mcp__get_actors_in_level`
Expected: 27개 액터 — `WorldSettings1`, `Brush1`, `Floor`, `DirectionalLight`, `SkyAtmosphere`, `SkyLight`, `ExponentialHeightFog`, `VolumetricCloud`, `SM_SkySphere`, `PlayerStart`, `Shape_Cube1`, `Shape_Cube6`, `NavMeshBoundsVolume`, `RecastNavMesh-Default`, `TargetPoint1`, `TargetPoint2`, `Shape_Cube2`, `Shape_Cube5`, `PostProcessVolume`, `PostProcessVolume2`, `GrappleAnchor_1`, `GrappleAnchor_2`, `GrappleAnchor_3`, `DefaultPhysicsVolume0`, `GameplayDebuggerPlayerManager0`, `Actor`, `AbstractNavData-Default`.

베이스라인이 다르면 spec § 1.1과 비교해 차이점을 사용자에게 보고하고 plan 진행 여부 확인.

- [ ] **Step 3: 기존 GrappleAnchor 클래스 경로 확인**

Call:
```
mcp__unreal-mcp__execute_python(script="""
import unreal
actor = unreal.EditorLevelLibrary.get_actor_reference('GrappleAnchor_1')
if actor:
    print('Class path:', actor.get_class().get_path_name())
    print('Mesh:', actor.static_mesh_component.static_mesh.get_path_name() if actor.static_mesh_component.static_mesh else 'None')
""")
```
Expected: 클래스 경로(예: `/Script/Engine.StaticMeshActor` 또는 BP 경로) + 메시 경로 출력.

이 경로는 Task 6(그래플링 타워)에서 새 GrappleAnchor 스폰할 때 그대로 사용. BP라면 `/Game/...` 경로를 그대로 `class_path`에 전달.

- [ ] **Step 4: AI 캐릭터 BP 경로 확인**

Call:
```
mcp__unreal-mcp__list_assets(path="/Game/Spy/Blueprints/Character", class_name="Blueprint")
```
Expected: AI 캐릭터 BP 경로 목록 (예: `/Game/Spy/Blueprints/Character/BP_AICharacter`). Task 8/9/10/11에서 BP class_path로 사용.

복수 결과면 사용자에게 어떤 BP를 더미·정면공격 AI·아군·적군에 쓸지 확인.

- [ ] **Step 5: 베이스라인 메모**

다음 정보를 plan 작업 메모로 기록:
- GrappleAnchor 클래스 경로: `__________`
- AI 캐릭터 BP 경로: `__________`
- 기존 NavMeshBoundsVolume의 BoxExtent: `__________` (`execute_python`으로 `nav.brush.brush_builder` 조회)

---

## Task 1: 기존 액터 정리 (cleanup)

**Files:** `SkillProject/Content/Spy/Maps/DevMap.umap` (수정)

**삭제 대상**: spec § 5의 삭제 리스트.

- [ ] **Step 1: Floor 삭제**

Call: `mcp__unreal-mcp__delete_actor(actor_name="Floor")`
Expected: 성공 응답.

- [ ] **Step 2: 파쿠르 큐브 4개 삭제**

Calls (순차):
- `delete_actor(actor_name="Shape_Cube1")`
- `delete_actor(actor_name="Shape_Cube2")`
- `delete_actor(actor_name="Shape_Cube5")`
- `delete_actor(actor_name="Shape_Cube6")`

- [ ] **Step 3: PlayerStart 삭제**

Call: `delete_actor(actor_name="PlayerStart")`
Expected: 성공. (Task 4에서 (0,0,92)에 새로 스폰)

- [ ] **Step 4: TargetPoint 2개 삭제**

Calls:
- `delete_actor(actor_name="TargetPoint1")`
- `delete_actor(actor_name="TargetPoint2")`

- [ ] **Step 5: 기존 GrappleAnchor 3개 삭제**

Calls:
- `delete_actor(actor_name="GrappleAnchor_1")`
- `delete_actor(actor_name="GrappleAnchor_2")`
- `delete_actor(actor_name="GrappleAnchor_3")`

- [ ] **Step 6: 정체불명 빈 액터 "Actor" 삭제**

Call: `delete_actor(actor_name="Actor")`
실패해도 무시(이미 정리됐을 수 있음).

- [ ] **Step 7: 정리 검증**

Call: `mcp__unreal-mcp__get_actors_in_level()`
Expected: 27 - 12 = 15개 액터. 보존 목록(`WorldSettings1`, `Brush1`, `DirectionalLight`, `SkyAtmosphere`, `SkyLight`, `ExponentialHeightFog`, `VolumetricCloud`, `SM_SkySphere`, `NavMeshBoundsVolume`, `RecastNavMesh-Default`, `PostProcessVolume`, `PostProcessVolume2`, `DefaultPhysicsVolume0`, `GameplayDebuggerPlayerManager0`, `AbstractNavData-Default`)만 남아있어야 함.

- [ ] **Step 8: DevMap 저장**

Call: `mcp__unreal-mcp__save_asset(asset_path="/Game/Spy/Maps/DevMap")`
Expected: 성공.

- [ ] **Step 9: Stage (commit 보류)**

Run from main worktree:
```bash
git -C "C:/Users/Tae/Desktop/MyProject/SpyProject/UnrealSkillProject" add SkillProject/Content/Spy/Maps/DevMap.umap
git -C "C:/Users/Tae/Desktop/MyProject/SpyProject/UnrealSkillProject" status --short SkillProject/Content/Spy/Maps/
```
Expected: `M SkillProject/Content/Spy/Maps/DevMap.umap` 표시.

제안 커밋 메시지: `[Map] DevMap — 기존 흩어진 액터 정리(파쿠르 큐브·PlayerStart·TargetPoint·GrappleAnchor)`. 사용자 명시 요청 시에만 commit 실행.

---

## Task 2: NavMeshBoundsVolume 확장 (8000×8000)

**Files:** `SkillProject/Content/Spy/Maps/DevMap.umap` (수정)

전체 맵을 커버하도록 NavMesh bounds를 9000×9000(여유 500cm) × 1000으로 확장. set_actor_property로는 BoxExtent를 직접 변경하기 어려워 Python 사용.

- [ ] **Step 1: NavMeshBoundsVolume 위치 (0,0,0)으로 이동 + 확장**

Call:
```
mcp__unreal-mcp__execute_python(script="""
import unreal

nav_actors = unreal.EditorLevelLibrary.get_all_level_actors_of_class(unreal.NavMeshBoundsVolume)
if not nav_actors:
    print('ERROR: NavMeshBoundsVolume not found')
else:
    nav = nav_actors[0]
    nav.set_actor_location(unreal.Vector(0.0, 0.0, 0.0), False, False)

    builder = nav.brush.brush_builder
    builder.x = 9000.0
    builder.y = 9000.0
    builder.z = 1000.0
    builder.build(nav.brush)

    nav.brush.modify()
    nav.modify()
    unreal.EditorLevelLibrary.editor_invalidate_viewports()

    nav_bounds = nav.get_actor_bounds(False)
    print('NavMesh new origin:', nav.get_actor_location())
    print('NavMesh new extent:', nav_bounds[1])
""")
```
Expected: origin (0,0,0), extent ≈ (4500, 4500, 500).

- [ ] **Step 2: NavMesh 빌드 트리거**

Call:
```
mcp__unreal-mcp__execute_python(script="""
import unreal
unreal.EditorLevelLibrary.set_level_dirty()
unreal.SystemLibrary.execute_console_command(None, 'RebuildNavigation')
print('Navigation rebuild triggered')
""")
```
Expected: 출력 후 에디터 좌하단 'Building Paths...' 진행. (큰 NavMesh는 5-30초 소요 가능)

- [ ] **Step 3: 저장 + Stage**

Call: `save_asset(asset_path="/Game/Spy/Maps/DevMap")`

```bash
git -C "C:/Users/Tae/Desktop/MyProject/SpyProject/UnrealSkillProject" add SkillProject/Content/Spy/Maps/DevMap.umap
```

제안 커밋 메시지: `[Map] DevMap — NavMeshBoundsVolume 8000×8000으로 확장`.

---

## Task 3: 헬퍼 함수 정의 (Python 모듈, 후속 Task에서 재사용)

**Files:** 임시 메모리 함수 (영구 파일 작성 X)

후속 zone에서 반복적으로 사용할 헬퍼 — "큐브 spawn → 메시 + 위치 + 스케일 설정"을 한 번에 처리. `execute_python`으로 함수 정의 후 같은 세션에서 재호출.

- [ ] **Step 1: 헬퍼 함수 등록**

Call:
```
mcp__unreal-mcp__execute_python(script="""
import unreal

EDITOR = unreal.EditorActorSubsystem()
ASSET_REG = unreal.AssetRegistryHelpers.get_asset_registry()

CUBE_MESH = unreal.EditorAssetLibrary.load_asset('/Engine/BasicShapes/Cube.Cube')

def spawn_cube(name, x, y, z, sx=1.0, sy=1.0, sz=1.0):
    loc = unreal.Vector(float(x), float(y), float(z))
    rot = unreal.Rotator(0, 0, 0)
    actor = EDITOR.spawn_actor_from_class(unreal.StaticMeshActor, loc, rot)
    if actor is None:
        print(f'FAIL spawn_cube({name})')
        return None
    actor.set_actor_label(name)
    sm_comp = actor.static_mesh_component
    sm_comp.set_static_mesh(CUBE_MESH)
    actor.set_actor_scale3d(unreal.Vector(float(sx), float(sy), float(sz)))
    sm_comp.set_mobility(unreal.ComponentMobility.STATIC)
    return actor

def spawn_target_point(name, x, y, z):
    loc = unreal.Vector(float(x), float(y), float(z))
    actor = EDITOR.spawn_actor_from_class(unreal.TargetPoint, loc, unreal.Rotator(0,0,0))
    if actor is None:
        print(f'FAIL spawn_target_point({name})')
        return None
    actor.set_actor_label(name)
    return actor

# 전역 등록
unreal.SystemLibrary.print_string(None, 'Helpers registered: spawn_cube, spawn_target_point', text_color=unreal.LinearColor(0,1,0,1), duration=2.0)
print('OK helpers registered')
""")
```
Expected: 'OK helpers registered'.

**주의:** Unreal Editor의 Python은 `execute_python` 호출마다 새 모듈 컨텍스트일 수 있음. 후속 Task의 Python 호출에서 함수가 사라져 있으면 같은 헬퍼 블록을 매번 prefix로 붙여야 함. 후속 Task의 코드 블록에는 헬퍼 정의를 함께 포함시켰음.

---

## Task 4: [H] 중앙 허브 구성

**Files:** `SkillProject/Content/Spy/Maps/DevMap.umap` (수정)

좌표: (0, 0). spec § 3.1 참조.

- [ ] **Step 1: 허브 Floor + PlayerStart spawn**

Call:
```
mcp__unreal-mcp__execute_python(script="""
import unreal
EDITOR = unreal.EditorActorSubsystem()
CUBE_MESH = unreal.EditorAssetLibrary.load_asset('/Engine/BasicShapes/Cube.Cube')

def spawn_cube(name, x, y, z, sx=1.0, sy=1.0, sz=1.0):
    actor = EDITOR.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(float(x),float(y),float(z)), unreal.Rotator(0,0,0))
    actor.set_actor_label(name)
    sm = actor.static_mesh_component
    sm.set_static_mesh(CUBE_MESH)
    actor.set_actor_scale3d(unreal.Vector(float(sx),float(sy),float(sz)))
    sm.set_mobility(unreal.ComponentMobility.STATIC)
    return actor

# Floor
spawn_cube('Hub_Floor', 0, 0, -50, 10, 10, 1)  # z=-50으로 두면 윗면이 z=0

# PlayerStart
ps = EDITOR.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0,0,92), unreal.Rotator(0,0,0))
ps.set_actor_label('PlayerStart')

print('Hub: Floor + PlayerStart OK')
""")
```
Expected: `Hub: Floor + PlayerStart OK`.

- [ ] **Step 2: 라벨 큐브 7개 spawn (zone 방향 마커)**

Call:
```
mcp__unreal-mcp__execute_python(script="""
import unreal
EDITOR = unreal.EditorActorSubsystem()
CUBE_MESH = unreal.EditorAssetLibrary.load_asset('/Engine/BasicShapes/Cube.Cube')

def spawn_cube(name, x, y, z, sx=1.0, sy=1.0, sz=1.0):
    actor = EDITOR.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(float(x),float(y),float(z)), unreal.Rotator(0,0,0))
    actor.set_actor_label(name)
    sm = actor.static_mesh_component
    sm.set_static_mesh(CUBE_MESH)
    actor.set_actor_scale3d(unreal.Vector(float(sx),float(sy),float(sz)))
    sm.set_mobility(unreal.ComponentMobility.STATIC)
    return actor

# 라벨 큐브: 허브 가장자리 (반경 ~450) + 각 zone 방향
labels = [
    ('Label_A_Parkour',    400,    0, 150),  # +X
    ('Label_B_Grapple',    400,  400, 150),  # +X+Y
    ('Label_C_AIArena',      0,  400, 150),  # +Y
    ('Label_F_Team',      -400,  400, 150),  # -X+Y
    ('Label_E_Parry',     -400,    0, 150),  # -X
    ('Label_D_Combo',     -400, -400, 150),  # -X-Y
    ('Label_G_SpawnBot',   400, -400, 150),  # +X-Y
]
for name, x, y, z in labels:
    spawn_cube(name, x, y, z, 1, 1, 3)

print(f'Hub labels OK: {len(labels)} cubes')
""")
```
Expected: `Hub labels OK: 7 cubes`.

- [ ] **Step 3: 검증**

Call: `get_actors_in_level(name_filter="Hub_")` + `get_actors_in_level(name_filter="Label_")` + `get_actors_in_level(name_filter="PlayerStart")`
Expected: 각각 1개, 7개, 1개.

- [ ] **Step 4: 저장 + Stage**

`save_asset(asset_path="/Game/Spy/Maps/DevMap")`
```bash
git -C "C:/Users/Tae/Desktop/MyProject/SpyProject/UnrealSkillProject" add SkillProject/Content/Spy/Maps/DevMap.umap
```
제안 커밋 메시지: `[Map] DevMap — 중앙 허브 (Floor + PlayerStart + 7개 zone 라벨) 구성`.

---

## Task 5: [A] 파쿠르 코스 — (3000, 0)

**Files:** `SkillProject/Content/Spy/Maps/DevMap.umap` (수정)

spec § 3.2 참조. zone 바닥 + Vault 3 + WallClimb 3 + HangUp 2 + Depth 검증 3.

- [ ] **Step 1: 파쿠르 Floor + Vault 3개 spawn**

Call:
```
mcp__unreal-mcp__execute_python(script="""
import unreal
EDITOR = unreal.EditorActorSubsystem()
CUBE_MESH = unreal.EditorAssetLibrary.load_asset('/Engine/BasicShapes/Cube.Cube')

def spawn_cube(name, x, y, z, sx=1.0, sy=1.0, sz=1.0):
    actor = EDITOR.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(float(x),float(y),float(z)), unreal.Rotator(0,0,0))
    actor.set_actor_label(name)
    sm = actor.static_mesh_component
    sm.set_static_mesh(CUBE_MESH)
    actor.set_actor_scale3d(unreal.Vector(float(sx),float(sy),float(sz)))
    sm.set_mobility(unreal.ComponentMobility.STATIC)
    return actor

# Floor
spawn_cube('Parkour_Floor', 3700, 0, -50, 60, 40, 1)

# Vault 1/2/3 (높이 80/100/120, 두께 100, 폭 400)
spawn_cube('Parkour_Vault_1', 2300, 0, 40, 1, 4, 0.8)
spawn_cube('Parkour_Vault_2', 2600, 0, 50, 1, 4, 1.0)
spawn_cube('Parkour_Vault_3', 2900, 0, 60, 1, 4, 1.2)

print('Parkour Floor + Vault 1-3 OK')
""")
```
Expected: `Parkour Floor + Vault 1-3 OK`.

- [ ] **Step 2: WallClimb 3개 + HangUp 2개 spawn**

Call:
```
mcp__unreal-mcp__execute_python(script="""
import unreal
EDITOR = unreal.EditorActorSubsystem()
CUBE_MESH = unreal.EditorAssetLibrary.load_asset('/Engine/BasicShapes/Cube.Cube')

def spawn_cube(name, x, y, z, sx=1.0, sy=1.0, sz=1.0):
    actor = EDITOR.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(float(x),float(y),float(z)), unreal.Rotator(0,0,0))
    actor.set_actor_label(name)
    sm = actor.static_mesh_component
    sm.set_static_mesh(CUBE_MESH)
    actor.set_actor_scale3d(unreal.Vector(float(sx),float(sy),float(sz)))
    sm.set_mobility(unreal.ComponentMobility.STATIC)
    return actor

# WallClimb 1/2/3 (높이 250/350/450)
spawn_cube('Parkour_WallClimb_1', 3300, 0, 125, 1, 4, 2.5)
spawn_cube('Parkour_WallClimb_2', 3700, 0, 175, 1, 4, 3.5)
spawn_cube('Parkour_WallClimb_3', 4100, 0, 225, 1, 4, 4.5)

# HangUp 1/2 (높이 500/600)
spawn_cube('Parkour_HangUp_1', 4600, 0, 250, 1, 4, 5.0)
spawn_cube('Parkour_HangUp_2', 5100, 0, 300, 1, 4, 6.0)

print('Parkour WallClimb 1-3 + HangUp 1-2 OK')
""")
```
Expected: `Parkour WallClimb 1-3 + HangUp 1-2 OK`.

- [ ] **Step 3: Depth 검증 큐브 3개 spawn (-Y 방향)**

Call:
```
mcp__unreal-mcp__execute_python(script="""
import unreal
EDITOR = unreal.EditorActorSubsystem()
CUBE_MESH = unreal.EditorAssetLibrary.load_asset('/Engine/BasicShapes/Cube.Cube')

def spawn_cube(name, x, y, z, sx=1.0, sy=1.0, sz=1.0):
    actor = EDITOR.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(float(x),float(y),float(z)), unreal.Rotator(0,0,0))
    actor.set_actor_label(name)
    sm = actor.static_mesh_component
    sm.set_static_mesh(CUBE_MESH)
    actor.set_actor_scale3d(unreal.Vector(float(sx),float(sy),float(sz)))
    sm.set_mobility(unreal.ComponentMobility.STATIC)
    return actor

# Depth 검증 (두께 50/200/400, 폭 400, 높이 50)
spawn_cube('Parkour_Depth_50',  3000,  -800, 25, 0.5, 4, 0.5)
spawn_cube('Parkour_Depth_200', 3000, -1300, 25, 2.0, 4, 0.5)
spawn_cube('Parkour_Depth_400', 3000, -1800, 25, 4.0, 4, 0.5)

print('Parkour Depth 50/200/400 OK')
""")
```
Expected: `Parkour Depth 50/200/400 OK`.

- [ ] **Step 4: 검증**

Call: `get_actors_in_level(name_filter="Parkour_")`
Expected: 12개 (`Floor`, `Vault_1/2/3`, `WallClimb_1/2/3`, `HangUp_1/2`, `Depth_50/200/400`).

- [ ] **Step 5: 저장 + Stage**

`save_asset(asset_path="/Game/Spy/Maps/DevMap")`
```bash
git -C "C:/Users/Tae/Desktop/MyProject/SpyProject/UnrealSkillProject" add SkillProject/Content/Spy/Maps/DevMap.umap
```
제안 커밋 메시지: `[Map] DevMap — [A] 파쿠르 코스(Vault·WallClimb·HangUp·Depth) 추가`.

---

## Task 6: [B] 그래플링 타워 — (3000, 3000)

**Files:** `SkillProject/Content/Spy/Maps/DevMap.umap` (수정)

spec § 3.3 참조. Floor + 기둥 4 + GrappleAnchor 8.

**참고:** Task 0에서 확인한 GrappleAnchor 클래스 경로를 사용. 만약 BP라면 `unreal.EditorAssetLibrary.load_blueprint_class('/Game/...')`로 로드 후 `spawn_actor_from_class`. 여기 plan에서는 단순화를 위해 StaticMeshActor + 작은 큐브로 구현 (그래플링 타겟팅 컴포넌트의 스캔 조건에 메시 크기·태그가 영향을 주는 경우 Task 0 결과에 맞춰 스크립트 조정).

- [ ] **Step 1: 그래플링 Floor + 기둥 4개**

Call:
```
mcp__unreal-mcp__execute_python(script="""
import unreal
EDITOR = unreal.EditorActorSubsystem()
CUBE_MESH = unreal.EditorAssetLibrary.load_asset('/Engine/BasicShapes/Cube.Cube')

def spawn_cube(name, x, y, z, sx=1.0, sy=1.0, sz=1.0):
    actor = EDITOR.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(float(x),float(y),float(z)), unreal.Rotator(0,0,0))
    actor.set_actor_label(name)
    sm = actor.static_mesh_component
    sm.set_static_mesh(CUBE_MESH)
    actor.set_actor_scale3d(unreal.Vector(float(sx),float(sy),float(sz)))
    sm.set_mobility(unreal.ComponentMobility.STATIC)
    return actor

# Floor
spawn_cube('Grapple_Floor', 3000, 3000, -50, 20, 20, 1)

# 기둥 4개
spawn_cube('Grapple_Pillar_1', 3500, 2500, 200, 0.5, 0.5, 4)
spawn_cube('Grapple_Pillar_2', 3800, 3000, 300, 0.5, 0.5, 6)
spawn_cube('Grapple_Pillar_3', 3500, 3500, 450, 0.5, 0.5, 9)
spawn_cube('Grapple_Pillar_4', 2700, 3000, 600, 0.5, 0.5, 12)

print('Grapple Floor + Pillars 1-4 OK')
""")
```
Expected: `Grapple Floor + Pillars 1-4 OK`.

- [ ] **Step 2: GrappleAnchor 8개 spawn**

**클래스 경로 결정:**
- Task 0 Step 3에서 `/Script/Engine.StaticMeshActor`였다면 → 아래 스크립트 그대로
- BP였다면(예: `/Game/Spy/Blueprints/GrappleAnchor.GrappleAnchor_C`) → 스크립트의 `unreal.StaticMeshActor` 부분을 BP 클래스 로드로 교체:
  ```python
  bp = unreal.EditorAssetLibrary.load_blueprint_class('/Game/Spy/Blueprints/GrappleAnchor')
  actor = EDITOR.spawn_actor_from_class(bp, loc, rot)
  ```

Call (StaticMeshActor 가정):
```
mcp__unreal-mcp__execute_python(script="""
import unreal
EDITOR = unreal.EditorActorSubsystem()
CUBE_MESH = unreal.EditorAssetLibrary.load_asset('/Engine/BasicShapes/Cube.Cube')

def spawn_cube(name, x, y, z, sx=1.0, sy=1.0, sz=1.0):
    actor = EDITOR.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(float(x),float(y),float(z)), unreal.Rotator(0,0,0))
    actor.set_actor_label(name)
    sm = actor.static_mesh_component
    sm.set_static_mesh(CUBE_MESH)
    actor.set_actor_scale3d(unreal.Vector(float(sx),float(sy),float(sz)))
    sm.set_mobility(unreal.ComponentMobility.STATIC)
    return actor

# GrappleAnchor 8개 (작은 큐브 0.3 스케일)
anchors = [
    ('GrappleAnchor_Near_L',    2200, 2700,  400),
    ('GrappleAnchor_Near_R',    2200, 3300,  400),
    ('GrappleAnchor_Mid_L',     3500, 2200,  600),
    ('GrappleAnchor_Mid_R',     3500, 3800,  600),
    ('GrappleAnchor_Far_Up',    3800, 3000, 1200),
    ('GrappleAnchor_Far_Side',  4300, 3000,  800),
    ('GrappleAnchor_HighL',     2700, 2300, 1200),
    ('GrappleAnchor_HighR',     2700, 3700, 1200),
]
for name, x, y, z in anchors:
    spawn_cube(name, x, y, z, 0.3, 0.3, 0.3)

print(f'GrappleAnchors OK: {len(anchors)}')
""")
```
Expected: `GrappleAnchors OK: 8`.

**Note:** `SpyGrappleTargetingComponent`가 액터 태그(예: `GrappleAnchor`)나 BP 클래스 IsA로 후보를 스캔한다면 단순 StaticMeshActor는 인식되지 않을 수 있다. Task 0 Step 3에서 확인한 기존 `GrappleAnchor_1`의 태그·컴포넌트를 그대로 복제해야 정확한 시연 가능. 코드 차원에서 컴포넌트 검증 후 boilerplate 보강은 본 plan 범위 외 — 필요시 별도 plan으로 분리.

- [ ] **Step 3: 검증**

Call: `get_actors_in_level(name_filter="Grapple_")` + `get_actors_in_level(name_filter="GrappleAnchor_")`
Expected: 5개(Floor + Pillar 4) + 8개.

- [ ] **Step 4: 저장 + Stage**

`save_asset(asset_path="/Game/Spy/Maps/DevMap")`
```bash
git -C "C:/Users/Tae/Desktop/MyProject/SpyProject/UnrealSkillProject" add SkillProject/Content/Spy/Maps/DevMap.umap
```
제안 커밋 메시지: `[Map] DevMap — [B] 그래플링 타워(기둥 4 + GrappleAnchor 8) 추가`.

---

## Task 7: [C] AI 전투 아레나 — (0, 3500)

**Files:** `SkillProject/Content/Spy/Maps/DevMap.umap` (수정)

spec § 3.4 참조. 원형 평지 + 중앙 장애물 2 + 둘레 TargetPoint 3.

- [ ] **Step 1: 원형 Floor + 중앙 장애물**

Call:
```
mcp__unreal-mcp__execute_python(script="""
import unreal
EDITOR = unreal.EditorActorSubsystem()
CUBE_MESH = unreal.EditorAssetLibrary.load_asset('/Engine/BasicShapes/Cube.Cube')

def spawn_cube(name, x, y, z, sx=1.0, sy=1.0, sz=1.0):
    actor = EDITOR.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(float(x),float(y),float(z)), unreal.Rotator(0,0,0))
    actor.set_actor_label(name)
    sm = actor.static_mesh_component
    sm.set_static_mesh(CUBE_MESH)
    actor.set_actor_scale3d(unreal.Vector(float(sx),float(sy),float(sz)))
    sm.set_mobility(unreal.ComponentMobility.STATIC)
    return actor

# Floor (실제 원형은 BP 필요 — 1차에선 큰 사각 평지로 대체. 반경 ~1500)
spawn_cube('AIArena_Floor', 0, 3500, -50, 30, 30, 1)

# 중앙 장애물 2개
spawn_cube('AIArena_Obstacle_1',   0, 3500, 100, 2.0, 2.0, 2.0)
spawn_cube('AIArena_Obstacle_2', 300, 3300, 100, 1.5, 1.5, 2.0)

print('AIArena Floor + Obstacles OK')
""")
```
Expected: `AIArena Floor + Obstacles OK`.

- [ ] **Step 2: AI 스폰 TargetPoint 3개**

Call:
```
mcp__unreal-mcp__execute_python(script="""
import unreal
EDITOR = unreal.EditorActorSubsystem()

def spawn_target_point(name, x, y, z):
    actor = EDITOR.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(float(x),float(y),float(z)), unreal.Rotator(0,0,0))
    actor.set_actor_label(name)
    return actor

# AI 스폰 위치 (이름에 'TargetPoint' 포함 필수 — memory: project_ai_spawn.md)
spawn_target_point('AIArena_TargetPoint_A', -1000, 3500, 50)
spawn_target_point('AIArena_TargetPoint_B',   500, 4350, 50)
spawn_target_point('AIArena_TargetPoint_C',   500, 2650, 50)

print('AIArena TargetPoints OK')
""")
```
Expected: `AIArena TargetPoints OK`.

- [ ] **Step 3: 검증**

Call: `get_actors_in_level(name_filter="AIArena_")`
Expected: 6개 (Floor + Obstacle ×2 + TargetPoint ×3).

- [ ] **Step 4: 저장 + Stage**

`save_asset(asset_path="/Game/Spy/Maps/DevMap")`
```bash
git -C "C:/Users/Tae/Desktop/MyProject/SpyProject/UnrealSkillProject" add SkillProject/Content/Spy/Maps/DevMap.umap
```
제안 커밋 메시지: `[Map] DevMap — [C] AI 전투 아레나(중앙 장애물 + 둘레 TargetPoint 3) 추가`.

---

## Task 8: [D] 콤보 더미 링 — (-3000, -3000)

**Files:** `SkillProject/Content/Spy/Maps/DevMap.umap` (수정)

spec § 3.5 참조. **변경 사항:** 사용자 결정에 따라 정적 더미 캐릭터 배치 대신 TargetPoint 4개만 배치(memory `project_ai_spawn.md` 패턴 — SpawnBotManager가 PIE 시점에 spawn). 더미용 무한 체력 GE는 별도 plan으로.

- [ ] **Step 1: 콤보 Floor + TargetPoint 4개**

Call:
```
mcp__unreal-mcp__execute_python(script="""
import unreal
EDITOR = unreal.EditorActorSubsystem()
CUBE_MESH = unreal.EditorAssetLibrary.load_asset('/Engine/BasicShapes/Cube.Cube')

def spawn_cube(name, x, y, z, sx=1.0, sy=1.0, sz=1.0):
    actor = EDITOR.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(float(x),float(y),float(z)), unreal.Rotator(0,0,0))
    actor.set_actor_label(name)
    sm = actor.static_mesh_component
    sm.set_static_mesh(CUBE_MESH)
    actor.set_actor_scale3d(unreal.Vector(float(sx),float(sy),float(sz)))
    sm.set_mobility(unreal.ComponentMobility.STATIC)
    return actor

def spawn_target_point(name, x, y, z):
    actor = EDITOR.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(float(x),float(y),float(z)), unreal.Rotator(0,0,0))
    actor.set_actor_label(name)
    return actor

spawn_cube('Combo_Floor', -3000, -3000, -50, 20, 20, 1)

points = [
    ('Combo_TargetPoint_Center', -3000, -3000, 50),
    ('Combo_TargetPoint_12',     -2500, -3000, 50),
    ('Combo_TargetPoint_4',      -3300, -2500, 50),
    ('Combo_TargetPoint_8',      -3300, -3500, 50),
]
for name, x, y, z in points:
    spawn_target_point(name, x, y, z)
""")
```

- [ ] **Step 2: 검증**

Call: `get_actors_in_level(name_filter="Combo_")`
Expected: 5개 (Floor + TargetPoint ×4).

- [ ] **Step 3: 저장 + Stage**

`save_asset(asset_path="/Game/Spy/Maps/DevMap")`
```bash
git -C "C:/Users/Tae/Desktop/MyProject/SpyProject/UnrealSkillProject" add SkillProject/Content/Spy/Maps/DevMap.umap
```
제안 커밋 메시지: `[Map] DevMap — [D] 콤보 zone(Floor + TargetPoint 4) 추가`.

---

## Task 9: [E] 패링 zone — (-3000, 0)

**Files:** `SkillProject/Content/Spy/Maps/DevMap.umap` (수정)

spec § 3.6 참조. **변경 사항:** AI 캐릭터 직접 배치 대신 TargetPoint 2개 배치.

- [ ] **Step 1: 패링 Floor + TargetPoint 2개**

Call:
```
mcp__unreal-mcp__execute_python(script="""
import unreal
EDITOR = unreal.EditorActorSubsystem()
CUBE_MESH = unreal.EditorAssetLibrary.load_asset('/Engine/BasicShapes/Cube.Cube')

def spawn_cube(name, x, y, z, sx=1.0, sy=1.0, sz=1.0):
    actor = EDITOR.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(float(x),float(y),float(z)), unreal.Rotator(0,0,0))
    actor.set_actor_label(name)
    sm = actor.static_mesh_component
    sm.set_static_mesh(CUBE_MESH)
    actor.set_actor_scale3d(unreal.Vector(float(sx),float(sy),float(sz)))
    sm.set_mobility(unreal.ComponentMobility.STATIC)
    return actor

def spawn_target_point(name, x, y, z):
    actor = EDITOR.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(float(x),float(y),float(z)), unreal.Rotator(0,0,0))
    actor.set_actor_label(name)
    return actor

spawn_cube('Parry_Floor', -3000, 0, -50, 15, 15, 1)

points = [
    ('Parry_TargetPoint_1', -2500, -300, 50),
    ('Parry_TargetPoint_2', -2500,  300, 50),
]
for name, x, y, z in points:
    spawn_target_point(name, x, y, z)
""")
```

- [ ] **Step 2: 검증**

Call: `get_actors_in_level(name_filter="Parry_")`
Expected: 3개.

- [ ] **Step 3: 저장 + Stage**

`save_asset(asset_path="/Game/Spy/Maps/DevMap")`
```bash
git -C "C:/Users/Tae/Desktop/MyProject/SpyProject/UnrealSkillProject" add SkillProject/Content/Spy/Maps/DevMap.umap
```
제안 커밋 메시지: `[Map] DevMap — [E] 패링 zone(Floor + TargetPoint 2) 추가`.

---

## Task 10: [F] 팀 시스템 zone — (-3000, 3000)

**Files:** `SkillProject/Content/Spy/Maps/DevMap.umap` (수정)

spec § 3.7 참조. **변경 사항:** AI 캐릭터 직접 배치 대신 TargetPoint 4개 배치(Ally/Enemy 이름 분기). TeamId 주입은 후속 plan으로.

- [ ] **Step 1: 팀 Floor + TargetPoint 4개**

Call:
```
mcp__unreal-mcp__execute_python(script="""
import unreal
EDITOR = unreal.EditorActorSubsystem()
CUBE_MESH = unreal.EditorAssetLibrary.load_asset('/Engine/BasicShapes/Cube.Cube')

def spawn_cube(name, x, y, z, sx=1.0, sy=1.0, sz=1.0):
    actor = EDITOR.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(float(x),float(y),float(z)), unreal.Rotator(0,0,0))
    actor.set_actor_label(name)
    sm = actor.static_mesh_component
    sm.set_static_mesh(CUBE_MESH)
    actor.set_actor_scale3d(unreal.Vector(float(sx),float(sy),float(sz)))
    sm.set_mobility(unreal.ComponentMobility.STATIC)
    return actor

def spawn_target_point(name, x, y, z):
    actor = EDITOR.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(float(x),float(y),float(z)), unreal.Rotator(0,0,0))
    actor.set_actor_label(name)
    return actor

spawn_cube('Team_Floor', -3000, 3000, -50, 20, 20, 1)

points = [
    # 아군 (TeamId 0 예정) — 좌측 라인
    ('Team_Ally_TargetPoint_1',  -2700, 2700, 50),
    ('Team_Ally_TargetPoint_2',  -2700, 3300, 50),
    # 적군 (TeamId 1 예정) — 우측 라인
    ('Team_Enemy_TargetPoint_1', -3300, 2700, 50),
    ('Team_Enemy_TargetPoint_2', -3300, 3300, 50),
]
for name, x, y, z in points:
    spawn_target_point(name, x, y, z)
""")
```

- [ ] **Step 2: 검증**

Call: `get_actors_in_level(name_filter="Team_")`
Expected: 5개.

- [ ] **Step 3: 저장 + Stage**

`save_asset(asset_path="/Game/Spy/Maps/DevMap")`
```bash
git -C "C:/Users/Tae/Desktop/MyProject/SpyProject/UnrealSkillProject" add SkillProject/Content/Spy/Maps/DevMap.umap
```
제안 커밋 메시지: `[Map] DevMap — [F] 팀 zone(Floor + Ally/Enemy TargetPoint 4) 추가`.

---

## Task 11: [G] SpawnBot 구역 — (3000, -3000)

**Files:** `SkillProject/Content/Spy/Maps/DevMap.umap` (수정)

spec § 3.8 참조. Floor + 트리거 박스 + TargetPoint 6개.

- [ ] **Step 1: Floor + TargetPoint 6개**

Call:
```
mcp__unreal-mcp__execute_python(script="""
import unreal
EDITOR = unreal.EditorActorSubsystem()
CUBE_MESH = unreal.EditorAssetLibrary.load_asset('/Engine/BasicShapes/Cube.Cube')

def spawn_cube(name, x, y, z, sx=1.0, sy=1.0, sz=1.0):
    actor = EDITOR.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(float(x),float(y),float(z)), unreal.Rotator(0,0,0))
    actor.set_actor_label(name)
    sm = actor.static_mesh_component
    sm.set_static_mesh(CUBE_MESH)
    actor.set_actor_scale3d(unreal.Vector(float(sx),float(sy),float(sz)))
    sm.set_mobility(unreal.ComponentMobility.STATIC)
    return actor

def spawn_target_point(name, x, y, z):
    actor = EDITOR.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(float(x),float(y),float(z)), unreal.Rotator(0,0,0))
    actor.set_actor_label(name)
    return actor

spawn_cube('SpawnBot_Floor', 3000, -3000, -50, 20, 20, 1)

# 스폰 위치 (이름에 'TargetPoint' 포함)
points = [
    ('SpawnBot_TargetPoint_A', 2400, -3000, 50),
    ('SpawnBot_TargetPoint_B', 3600, -3000, 50),
    ('SpawnBot_TargetPoint_C', 3000, -2400, 50),
    ('SpawnBot_TargetPoint_D', 3000, -3600, 50),
    ('SpawnBot_TargetPoint_E', 2700, -2700, 50),
    ('SpawnBot_TargetPoint_F', 3300, -3300, 50),
]
for name, x, y, z in points:
    spawn_target_point(name, x, y, z)
print(f'SpawnBot zone OK: Floor + {len(points)} TargetPoints')
""")
```
Expected: `SpawnBot zone OK: Floor + 6 TargetPoints`.

**SpawnBot 트리거/매니저 호스트 액터:** `SpySpawnBotManagerComponent`를 가진 액터(BP)가 있다면 그 BP 경로를 Task 0 Step 4와 동일 방식으로 확인 후 zone 중심에 추가 spawn. 본 plan에선 1차로 TargetPoint만 배치하고, 매니저 액터는 별도 Task로 분리(클래스 명세를 모르므로).

- [ ] **Step 2: 검증**

Call: `get_actors_in_level(name_filter="SpawnBot_")`
Expected: 7개.

- [ ] **Step 3: 저장 + Stage**

`save_asset(asset_path="/Game/Spy/Maps/DevMap")`
```bash
git -C "C:/Users/Tae/Desktop/MyProject/SpyProject/UnrealSkillProject" add SkillProject/Content/Spy/Maps/DevMap.umap
```
제안 커밋 메시지: `[Map] DevMap — [G] SpawnBot zone(TargetPoint 6) 추가`.

---

## Task 12: 최종 검증 (PIE)

**Files:** 없음 (검증만)

- [ ] **Step 1: 전체 액터 카운트**

Call: `mcp__unreal-mcp__get_actors_in_level()`
Expected:
- 보존 15 + 허브(Floor + PlayerStart + Label×7) 9 + 파쿠르 12 + 그래플링 13 + AI 아레나 6 + 콤보 5 + 패링 3 + 팀 5 + SpawnBot 7 = **75개**

±2 정도 오차는 NavMesh 보조 액터 등으로 발생 가능. 큰 차이가 있으면 누락된 zone 점검.

- [ ] **Step 2: NavMesh 빌드 상태 확인**

에디터 좌하단의 'Building Paths...' 인디케이터가 사라졌는지 확인. 사라졌으면 모든 zone Floor에 RecastNavMesh 그린 메시가 표시되어야 함(에디터 P 키로 NavMesh 표시 토글).

- [ ] **Step 3: 인수 기준 PIE 시연 (사용자 직접 검증)**

에디터에서 PIE 실행 후 아래 항목 수동 확인 — spec § 7 인수 기준:

```
[ ] PlayerStart에서 시작해 7개 zone 전부 도달 가능 (디버그 콘솔 'ghost' 또는 'fly' 활용)
[ ] [A] Vault → WallClimb → HangUp 단계 순차 발동
[ ] [B] viewport 중앙 GrappleAnchor가 거리·각도별로 다양함을 확인
[ ] [C] AI가 EQS 기반으로 좌/우 회피하면서 어빌리티 발동 (TargetPoint에서 AI 자동 스폰 가정)
[ ] [D] 콤보 3타 이상 더미 대상 발동
[ ] [E] 정면 공격 AI에 대해 패링 윈도우 동작
[ ] [F] 아군 AI vs 적군 AI 데미지 분기 (※ TeamId 미주입 시 후속 작업 필요)
[ ] [G] SpawnBot 매니저가 TargetPoint 자동 수집해 스폰 (※ 매니저 액터 미배치 시 후속 작업 필요)
[ ] NavMesh가 모든 zone Floor에 표시
```

- [ ] **Step 4: 최종 stage 상태 확인**

Run from main worktree:
```bash
git -C "C:/Users/Tae/Desktop/MyProject/SpyProject/UnrealSkillProject" status --short
```
Expected: `M SkillProject/Content/Spy/Maps/DevMap.umap` + `A docs/superpowers/specs/2026-05-05-devmap-rebuild-design.md` + `A docs/superpowers/plans/2026-05-05-devmap-rebuild.md`.

- [ ] **Step 5: 통합 커밋 메시지 제안 (사용자 명시 요청 시에만 commit)**

```
[Map] DevMap — 7개 zone 리뉴얼 (파쿠르·그래플링·AI전투·콤보·패링·팀·SpawnBot)

- 기존 흩어진 큐브·앵커·TargetPoint 정리, NavMesh 8000×8000 확장
- 중앙 허브(PlayerStart) + 7개 zone(시연·디버그용 액터 배치)
- spec docs/superpowers/specs/2026-05-05-devmap-rebuild-design.md
- plan docs/superpowers/plans/2026-05-05-devmap-rebuild.md
```

또는 zone별로 분리 커밋(Task 1~11 각각)을 선호하면 각 Task의 stage 시점에 commit. 사용자 결정.

---

## 후속 작업 (본 plan 범위 외)

spec § 6 위험 & 미해결 이슈에서 도출:

1. **콤보 더미 무한 체력 GE** — `Combo_Dummy_*` 액터에 `Tag.IsDummy` 또는 `GE_InfiniteHealth` 적용. AI BP 분리 또는 런타임 GE 적용 로직 필요.
2. **TeamId 주입** — `Team_Ally_*`, `Team_Enemy_*` 액터의 `CharacterAssetData` 분리(아군용·적군용 두 에셋). spec § 6 권장.
3. **GrappleAnchor BP 검증** — Task 0 결과에 따라 단순 큐브 대신 기존 GrappleAnchor BP를 spawn해야 그래플링 컴포넌트가 인식.
4. **SpawnBotManager 호스트 액터 배치** — `SpySpawnBotManagerComponent`를 가진 BP를 SpawnBot zone 중심에 spawn.
5. **시각 폴리싱** — zone별 색상 머티리얼 적용(spec § 4.3 — YAGNI로 생략).
