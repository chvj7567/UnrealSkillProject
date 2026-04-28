# Grappling Hook — Targeting UI & Highlight System Design

**Date:** 2026-04-27
**Branch:** feature/Grappling-Hook
**Phase:** 2 (Phase 1 = 기본 그래플링 훅, 완료)
**Approach:** USpyGrappleTargetingComponent + Delegate 구독

---

## Overview

기존 `USpyGA_GrappleHook`의 무조건 LineTrace 방식을 폐기하고, `GrappleAnchor` Actor Tag가 붙은 전용 앵커 액터만을 타겟으로 삼는 사전 타겟팅 시스템을 추가한다.

- **거리 + 화면 중앙 AND 조건** 충족 시에만 UI 프롬프트 + Highlight 표시
- 타겟 변경 이벤트는 `OnGrappleTargetChanged` Delegate로 전파 → UI/Highlight/GA가 구독
- 컴포넌트는 `SpyPawnExtensionComponent` 경유 동적 주입 (`SpyCharacter` 직접 의존 금지)

---

## Architecture

```
USpyGrappleTargetingComponent (C++ 신규)
│
├── TickComponent
│   ├── SphereOverlapActors(GrapplePromptRange)
│   │   오브젝트 타입: WorldStatic + WorldDynamic
│   ├── Actor Tag "GrappleAnchor" 필터
│   ├── ProjectWorldLocationToScreen → ViewportCenter 픽셀 거리 계산
│   ├── DistToCenter < GrappleTargetingScreenRadius → 유효 후보
│   └── 후보 중 DistToCenter 최솟값 선택 → CachedTarget 변경 시 Delegate 발행
│
├── OnGrappleTargetChanged (BlueprintAssignable)
│   ├── ① WBP_GrapplePrompt  — Visible / Hidden
│   ├── ② BP_SpyCharacter    — CustomDepth 토글 (Highlight)
│   └── ③ SpyGA_GrappleHook  — GetCurrentGrappleTarget() 직접 조회
│
├── Server_SetGrappleTarget (Server Reliable RPC)
│   └── 클라이언트 Tick에서 타겟 변경 감지 시 호출 → 서버 CurrentGrappleTarget 갱신
│
└── CurrentGrappleTarget (Replicated, 서버→클라이언트)
    └── GA가 서버에서 읽어 LaunchToTarget 호출
```

---

## 파일 목록

### 신규 C++ 파일

| 파일 | 역할 |
|---|---|
| `ManagerComponent/SpyGrappleTargetingComponent.h` | 컴포넌트 선언 |
| `ManagerComponent/SpyGrappleTargetingComponent.cpp` | 스캔 + Delegate + Replicated 캐시 |

### 수정 C++ 파일

| 파일 | 변경 내용 |
|---|---|
| `Data/SpyMovementConfig.h` | `GrapplePromptRange`, `GrappleTargetingScreenRadius` 필드 추가 |
| `AbilitySystem/Movement/SpyGA_GrappleHook.h/.cpp` | `TryLineTrace` 제거 → Component `GetCurrentGrappleTarget()` 조회로 교체 |

### 에디터 작업 (Blueprint / Asset)

| 에셋 | 변경 내용 |
|---|---|
| `SpyCharacterAssetData` DataAsset | `CommonComponentClasses`에 `SpyGrappleTargetingComponent` 추가 |
| `WBP_GrapplePrompt` (신규 Widget BP) | "E 키를 눌러 그래플링" 프롬프트 |
| `BP_SpyCharacter` (수정) | `OnGrappleTargetChanged` 구독 → CustomDepth 토글 + 위젯 표시/숨김 |
| `SpyMovementConfig` DataAsset | 신규 필드 값 입력 |
| `IMC_Default` | `IA_Grapple` → `Input.Ability.Skill.11` 바인딩 확인 |

> **주의:** `SpyCharacter.h/.cpp` 직접 수정 금지. 컴포넌트는 `SpyPawnExtensionComponent`가 `CharacterAssetData→GetAllComponentClasses()`를 읽어 `NewObject + RegisterComponent`로 동적 주입.

---

## Execution Flow

```
[매 Tick — 클라이언트]
  SpyGrappleTargetingComponent::TickComponent()
    SphereOverlap(GrapplePromptRange) → "GrappleAnchor" 태그 필터
    ProjectWorldLocationToScreen → DistToCenter 계산
    유효 후보 있음 → CachedTarget 갱신 + OnGrappleTargetChanged.Broadcast(Actor)
    유효 후보 없음 → CachedTarget = null + OnGrappleTargetChanged.Broadcast(nullptr)

[Delegate 수신 — 클라이언트]
  WBP_GrapplePrompt → SetVisibility(Visible / Hidden)
  BP_SpyCharacter   → OldTarget::SetRenderCustomDepth(false)
                    → NewTarget::SetRenderCustomDepth(true)

[Replicated — 서버]
  CurrentGrappleTarget 서버에 레플리케이트됨

[E 키 입력]
  GA::ActivateAbility()
    HasAuthority == false → return (LocalPredicted 패턴 유지)
    HasAuthority == true:
      Component = FindComponentByClass<USpyGrappleTargetingComponent>()
      Target = Component→GetCurrentGrappleTarget()
      Target == null → EndAbility(cancelled)
      Target 유효 → LaunchToTarget(Target→GetActorLocation())
                  → CableActor 스폰 + GrappleTick Task 시작
```

