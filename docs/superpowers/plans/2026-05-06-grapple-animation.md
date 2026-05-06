# Grapple Animation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** `USpyGA_GrappleHook`에 루핑 AnimMontage 1개를 추가해 그래플링 도중 캐릭터가 공중에 매달려 끌려가는 자세를 표현한다.

**Architecture:** AnimBlueprint / Locomotion State Machine은 미변경. GA의 `ActivateAbility` 끝에서 `UAbilityTask_PlayMontageAndWait`로 루핑 Montage 재생, GA `EndAbility` 시 Task가 자동으로 Montage Stop. `AirLoopMontage` nullptr-safe 처리로 에셋 미할당 시 기존 동작 유지.

**Tech Stack:** Unreal Engine 5.7, GAS (GameplayAbilitySystem), C++, AnimMontage (Looping)

**Spec Reference:** `docs/superpowers/specs/2026-05-06-grapple-animation-design.md`

**Branch:** `feature/Grappling-Hook` (현재 브랜치 그대로 사용)

---

## File Structure

| Path | Action | Responsibility |
|---|---|---|
| `SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyGA_GrappleHook.h` | Modify | `AirLoopMontage` UPROPERTY 추가 |
| `SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyGA_GrappleHook.cpp` | Modify | `PlayMontageAndWait` Task 호출 추가 (nullptr-safe) |
| `SkillProject/Content/Spy/Animation/Grapple/AM_Grapple_AirLoop.uasset` | Create | 루핑 AnimMontage (placeholder OK) |
| `SkillProject/Content/Spy/Blueprints/GameplayAbilities/GA_GrappleHook.uasset` | Modify | `AirLoopMontage` 슬롯에 위 Montage 할당 |

---

### Task 1: 헤더에 AirLoopMontage UPROPERTY 추가

**Files:**
- Modify: `SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyGA_GrappleHook.h`

- [ ] **Step 1: forward declaration 추가**

상단 forward declaration 블록에 `UAnimMontage` 추가.

기존:
```cpp
class AGrappleCableActor;
class USpyMovementConfig;
class USpyGrappleTargetingComponent;
```

변경 후:
```cpp
class AGrappleCableActor;
class USpyMovementConfig;
class USpyGrappleTargetingComponent;
class UAnimMontage;
```

- [ ] **Step 2: protected 멤버에 UPROPERTY 추가**

기존 `HandBoneName` 멤버 뒤에 `AirLoopMontage` 추가.

기존:
```cpp
protected:
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    TObjectPtr<USpyMovementConfig> MovementConfig;

    UPROPERTY(EditDefaultsOnly, Category = "Config")
    FName HandBoneName = FName("hand_r");
```

변경 후:
```cpp
protected:
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    TObjectPtr<USpyMovementConfig> MovementConfig;

    UPROPERTY(EditDefaultsOnly, Category = "Config")
    FName HandBoneName = FName("hand_r");

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    TObjectPtr<UAnimMontage> AirLoopMontage;
```

- [ ] **Step 3: 빌드 검증**

Unreal Editor의 Live Coding(Ctrl+Alt+F11) 또는 Visual Studio에서 `SkillProjectEditor` 타겟 빌드.
Expected: 컴파일 성공, 새 UPROPERTY가 GA Blueprint Class Defaults에 노출.

---

### Task 2: ActivateAbility에 PlayMontageAndWait 호출 추가

**Files:**
- Modify: `SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyGA_GrappleHook.cpp`

- [ ] **Step 1: include 추가**

기존 include 블록에 `AbilityTask_PlayMontageAndWait.h` 추가.

기존:
```cpp
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
```

변경 후:
```cpp
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
```

- [ ] **Step 2: ActivateAbility 함수 끝에 Montage Task 추가**

위치: 서버 Timeout 블록(`if (bAuthority) { ... }`) 닫는 `}` 직후, 함수 닫는 `}` 직전.

