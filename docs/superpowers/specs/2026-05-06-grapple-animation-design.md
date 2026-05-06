# Grapple Animation Design

**Date:** 2026-05-06
**Branch:** feature/Grappling-Hook
**Approach:** Looping AnimMontage in GA (no AnimBP changes)

---

## Overview

그래플링 도중 캐릭터가 공중에 매달려 끌려가는 자세를 표현하기 위해, **루핑 AnimMontage 1개**를 `USpyGA_GrappleHook`에서 직접 재생한다. AnimBP / Locomotion State Machine은 수정하지 않는다.

**범위 결정**
- Throw(던지기) / Land(착지) 모션은 추가하지 않음 (공중 자세 1단계만)
- 비행 시간이 거리에 따라 가변이라는 그래플 특성상 루핑 Montage가 가장 단순하고 안전

**구현 방식 결정**
- AnimBP State Machine 추가 / LinkedAnimLayer 교체 대신, GA가 `UAbilityTask_PlayMontageAndWait`로 Montage 재생
- ABP 미변경 → 모든 변경이 코드 + DataAsset 슬롯 + 1개 AnimMontage 에셋으로 한정

---

## Architecture

### 변경 파일

| 파일 | 변경 내용 |
|---|---|
| `SpyGA_GrappleHook.h` | `UPROPERTY(EditDefaultsOnly, Category="Animation") TObjectPtr<UAnimMontage> AirLoopMontage` 1개 추가 |
| `SpyGA_GrappleHook.cpp` | `ActivateAbility` 끝부분에 `UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy` 호출. nullptr-safe 처리 |

### 신규 에셋 슬롯

| 에셋 | 권장 경로 | 사양 |
|---|---|---|
| `AM_Grapple_AirLoop` | `Content/Spy/Animation/Grapple/` | 단일 정적 포즈를 루프 섹션으로 / `bEnableAutoBlendOut = false` / `Slot = DefaultSlot` |

### 미변경

- `AGrappleCableActor`, `USpyAbilityTask_GrappleTick`, `USpyMovementConfig`, `USpyAnimAssetData`
- 모든 AnimBlueprint, Locomotion State Machine
- GameplayTags (`Character_State_Grapple`, `Lock_Input_Move` 그대로 사용)

---

## Execution Flow

```
[입력] → GA ActivateAbility (LocalPredicted, 양쪽 호출)
   ↓
[양쪽] AddLooseGameplayTag(Lock_Input_Move, Character_State_Grapple)
   ↓
[서버만] CableActor 스폰 + InitCable
   ↓
[양쪽] GrappleTick Task ReadyForActivation       (기존)
   ↓
[양쪽] PlayMontageAndWait(AirLoopMontage)        (신규)
   │   - 루핑 Montage라 OnCompleted 미호출
   │   - GA EndAbility 시 Task OnDestroy → Montage Stop 자동 처리
   ↓
[서버] Distance ≤ ArrivalThreshold → OnGrappleArrived → EndAbility
   ↓
[EndAbility] 태그 제거 + CableActor Destroy + Montage 자동 정리
```

**핵심 포인트**

- `LocalPredicted` GA이므로 양쪽(서버·로컬 클라)에서 `PlayMontageAndWait` 호출. GAS 표준 Montage Replication이 다른 시뮬 클라에 자동 전파.
- Montage 명시적 Stop 호출 불필요 — `UAbilityTask_PlayMontageAndWait::OnDestroy`가 GA 종료 시 처리.
- `AirLoopMontage`가 nullptr인 경우 Task 생성을 스킵 (안전 fallback).

---

## Code Sketch

### 헤더 추가

```cpp
// SpyGA_GrappleHook.h
class UAnimMontage;

// USpyGA_GrappleHook 내부
protected:
    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    TObjectPtr<UAnimMontage> AirLoopMontage;
```

### ActivateAbility 마지막 블록 추가

```cpp
// 기존 GrappleTick Task ReadyForActivation 직후
if (AirLoopMontage)
{
    if (UAbilityTask_PlayMontageAndWait* MontageTask =
        UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, AirLoopMontage))
    {
        MontageTask->ReadyForActivation();
    }
}
```

콜백(`OnCompleted` / `OnInterrupted` 등) 바인딩은 불필요 — GA EndAbility 시 Task가 자동 정리.

---

## Multiplayer / Replication

| 작업 | 실행 주체 | 방법 |
|---|---|---|
| Montage 재생 | 양쪽 호출 (서버·로컬 클라) | LocalPredicted GA 기본 동작 |
| 다른 시뮬 클라 동기화 | 서버 → 시뮬 클라 | GAS 표준 Montage Replication |
| Montage 정지 | 양쪽 | GA EndAbility → Task OnDestroy |

CableActor / 태그 / GrappleTick 같은 기존 흐름은 그대로 유지됨.

---

## Edge Cases

| 시나리오 | 처리 |
|---|---|
| 정상 도착 | `OnGrappleArrived` → `EndAbility` → Montage 자동 Stop → ABP가 Locomotion으로 자연 블렌드 |
| 서버 Timeout | 기존 `UAbilityTask_WaitDelay` 발동 → 동일 흐름 |
| `AirLoopMontage` 미할당 | Task 생성 스킵, 기존 동작과 동일 |
| 외부 캔슬 (피격 / 강제 종료) | `EndAbility(bWasCancelled=true)` → Task OnDestroy → Montage Stop |
| 캐릭터 사망 | ASC 정리 흐름에서 GA EndAbility 발동 → 동일 |

---

## Constraints

- **Montage Slot**: `DefaultSlot` 사용. 그래플 도중 `Lock_Input_Move`로 다른 이동 입력이 막혀있어 슬롯 충돌 없음.
- **Asset Manager 미경유**: GA 인스턴스가 이미 로드된 상태에서 `TObjectPtr<UAnimMontage>` 하드 레퍼런스. 별도 Async Load 불필요.
- **Placeholder**: 본 디자인은 `AirLoopMontage` 슬롯이 nullptr여도 빌드/런타임이 정상 동작하도록 작성 → 에셋 확보 전에 코드부터 머지 가능.

---

## Out of Scope (이번 작업 아님)

- Throw / Land 모션 (현재 의도적으로 제외)
- 상체/하체 분리 (`UpperBody` 슬롯 등) — 필요 시 후속 작업
- 카메라 셰이크 / 사운드 / GameplayCue
- 네트워크 시뮬 클라에서의 별도 PlayMontage 보정 (GAS 표준 동작으로 충분하다고 판단)
- `SpyAnimAssetData` 통합 (캐릭터별 다양화는 후속 검토)