---

## 핵심 벡터 계산

### 스캔 — 화면 중앙 거리

```cpp
// 앵커 월드 좌표 → 스크린 좌표 변환
FVector2D ScreenPos;
PC->ProjectWorldLocationToScreen(AnchorLoc, ScreenPos, true);

// 뷰포트 중앙
FVector2D ViewportSize;
GEngine->GameViewport->GetViewportSize(ViewportSize);
FVector2D ViewportCenter = ViewportSize * 0.5f;

// 픽셀 거리
float DistToCenter = FVector2D::Distance(ScreenPos, ViewportCenter);

// 두 조건 AND
bool bInRange  = FVector::Dist(OwnerLoc, AnchorLoc) < GrapplePromptRange;
bool bOnScreen = DistToCenter < GrappleTargetingScreenRadius;
if (bInRange && bOnScreen) { /* 유효 후보 */ }
```

### 발사 — LaunchCharacter 포물선 (기존 유지)

```cpp
FVector Direction    = (TargetLoc - CharLoc).GetSafeNormal();
float   HorzDist     = FVector::Dist2D(CharLoc, TargetLoc);
float   Speed        = HorzDist / GrappleFlightTime;

FVector LaunchVel    = Direction * Speed;
LaunchVel.Z         += HorzDist * GrappleLaunchArcZScale;

Character->LaunchCharacter(LaunchVel, true, true);
```

---

## DataAsset 필드 추가 (SpyMovementConfig)

| 필드 | 타입 | 권장값 | 역할 |
|---|---|---|---|
| `GrapplePromptRange` | `float` | `1500.f` | SphereOverlap 반경 (UI 표시 거리) |
| `GrappleTargetingScreenRadius` | `float` | `150.f` | 화면 중앙 감지 픽셀 반경 |

기존 `GrappleMaxRange`(3000)은 GA 내 발사 유효성 검증용으로 유지.

---

## Delegate 구독 상세 (Blueprint)

```
BP_SpyCharacter::BeginPlay
  → GrappleComp = GetComponentByClass(SpyGrappleTargetingComponent)
  → GrappleComp.OnGrappleTargetChanged.AddDynamic(Self, HandleGrappleTargetChanged)

HandleGrappleTargetChanged(NewTarget):
  [NewTarget != null]
    OldTarget (캐시) → GetMesh → SetRenderCustomDepth(false)
    NewTarget        → GetMesh → SetRenderCustomDepth(true)
    WBP_GrapplePrompt → SetVisibility(Visible)
    OldTargetCache = NewTarget
  [NewTarget == null]
    OldTargetCache   → GetMesh → SetRenderCustomDepth(false)
    WBP_GrapplePrompt → SetVisibility(Hidden)
    OldTargetCache = null
```

---

## Multiplayer / Replication

| 작업 | 실행 주체 | 방법 |
|---|---|---|
| 스캔 + UI + Highlight | 로컬 클라이언트 | TickComponent (서버 불필요) |
| `CurrentGrappleTarget` 갱신 | 클라이언트→서버 | `Server_SetGrappleTarget` (Server Reliable RPC) |
| GA LaunchCharacter | 서버 | `HasAuthority()` 체크 후 Component에서 읽기 |
| CableActor 스폰 | 서버 | `bReplicates=true` Actor |

---

## Constraints

- `SpyCharacter` 직접 의존 금지 — 컴포넌트 동적 주입 패턴 준수
- `MovementConfig`는 `EditDefaultsOnly` 불가(동적 주입) → `BeginPlay`에서 `SpyAssetManager::LoadAssetSync`로 획득
- `GrappleAnchor` 마킹은 Actor Tag 방식 (`Tags` 배열에 `"GrappleAnchor"` 추가)
- `CurrentGrappleTarget`은 서버가 권위 보유 — 클라이언트 Tick에서 변경 감지 시 `Server_SetGrappleTarget` RPC 호출
- CustomDepth 활성화는 Post Process Volume에서 `Custom Depth-Stencil Pass = Enabled` 필요
- `WBP_GrapplePrompt`는 캐릭터 HUD에서 AddToViewport로 추가, Visibility로 토글
