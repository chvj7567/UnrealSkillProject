# Weapon Trail Effect Design

**Date:** 2026-04-30  
**Status:** Approved

## Overview

`GA_Skill*` 계열 Gameplay Ability가 활성화되는 동안 무기(`ASpyWeapon`)에 트레일 파티클 이펙트를 붙인다.  
이펙트 에셋은 프로젝트 기존 에셋인 `P_Kallari_Primary_Trail`을 사용한다.

## Scope

- **수정 대상:** `ASpyWeapon`, `USpyGameplayAbility_SkillAction`
- **적용 범위:** `USpyGameplayAbility_SkillAction`을 베이스로 하는 모든 `GA_Skill*` Blueprint GA
- **트레일 활성 구간:** GA `ActivateAbility` ~ `EndAbility`

## ASpyWeapon 변경

### 추가 멤버

```cpp
// UPROPERTY
UPROPERTY(EditDefaultsOnly, Category = "FX")
TObjectPtr<UParticleSystem> TrailEffect;

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FX")
TObjectPtr<UParticleSystemComponent> WeaponTrailComponent;
```

### 생성자

- `WeaponTrailComponent`를 `CreateDefaultSubobject`로 생성
- `WeaponSkeletalMeshComponent`에 Attach
- `bAutoActivate = false` — 기본 비활성화

### 인터페이스 함수

```cpp
void ActivateTrail();   // SetTemplate(TrailEffect) + Activate()
void DeactivateTrail(); // Deactivate()
```

## USpyGameplayAbility_SkillAction 변경

### ActivateAbility()

기존 로직 이후 (`HasAuthority()` 블록 밖) 트레일 활성화:

```cpp
ASpyCharacter* SpyChar = Cast<ASpyCharacter>(GetAvatarActorFromActorInfo());
if (SpyChar && SpyChar->GetSpyWeapon())
{
    SpyChar->GetSpyWeapon()->ActivateTrail();
}
```

### EndAbility() (신규 오버라이드)

트레일 비활성화 후 `Super::EndAbility()` 호출:

```cpp
ASpyCharacter* SpyChar = Cast<ASpyCharacter>(GetAvatarActorFromActorInfo());
if (SpyChar && SpyChar->GetSpyWeapon())
{
    SpyChar->GetSpyWeapon()->DeactivateTrail();
}
Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
```

## 멀티플레이 고려사항

- 트레일 이펙트는 비주얼 전용이므로 `HasAuthority()` 체크 없이 실행
- 서버/클라이언트 모두 GA를 실행하는 구조에서 각자 로컬에서 처리

## 에셋 경로

| 에셋 | 경로 |
|------|------|
| 트레일 파티클 | `Content/Spy/FX/Particles/Kallari/Abilities/Primary/FX/P_Kallari_Primary_Trail` |

## 수정 파일 목록

| 파일 | 변경 내용 |
|------|-----------|
| `Source/SkillProject/Item/SpyWeapon.h` | `TrailEffect`, `WeaponTrailComponent` 추가, `ActivateTrail`/`DeactivateTrail` 선언 |
| `Source/SkillProject/Item/SpyWeapon.cpp` | 생성자 초기화, `ActivateTrail`/`DeactivateTrail` 구현 |
| `Source/SkillProject/AbilitySystem/Skill/SpyGameplayAbility_SkillAction.h` | `EndAbility` 선언 추가 |
| `Source/SkillProject/AbilitySystem/Skill/SpyGameplayAbility_SkillAction.cpp` | `ActivateAbility` 트레일 호출 추가, `EndAbility` 구현 |