기존(참고용):
```cpp
    if (bAuthority)
    {
        const float Distance = FVector::Dist(Char->GetActorLocation(), TargetLocation);
        const float Timeout  = (PullSpeed > 0.f ? Distance / PullSpeed : 1.f) * 2.f;
        UAbilityTask_WaitDelay* TimeoutTask = UAbilityTask_WaitDelay::WaitDelay(this, Timeout);
        TimeoutTask->OnFinish.AddDynamic(this, &USpyGA_GrappleHook::OnGrappleArrived);
        TimeoutTask->ReadyForActivation();
        UE_LOG(LogTemp, Warning, TEXT("[GrappleGA][SRV] Timeout=%.2fs"), Timeout);
    }
}
```

변경 후:
```cpp
    if (bAuthority)
    {
        const float Distance = FVector::Dist(Char->GetActorLocation(), TargetLocation);
        const float Timeout  = (PullSpeed > 0.f ? Distance / PullSpeed : 1.f) * 2.f;
        UAbilityTask_WaitDelay* TimeoutTask = UAbilityTask_WaitDelay::WaitDelay(this, Timeout);
        TimeoutTask->OnFinish.AddDynamic(this, &USpyGA_GrappleHook::OnGrappleArrived);
        TimeoutTask->ReadyForActivation();
        UE_LOG(LogTemp, Warning, TEXT("[GrappleGA][SRV] Timeout=%.2fs"), Timeout);
    }

    if (AirLoopMontage)
    {
        if (UAbilityTask_PlayMontageAndWait* MontageTask =
            UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
                this, NAME_None, AirLoopMontage))
        {
            MontageTask->ReadyForActivation();
            UE_LOG(LogTemp, Warning, TEXT("[GrappleGA][%s] AirLoopMontage Task started"),
                bAuthority ? TEXT("SRV") : TEXT("CLI"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[GrappleGA][%s] AirLoopMontage is null — skipping Montage Task"),
            bAuthority ? TEXT("SRV") : TEXT("CLI"));
    }
}
```

콜백(OnCompleted, OnInterrupted) 바인딩 불필요 — `CreatePlayMontageAndWaitProxy`의 기본 인자 `bStopWhenAbilityEnds=true` 덕분에 GA EndAbility 시 Task가 OnDestroy → Montage Stop 자동 처리.

- [ ] **Step 3: 빌드 검증**

Live Coding 또는 VS에서 빌드.
Expected: 컴파일 성공.

---

### Task 3: 루핑 AnimMontage 에셋 생성 (placeholder)

> 이 task는 Unreal Editor에서 수행한다. `mcp__unreal__execute_python`으로 자동화 시도 가능하지만 fragile하므로 수동 절차를 우선 명시.

**Files:**
- Create: `SkillProject/Content/Spy/Animation/Grapple/AM_Grapple_AirLoop.uasset`

- [ ] **Step 1: 폴더 생성**

Content Browser → `Content/Spy/Animation/` 우클릭 → `New Folder` → 이름 `Grapple`.

- [ ] **Step 2: 베이스 AnimSequence 선택 후 Montage 생성**

placeholder로 사용할 AnimSequence를 우클릭 → `Create > Create AnimMontage`.

권장 후보(존재하는 것 중 자연스러운 포즈):
- `Falling_Idle` 계열
- `Hanging_Idle` 계열
- 마땅한 것 없으면 임시로 일반 Idle

생성된 Montage 이름을 `AM_Grapple_AirLoop`로 변경하고 `Content/Spy/Animation/Grapple/`로 이동.

- [ ] **Step 3: Montage Editor 열기**

`AM_Grapple_AirLoop` 더블클릭.

- [ ] **Step 4: 루프 섹션 설정**

Montage Editor 하단 Sections 패널 → 기본 섹션(보통 `Default`) 우클릭 → `Set as Loop` (또는 `Next Section`을 자기 자신으로 지정).

검증: 미리보기 창에서 포즈가 무한 반복되는지 확인.

- [ ] **Step 5: AutoBlendOut 비활성화**

Asset Details 패널 → `Blend Out` 또는 `Blend Option` → `Enable Auto Blend Out` 체크 해제.

이유: GA 종료 시 Task가 명시적으로 Stop하므로 Montage 자체의 자동 블렌드아웃은 불필요/방해됨.

- [ ] **Step 6: Slot 확인**

좌측 Anim Slot 패널 또는 Track 헤더 → Slot 이름이 `DefaultGroup.DefaultSlot`인지 확인. 다르면 변경.

- [ ] **Step 7: 저장**

