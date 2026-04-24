# Hit Camera Shake Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 플레이어가 피격당할 때와 적을 때릴 때 카메라 쉐이크를 트리거하는 시스템 구현.

**Architecture:** `SpyHealthComponent`에 `OnHit` 델리게이트를 추가하고, `HandleHealthChanged`에서 크리티컬 여부를 추출해 broadcast한다. `SpyPlayerController`가 자신 캐릭터의 `OnHit`을 구독해 "맞을 때" 쉐이크를 처리하고, `HandleHealthChanged`에서 공격자의 PlayerController를 직접 호출해 "때릴 때" 쉐이크를 처리한다.

**Tech Stack:** Unreal Engine 5.7, GAS (GameplayAbilitySystem), UCameraShakeBase (Perlin Noise), FSKGameplayEffectContext

---

## 파일 맵

| 파일 | 변경 유형 | 책임 |
|------|----------|------|
| `Source/SkillProject/Character/SpyHealthComponent.h` | 수정 | `FSpyHealth_HitEvent` 델리게이트 선언, `OnHit` UPROPERTY |
| `Source/SkillProject/Character/SpyHealthComponent.cpp` | 수정 | `HandleHealthChanged`에서 critical 추출·broadcast·공격자 PC 알림 |
| `Source/SkillProject/System/SpyPlayerController.h` | 수정 | 쉐이크 프로퍼티, `HandleReceivedHit`, `HandleDealtHit`, `OnUnPossess` 선언 |
| `Source/SkillProject/System/SpyPlayerController.cpp` | 수정 | 바인딩 로직 및 쉐이크 트리거 구현 |

---

## Task 1: SpyHealthComponent — OnHit 델리게이트 추가

**Files:**
- Modify: `Source/SkillProject/Character/SpyHealthComponent.h`
- Modify: `Source/SkillProject/Character/SpyHealthComponent.cpp`

- [ ] **Step 1: `SpyHealthComponent.h`에 델리게이트 및 프로퍼티 추가**

기존 `FSpyHealth_DeathEvent` 선언 바로 아래에 추가:

```cpp
// SpyHealthComponent.h — 기존 델리게이트 선언들 아래에 추가
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSpyHealth_HitEvent,
    float, Damage,
    bool, bCritical,
    AActor*, DamageCauser);
```

`public:` 섹션의 `OnDeath` 프로퍼티 아래에 추가:

```cpp
UPROPERTY(BlueprintAssignable)
FSpyHealth_HitEvent OnHit;
```

- [ ] **Step 2: `SpyHealthComponent.cpp`에 include 추가**

파일 상단 include 블록 끝에 추가:

```cpp
#include "SKGameplayEffectContext.h"
#include "System/SpyPlayerController.h"
```

- [ ] **Step 3: `HandleHealthChanged`에서 OnHit broadcast 및 공격자 PC 알림 추가**

`SpyHealthComponent.cpp`의 `HandleHealthChanged` 전체를 아래로 교체:

```cpp
void USpyHealthComponent::HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
    if (NewValue <= 0)
    {
        OnDeath.Broadcast(DamageInstigator, DamageCauser);
    }

    OnHealthChanged.Broadcast(this, OldValue, NewValue, DamageInstigator);

    // 실제 피해가 발생한 경우에만 (DamageMagnitude < 0 = 감소)
    if (DamageMagnitude < 0.0f)
    {
        bool bCritical = false;
        if (DamageEffectSpec)
        {
            FSKGameplayEffectContext* Ctx = FSKGameplayEffectContext::ExtractEffectContext(
                DamageEffectSpec->GetContext());
            if (Ctx)
            {
                bCritical = Ctx->IsCritical();
            }
        }

        const float ActualDamage = FMath::Abs(DamageMagnitude);

        // 피격자 쪽 이벤트
        OnHit.Broadcast(ActualDamage, bCritical, DamageCauser);

        // 공격자(DamageCauser)가 플레이어라면 해당 PC에도 알림
        if (APawn* CauserPawn = Cast<APawn>(DamageCauser))
        {
            if (ASpyPlayerController* PC = Cast<ASpyPlayerController>(CauserPawn->GetController()))
            {
                PC->HandleDealtHit(bCritical);
            }
        }
    }
}
```

