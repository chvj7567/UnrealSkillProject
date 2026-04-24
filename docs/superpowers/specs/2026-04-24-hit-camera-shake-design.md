# 피격 Camera Shake 설계

## 개요

플레이어가 피격당할 때와 적을 공격할 때 카메라 쉐이크를 트리거하는 시스템.
크리티컬 여부에 따라 쉐이크 강도를 차등 적용한다.

---

## 요구사항

- 플레이어가 **맞을 때** 카메라 쉐이크 발생
- 플레이어가 **적을 때릴 때** 카메라 쉐이크 발생 (히트 확인 피드백)
- 일반 피격: 약한 쉐이크 (Light)
- 크리티컬 피격: 강한 쉐이크 (Heavy)
- 쉐이크 연출: 짧고 강하게 (0.2~0.3초)

---

## 아키텍처

### 전체 흐름

```
[플레이어가 맞을 때]
USKAttributeSet::PostGameplayEffectExecute()
  → USpyHealthComponent::OnHit.Broadcast(Damage, bCritical, DamageCauser)
  → ASpyPlayerController::HandleReceivedHit()
    → PlayerCameraManager->StartCameraShake(Light or Heavy)

[플레이어가 적을 때릴 때]
적 USKAttributeSet::PostGameplayEffectExecute()
  → 적 USpyHealthComponent::OnHit.Broadcast(Damage, bCritical, DamageCauser)
  → DamageCauser(폰) → GetController() → Cast<ASpyPlayerController>
    → SpyPlayerController->HandleDealtHit(bCritical)
      → PlayerCameraManager->StartCameraShake(Light or Heavy)
```

### 설계 원칙

- **카메라 로직은 PlayerController만 담당** — Character·AI는 카메라를 모름
- AI는 PlayerController가 없으므로 자동으로 필터링됨
- `ASpyPlayerController::OnPossess()`에서 Character의 HealthComponent에 바인딩

---

## 컴포넌트 상세

### 1. USpyHealthComponent — 델리게이트 추가

```cpp
// 피격 이벤트 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHit,
    float, Damage,
    bool, bCritical,
    AActor*, DamageCauser);

UPROPERTY(BlueprintAssignable)
FOnHit OnHit;
```

### 2. USKAttributeSet::PostGameplayEffectExecute() — broadcast 추가

Health 감소 처리 후, 대상 캐릭터의 `USpyHealthComponent::OnHit`을 broadcast.
`FSKGameplayEffectContext::IsCritical()`로 크리티컬 여부 판단.

### 3. ASpyPlayerController — 쉐이크 처리

```cpp
// 에디터에서 교체 가능한 쉐이크 에셋
UPROPERTY(EditDefaultsOnly, Category="Camera")
TSubclassOf<UCameraShakeBase> HitShakeLight;   // 일반 피격

UPROPERTY(EditDefaultsOnly, Category="Camera")
TSubclassOf<UCameraShakeBase> HitShakeHeavy;   // 크리티컬 피격

// 내가 맞을 때 (Character HealthComponent에서 호출)
void HandleReceivedHit(float Damage, bool bCritical, AActor* DamageCauser);

// 내가 때릴 때 (적 HealthComponent OnHit에서 DamageCauser 추출 후 호출)
void HandleDealtHit(bool bCritical);
```

`OnPossess()`에서 Character의 `USpyHealthComponent::OnHit`에 `HandleReceivedHit` 바인딩.
`OnUnPossess()`에서 언바인딩.

### 4. 쉐이크 에셋 (블루프린트)

| 에셋 | 기반 클래스 | Scale | Duration | 용도 |
|------|------------|-------|----------|------|
| `BP_CameraShake_Light` | `UCameraShakeBase` | 0.5 | 0.2초 | 일반 피격 / 일반 히트 확인 |
| `BP_CameraShake_Heavy` | `UCameraShakeBase` | 1.0 | 0.3초 | 크리티컬 피격 / 크리티컬 히트 확인 |

Perlin Noise 기반 (`UPerlinNoiseCameraShakePattern`) 사용 — 짧고 강한 랜덤 진동.

---

## 변경 파일 목록

| 파일 | 변경 내용 |
|------|----------|
| `SpyHealthComponent.h/.cpp` | `FOnHit` 델리게이트 추가, broadcast 함수 추가 |
| `SKAttributeSet.cpp` | `PostGameplayEffectExecute()`에서 `OnHit` broadcast |
| `SpyPlayerController.h/.cpp` | 쉐이크 프로퍼티, `HandleReceivedHit()`, `HandleDealtHit()`, 바인딩 로직 |
| `BP_SpyPlayerController` | `HitShakeLight`, `HitShakeHeavy` 에셋 할당 |
| `BP_CameraShake_Light` (신규) | 일반 쉐이크 에셋 |
| `BP_CameraShake_Heavy` (신규) | 크리티컬 쉐이크 에셋 |

---

## 미결 사항

없음.
