# Grappling Hook System Design

**Date:** 2026-04-27  
**Branch:** feature/Grappling-Hook  
**Approach:** GA Single Class + AbilityTask Tick

---

## Overview

직선 견인형 그래플링 훅 시스템. 카메라 정면 Line Trace로 목표 지점을 감지하고, LaunchCharacter로 포물선 이동 후 도착 시 자동 종료.

---

## Architecture

### 신규 파일

| 파일 | 역할 |
|---|---|
| `SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyGA_GrappleHook.h/.cpp` | 그래플링 훅 GA 본체 |
| `SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyAbilityTask_GrappleTick.h/.cpp` | 매 프레임 거리 체크 + 케이블 위치 갱신 |
| `SkillProject/Source/SkillProject/Character/AGrappleCableActor.h/.cpp` | 케이블 시각화 Replicated Actor |

### 수정 파일

| 파일 | 변경 내용 |
|---|---|
| `SpyMovementConfig` DataAsset 클래스 | 그래플 설정 필드 4개 추가 |
| `SpyGameplayTags.h/.cpp` | `Character_State_Grapple` 태그 추가 |

---

## Execution Flow

```
[입력] → GA ActivateAbility()
   ↓
[클라이언트→서버] Server_TryGrapple() RPC
   ↓
[서버] LineTrace: CameraLocation → CameraLocation + CameraForward * GrappleMaxRange
   ↓ 히트 성공
[서버] GrappleTargetLocation 저장 (Replicated)
       LaunchCharacter() 호출
       Lock_Input_Move 태그 부여
       Character_State_Grapple 태그 부여
       AGrappleCableActor 스폰 (bReplicates=true)
   ↓
[GrappleTick Task — 서버+클라이언트 동시]
   서버: 매 Tick 거리 체크 → ArrivalThreshold 이하 → EndAbility()
   클라이언트: 매 Tick Cable 시작점을 손 본 위치로 갱신
   ↓
[EndAbility()]
   CableActor Destroy (서버 → 클라이언트 자동 소멸)
   Lock_Input_Move 태그 해제
   Character_State_Grapple 태그 해제
```

---

## Key Vector Calculations

### Line Trace
```cpp
TraceStart = CameraLocation;
TraceEnd   = CameraLocation + CameraForwardVector * GrappleMaxRange;
```

### LaunchCharacter (포물선)
```cpp
FVector Direction      = (TargetLocation - CharacterLocation).GetSafeNormal();
float   HorizontalDist = FVector::Dist2D(CharacterLocation, TargetLocation);
float   LaunchSpeed    = HorizontalDist / GrappleFlightTime;

FVector LaunchVelocity = Direction * LaunchSpeed;
LaunchVelocity.Z      += HorizontalDist * GrappleLaunchArcZScale;
// GrappleLaunchArcZScale ≈ 0.3~0.6
```

### 도착 판정
```cpp
float Distance = FVector::Dist(CharacterLocation, GrappleTargetLocation);
if (Distance <= GrappleArrivalThreshold) EndAbility();
```

---

## DataAsset Fields (SpyMovementConfig 추가)

| 필드명 | 타입 | 설명 | 권장값 |
|---|---|---|---|
| `GrappleMaxRange` | `float` | Line Trace 최대 거리 | 3000.0 |
| `GrappleArrivalThreshold` | `float` | 도착 판정 거리 | 150.0 |
| `GrappleLaunchArcZScale` | `float` | 포물선 Z 보정 계수 | 0.4 |
| `GrappleFlightTime` | `float` | 발사 속도 계산용 비행 시간(초) | 0.8 |

---

## Gameplay Tags

| 태그 | 용도 |
|---|---|
| `Character_State_Grapple` | 그래플링 중 상태 표시, 중복 발동 방지 |
| `Lock_Input_Move` (기존) | 이동 입력 잠금 |

---

## Multiplayer / Replication

| 작업 | 실행 주체 | 방법 |
|---|---|---|
| Line Trace | 서버 | `Server_TryGrapple` Reliable RPC |
| LaunchCharacter | 서버 | RPC 내부 직접 호출 |
| GrappleTargetLocation | 서버→클라이언트 | `UPROPERTY(Replicated)` on GA |
| 태그 부여/해제 | 서버 | GAS 내장 레플리케이션 |
| CableActor 스폰/소멸 | 서버 | `bReplicates=true` Actor |
| Cable 시작점 위치 갱신 | 각 클라이언트 | GrappleTick Task 내 Tick |
| 도착 판정 | 서버만 | `HasAuthority()` 체크 |

---

## Constraints

- CableComponent 플러그인(`CableComponent`) 활성화 필요 (`SkillProject.uproject`)
- 손 본 이름은 GA의 `EditDefaultsOnly` UPROPERTY로 설정 (하드코딩 금지)
- 입력 태그 바인딩은 기존 `SpyInputConfig` DataAsset에서 빈 슬롯에 할당