`Ctrl+S`. Content Browser에서 `Content/Spy/Animation/Grapple/AM_Grapple_AirLoop` 경로로 존재하는지 재확인.

---

### Task 4: GA Blueprint에 AirLoopMontage 슬롯 할당

**Files:**
- Modify: `SkillProject/Content/Spy/Blueprints/GameplayAbilities/GA_GrappleHook.uasset`

- [ ] **Step 1: GA Blueprint 열기**

Content Browser → `Content/Spy/Blueprints/GameplayAbilities/GA_GrappleHook` 더블클릭.

- [ ] **Step 2: Class Defaults에서 슬롯 채우기**

상단 툴바 `Class Defaults` 클릭 → Details 패널 → `Animation` 카테고리 → `Air Loop Montage` 슬롯 드롭다운에서 `AM_Grapple_AirLoop` 선택.

- [ ] **Step 3: 컴파일 + 저장**

`Compile` → `Save`. Output Log에 컴파일 에러 없어야 함.

(대안: `mcp__unreal__set_blueprint_cdo_property`로 자동화 가능. 에디터가 켜진 상태에서 시도)

---

### Task 5: 단일 플레이어 PIE 검증

- [ ] **Step 1: PIE 실행 (Standalone)**

Toolbar `Play` 버튼 옆 화살표 → `Number of Players: 1`, `Net Mode: Play Standalone` → `Play` 또는 `Alt+P`.

- [ ] **Step 2: 그래플 입력**

타겟 프롬프트가 뜨는 위치에서 그래플 입력 키 누르기 (`SpyInputConfig`의 `IA_GrappleHook` 바인딩).

Expected:
- 케이블이 캐릭터 손에서 발사
- 캐릭터가 공중에서 타겟으로 끌려감
- **공중에 매달려 끌려가는 포즈가 재생됨** (이전과 시각적으로 다름)

- [ ] **Step 3: 도착 시 자세 정상 전환**

도착 후:
- 케이블 사라짐
- 포즈가 자연스럽게 Locomotion(Idle/Fall)로 블렌드 아웃
- 캐릭터가 그 자리 포즈로 굳지 않음

- [ ] **Step 4: 로그 확인**

Output Log → 검색 `AirLoopMontage`. 다음 라인이 시간순으로 보여야 함:
```
[GrappleGA][CLI] AirLoopMontage Task started
[GrappleGA][SRV] AirLoopMontage Task started
```

(LocalPredicted GA라 양쪽에서 호출됨)

---

### Task 6: 멀티플레이어 PIE 검증 (Listen Server + 1 Client)

- [ ] **Step 1: PIE Net Mode 설정**

Toolbar Play 버튼 옆 화살표 → 다음 설정:
- `Number of Players: 2`
- `Net Mode: Play As Listen Server`

- [ ] **Step 2: PIE 실행**

`Alt+P`. 두 개의 게임 창이 열림.

- [ ] **Step 3: 양쪽에서 그래플 발사**

각 창에서 각각 그래플 입력.

Expected:
- **자기 시점**: 자기 캐릭터에 공중 자세 표시
- **상대 시점 (시뮬 클라)**: 상대 캐릭터에도 공중 자세 표시 (GAS 표준 Montage Replication)

- [ ] **Step 4: 동시 발동 검증**

양쪽이 동시에 그래플하는 경우에도 양쪽 시점에서 모두 자세가 정상적으로 보이는지 확인.

---

### Task 7: Edge Case — 슬롯 nullptr 안전성 검증

- [ ] **Step 1: GA Blueprint에서 슬롯 비우기**

`GA_GrappleHook` Class Defaults → `Air Loop Montage` 슬롯 옆 `[X]` 버튼 → None으로 변경 → 컴파일 + 저장.

- [ ] **Step 2: PIE 실행 후 그래플**

Expected:
- 빌드/런타임 크래시 없음
- 그래플 동작은 기존(자세 없이 fall idle)대로 작동
- Output Log에 다음 출력:
```
[GrappleGA][CLI] AirLoopMontage is null — skipping Montage Task
[GrappleGA][SRV] AirLoopMontage is null — skipping Montage Task
```

- [ ] **Step 3: 슬롯 복원**