- [ ] **Step 4: 빌드 확인**

Visual Studio에서 빌드하거나 언리얼 에디터에서 컴파일.
에러 없이 빌드 성공 확인.

- [ ] **Step 5: 커밋**

```bash
git add SkillProject/Source/SkillProject/Character/SpyHealthComponent.h
git add SkillProject/Source/SkillProject/Character/SpyHealthComponent.cpp
git commit -m "[Feature] SpyHealthComponent에 OnHit 델리게이트 추가"
```

---

## Task 2: SpyPlayerController — 카메라 쉐이크 로직 추가

**Files:**
- Modify: `Source/SkillProject/System/SpyPlayerController.h`
- Modify: `Source/SkillProject/System/SpyPlayerController.cpp`

- [ ] **Step 1: `SpyPlayerController.h`에 include, 프로퍼티, 함수 선언 추가**

기존 include 블록 끝에 추가:

```cpp
#include "Camera/CameraShakeBase.h"
#include "Character/SpyHealthComponent.h"
```

`protected:` 섹션 `virtual void BeginPlay()` 위에 `OnUnPossess` 추가:

```cpp
virtual void OnUnPossess() override;
```

`public:` 섹션 `ToggleCursorMode()` 아래에 추가:

```cpp
void HandleReceivedHit(float Damage, bool bCritical, AActor* DamageCauser);
void HandleDealtHit(bool bCritical);
```

`protected:` 섹션 `bool bCursorMode` 아래에 추가:

```cpp
UPROPERTY(EditDefaultsOnly, Category = "Camera|Shake")
TSubclassOf<UCameraShakeBase> HitShakeLight;

UPROPERTY(EditDefaultsOnly, Category = "Camera|Shake")
TSubclassOf<UCameraShakeBase> HitShakeHeavy;

// OnUnPossess 시점에 GetPawn()은 이미 null이므로 별도 보관
UPROPERTY()
TWeakObjectPtr<USpyHealthComponent> BoundHealthComponent;
```

- [ ] **Step 2: `SpyPlayerController.cpp`에 include 추가**

`SpyPlayerController.cpp`의 기존 include 블록에 이미 `Character/SpyCharacter.h`가 있으므로 추가 불필요. (`SpyPlayerController.h`에서 이미 include)

- [ ] **Step 3: `OnPossess`에 HealthComponent 바인딩 추가**

기존 `OnPossess` 함수를 아래로 교체:

```cpp
void ASpyPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    TargetingComp = GetPawn()->FindComponentByClass<USpyTargetingManagerComponent>();

    if (USpyHealthComponent* HC = USpyHealthComponent::FindHealthComponent(InPawn))
    {
        HC->OnHit.AddDynamic(this, &ASpyPlayerController::HandleReceivedHit);
        BoundHealthComponent = HC;
    }
}
```

- [ ] **Step 4: `OnUnPossess` 구현 추가**

`SpyPlayerController.cpp`에 `OnPossess` 바로 아래에 추가.
`OnUnPossess` 호출 시점에 `GetPawn()`은 이미 null이므로 `BoundHealthComponent`에서 직접 제거:

```cpp
void ASpyPlayerController::OnUnPossess()
{
    if (BoundHealthComponent.IsValid())
    {
        BoundHealthComponent->OnHit.RemoveDynamic(this, &ASpyPlayerController::HandleReceivedHit);
        BoundHealthComponent = nullptr;
    }

    Super::OnUnPossess();
}
```

- [ ] **Step 5: `HandleReceivedHit` / `HandleDealtHit` 구현 추가**

`SpyPlayerController.cpp` 끝에 추가:

