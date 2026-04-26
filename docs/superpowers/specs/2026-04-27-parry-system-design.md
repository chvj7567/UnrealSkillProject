# 패링 시스템 설계

## 요구사항 요약

- **방식**: 홀드형 — 버튼을 누르고 있는 동안 패링 윈도우 유지
- **방향**: 정면 공격만 막음 (방어자 기준 90도 이내)
- **성공 시**: 데미지 완전 무효화 + 방어자 `LaunchCharacter` 뒤로 넉백
- **사용자**: 플레이어 캐릭터만
- **비용**: 쿨다운·스태미나 없음

---

## 아키텍처 개요

콤보 시스템(`SpyAnimNotify_State_Combo`)과 동일한 AnimNotifyState + ASC 루즈 태그 패턴을 사용한다.
서버 전용 환경에서 AnimInstance가 틱하지 않을 수 있으므로, 서버 측 패링 상태는 GA가 직접 관리하고 AnimNotify는 클라이언트 시각 동기화 용도로만 사용한다.

---

## 전체 흐름

```
[홀드 입력] Input_Ability_Parry
      ↓
GA_Parry 활성화
  ├── 서버: ASC에 Character_State_Parry 태그 직접 추가
  └── 파링 몽타주 루프 재생 (replicated)
        └── SpyAnimNotify_State_Parry: 클라이언트 측 태그 동기화

[입력 해제]
      ↓
GA_Parry.InputReleased()
  ├── 서버: Character_State_Parry 태그 제거
  └── 몽타주 중단 + EndAbility

[공격자 GA 데미지 처리] — 서버에서만 실행
SKGameplayAbility_SkillAction::SendTagToTargetByWeapon/Sphere
  ├── 타겟 ASC에 Character_State_Parry 있는지 확인
  ├── 있으면: 정면 방향 체크
  │     ├── 성공: 데미지 이벤트 스킵 + 방어자 LaunchCharacter
  │     └── 실패: 기존 데미지 로직 그대로
  └── 없으면: 기존 데미지 로직 그대로
```

---

## 구성 요소

### 새로 생성

| 파일 | 역할 |
|------|------|
| `Source/SkillProject/AbilitySystem/Parry/SpyGameplayAbility_Parry.h/.cpp` | 홀드형 패링 GA |
| `Source/SkillProject/Character/AnimInstance/SpyAnimNotify_State_Parry.h/.cpp` | 클라이언트 태그 동기화용 AnimNotifyState |

### 수정

| 파일 | 변경 내용 |
|------|-----------|
| `Source/SKGAS/SKGameplayTags.h/.cpp` | `Character_State_Parry` 태그 추가 |
| `Source/SkillProject/Util/SpyGameplayTags.h/.cpp` | `Input_Ability_Parry` 태그 추가 |
| `Source/SKGAS/Ability/SKGameplayAbility_SkillAction.cpp` | `SendTagToTargetByWeapon/Sphere`에 패링 체크 삽입 |

---

## 새 Gameplay Tags

| 태그 | 위치 | 용도 |
|------|------|------|
| `Character_State_Parry` | `SKGameplayTags` (SKGAS 레이어) | 패링 윈도우 활성 상태 |
| `Input_Ability_Parry` | `SpyGameplayTags` (게임 전용) | 패링 입력 바인딩 |

---

## 핵심 로직 상세

### SpyAnimNotify_State_Parry

콤보 AnimNotify와 동일 패턴. 클라이언트에서 `Character_State_Parry` 루즈 태그를 Add/Remove한다.

```cpp
void NotifyBegin(...) { ASC->AddLooseGameplayTag(SKGameplayTags::Character_State_Parry); }
void NotifyEnd(...)   { ASC->RemoveLooseGameplayTag(SKGameplayTags::Character_State_Parry); }
```

### SpyGameplayAbility_Parry

```
ActivateAbility()
  ├── HasAuthority() → ASC->AddLooseGameplayTag(Character_State_Parry)
  └── PlayMontageAndWait(ParryMontage) — 루프 몽타주, 없으면 idle 유지

InputReleased()
  ├── HasAuthority() → ASC->RemoveLooseGameplayTag(Character_State_Parry)
  └── CancelAbility()
```

`KnockbackForce`는 `UPROPERTY(EditAnywhere)`로 BP에서 조절 가능하게 노출한다.

### 정면 체크

방어자의 전방 벡터와 공격자 방향 벡터의 내적으로 판정한다.

```cpp
FVector DefenderForward = Defender->GetActorForwardVector();
FVector ToAttacker = (Attacker->GetActorLocation() - Defender->GetActorLocation()).GetSafeNormal();
bool bIsFrontal = FVector::DotProduct(DefenderForward, ToAttacker) > 0.0f; // 90도 이내
```

### 넉백

방어자의 전방 반대 방향으로 `LaunchCharacter` 적용.

```cpp
FVector KnockbackDir = -Defender->GetActorForwardVector();
Defender->LaunchCharacter(KnockbackDir * KnockbackForce, true, false);
```

### SKGameplayAbility_SkillAction 패링 체크 삽입 위치

`SendTagToTargetByWeapon` / `SendTagToTargetBySphere` 내부에서 타겟에게 이벤트를 전송하기 직전:

```
타겟 루프 진입
  ├── 타겟 ASC에 Character_State_Parry 태그 있는지 확인
  ├── 있으면 bIsFrontal 체크
  │     ├── true: continue (데미지 스킵) + LaunchCharacter
  │     └── false: 기존 데미지 이벤트 전송
  └── 없으면: 기존 데미지 이벤트 전송
```

---

## 멀티플레이어 고려사항

- 패링 체크(`SendTagToTarget`)는 이미 `HasAuthority()` 블록 안에서 실행되므로 서버 권한 보장됨
- `LaunchCharacter`는 서버에서 호출 후 클라이언트에 레플리케이트됨
- `Character_State_Parry` 태그는 GA에서 서버 측 직접 관리 (AnimNotify 의존 없음)