검증 끝나면 다시 `AM_Grapple_AirLoop` 할당 → 컴파일 + 저장.

---

### Task 8: Edge Case — 외부 캔슬 시 Montage 자동 정지 검증

> 검증 시나리오는 게임 매커니즘에 맞는 것을 선택. 가능한 옵션은 캐릭터 사망(KillZ로 추락 등) 또는 콘솔로 GA 강제 종료.

- [ ] **Step 1: 그래플 발사**

평소처럼 그래플 시작.

- [ ] **Step 2: 도착 전 GA 강제 종료 트리거**

옵션 (A): KillZ 영역으로 끌려가게 해서 캐릭터 사망 유도
옵션 (B): 콘솔 명령 `AbilitySystem.CancelAbilities` 또는 디버그 키로 GA Cancel (프로젝트에 있는 경우)
옵션 (C): 도중에 다른 입력으로 BlockedTags에 의한 cancel 발생시키기

- [ ] **Step 3: Montage 자동 정지 확인**

Expected:
- GA 종료와 동시에 공중 자세 Montage가 자동 정지
- 캐릭터가 정상 Locomotion으로 복귀
- 케이블 actor도 Destroy 됨 (기존 EndAbility 로직)
- 포즈가 그 자리에 굳거나 무한 루프되지 않음

---

### Task 9: 변경사항 staging + 커밋 메시지 제안

> `.claude/rules/git-conventions.md`에 따라 Commit은 사용자가 명시적으로 요청할 때만 실행. 이 task는 staging까지만 수행하고 메시지를 제안한다.

- [ ] **Step 1: 변경 파일 stage**

```bash
git add SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyGA_GrappleHook.h \
        SkillProject/Source/SkillProject/AbilitySystem/Movement/SpyGA_GrappleHook.cpp \
        SkillProject/Content/Spy/Animation/Grapple/AM_Grapple_AirLoop.uasset \
        SkillProject/Content/Spy/Blueprints/GameplayAbilities/GA_GrappleHook.uasset
```

- [ ] **Step 2: staging 확인**

```bash
git status --short SkillProject/Source/SkillProject/AbilitySystem/Movement/ \
                   SkillProject/Content/Spy/Animation/Grapple/ \
                   SkillProject/Content/Spy/Blueprints/GameplayAbilities/
```

Expected: 위 4개 파일이 `A` 또는 `M` 상태로 표시됨.

- [ ] **Step 3: 커밋 메시지 제안 (사용자 승인 후 커밋)**

제안 메시지:
```
[Feature] SpyGA_GrappleHook — 그래플 공중 자세 루핑 Montage 추가
```

본문이 필요하면:
```
[Feature] SpyGA_GrappleHook — 그래플 공중 자세 루핑 Montage 추가

- AirLoopMontage UPROPERTY 추가, ActivateAbility에서 PlayMontageAndWait Task 호출
- nullptr-safe 처리로 슬롯 미할당 시 기존 동작 유지
- AM_Grapple_AirLoop placeholder 에셋 추가, GA_GrappleHook 슬롯 할당
```

---

## Self-Review Notes

- **Spec coverage**:
  - Architecture (변경 파일 표): Task 1-2 (코드), Task 3-4 (에셋)
  - Execution Flow / Code Sketch: Task 1 Step 2, Task 2 Step 2에 그대로 반영
  - Multiplayer / Replication: Task 6
  - Edge Cases (정상 도착 / nullptr / 외부 캔슬): Task 5, 7, 8
  - Constraints (DefaultSlot / nullptr-safe / Placeholder): Task 3 Step 6, Task 2 Step 2, Task 7
  - Out of Scope: 명시적 제외라 task 없음 (정상)
- **Placeholder scan**: TBD/TODO/일반 표현 없음. 모든 step에 구체 코드/명령/경로 포함.
- **Type consistency**: `AirLoopMontage` 명칭, `UAbilityTask_PlayMontageAndWait`, `CreatePlayMontageAndWaitProxy` 모든 task에서 일관됨.
- **검증 누락**: 서버 Timeout 시나리오는 아주 먼 거리 그래플 시에만 발동하므로 별도 task로 분리하지 않음 — Task 5/8 흐름에서 자연스럽게 커버.