```cpp
void ASpyPlayerController::HandleReceivedHit(float Damage, bool bCritical, AActor* DamageCauser)
{
    if (!PlayerCameraManager) return;

    TSubclassOf<UCameraShakeBase> ShakeClass = bCritical ? HitShakeHeavy : HitShakeLight;
    if (ShakeClass)
    {
        PlayerCameraManager->StartCameraShake(ShakeClass);
    }
}

void ASpyPlayerController::HandleDealtHit(bool bCritical)
{
    if (!PlayerCameraManager) return;

    TSubclassOf<UCameraShakeBase> ShakeClass = bCritical ? HitShakeHeavy : HitShakeLight;
    if (ShakeClass)
    {
        PlayerCameraManager->StartCameraShake(ShakeClass);
    }
}
```

- [ ] **Step 6: 빌드 확인**

Visual Studio에서 빌드하거나 언리얼 에디터에서 컴파일.
에러 없이 빌드 성공 확인.

- [ ] **Step 7: 커밋**

```bash
git add SkillProject/Source/SkillProject/System/SpyPlayerController.h
git add SkillProject/Source/SkillProject/System/SpyPlayerController.cpp
git commit -m "[Feature] SpyPlayerController에 피격/공격 카메라 쉐이크 로직 추가"
```

---

## Task 3: 에디터 — 블루프린트 쉐이크 에셋 생성 및 연결

> 이 작업은 언리얼 에디터에서 직접 수행한다.

- [ ] **Step 1: `BP_CameraShake_Light` 생성**

1. 콘텐츠 브라우저에서 적절한 폴더(예: `Content/VFX/CameraShake/`) 이동
2. 우클릭 → **Blueprint Class** → 부모 클래스 검색창에 `CameraShakeBase` 입력 → 선택
3. 이름: `BP_CameraShake_Light`
4. 열기 → **Root Shake Pattern** 드롭다운에서 `PerlinNoiseCameraShakePattern` 선택
5. 패턴 설정:
   - `Duration` → `0.2`
   - `X` (Location): `Amplitude = 3.0`, `Frequency = 25.0`
   - `Y` (Location): `Amplitude = 3.0`, `Frequency = 25.0`
   - `Pitch` (Rotation): `Amplitude = 1.0`, `Frequency = 25.0`
   - `Yaw` (Rotation): `Amplitude = 1.0`, `Frequency = 25.0`
6. 컴파일 & 저장

- [ ] **Step 2: `BP_CameraShake_Heavy` 생성**

1. 위와 동일한 폴더에 `BP_CameraShake_Heavy` 생성 (`CameraShakeBase` 상속)
2. **Root Shake Pattern** → `PerlinNoiseCameraShakePattern` 선택
3. 패턴 설정:
   - `Duration` → `0.3`
   - `X` (Location): `Amplitude = 7.0`, `Frequency = 30.0`
   - `Y` (Location): `Amplitude = 7.0`, `Frequency = 30.0`
   - `Pitch` (Rotation): `Amplitude = 2.5`, `Frequency = 30.0`
   - `Yaw` (Rotation): `Amplitude = 2.5`, `Frequency = 30.0`
4. 컴파일 & 저장

- [ ] **Step 3: `BP_SpyPlayerController`에 쉐이크 에셋 할당**

1. 콘텐츠 브라우저에서 `BP_SpyPlayerController` 열기
2. **Class Defaults** → **Camera | Shake** 카테고리 확인
3. `Hit Shake Light` → `BP_CameraShake_Light` 선택
4. `Hit Shake Heavy` → `BP_CameraShake_Heavy` 선택
5. 컴파일 & 저장

- [ ] **Step 4: 인게임 테스트 — 피격 쉐이크**

1. PIE(Play In Editor) 실행
2. 적에게 피격당하면 화면이 약하게 흔들리는지 확인
3. 크리티컬 피격 시 더 강하게 흔들리는지 확인

- [ ] **Step 5: 인게임 테스트 — 공격 쉐이크**

1. 적을 일반 공격 → 화면이 약하게 흔들리는지 확인
2. 크리티컬 공격 발생 → 더 강하게 흔들리는지 확인

- [ ] **Step 6: 커밋**

```bash
git add SkillProject/Content/VFX/CameraShake/
git commit -m "[Feature] 피격/공격 카메라 쉐이크 BP 에셋 추가 및 PC 연결"
```
