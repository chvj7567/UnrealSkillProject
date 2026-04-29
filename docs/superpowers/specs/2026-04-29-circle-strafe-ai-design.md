# Circle Strafe AI 설계 문서

**날짜:** 2026-04-29  
**범위:** EQS 기반 공전 AI 로직 — EQS 쿼리, BT Task, AIController Focus, 애니메이션 Direction 계산

---

## 1. 목표

공격 쿨다운 중인 AI가 플레이어 주변을 원형으로 공전(Circle Strafe)하도록 한다.  
공전 방향은 EQS 실행마다 랜덤으로 좌/우 결정하며, 이동 중에도 항상 타겟을 응시한다.  
8방향 블렌드스페이스로 좌/우 스트레이프 애니메이션을 출력한다.

---

## 2. 발동 조건

- 트리거: 공격 쿨다운 중 (BT Decorator로 쿨다운 상태 체크)
- 해제: 쿨다운 종료 → 기존 `BTTask_MoveToTarget`으로 복귀

---

## 3. 추가/수정 대상

| 구분 | 이름 | 경로 |
|------|------|------|
| C++ 신규 | `UBTTask_CircleStrafe` | `Source/SkillProject/AI/` |
| C++ 신규 | `UEnvQueryContext_StrafeDirection` | `Source/SkillProject/AI/` |
| 에디터 신규 | `EQS_CircleStrafe` | `Content/Spy/AI/` |
| BB Key 추가 | `StrafeLocation` (Vector), `bStrafeLeft` (Bool) | `BB_SpyAI` |
| C++ 수정 | `SpyAnimManagerComponent::CalcDirectionAngle()` | 기존 파일 수정 |

---

## 4. EQS 설정 (`EQS_CircleStrafe`)

### Generator
- 타입: `EnvQueryGenerator_Donut`
- Center Context: Target Actor (플레이어)
- Inner Radius: 500
- Outer Radius: 800
- Number of Rings: 2
- Points Per Ring: 16

### Tests

**① Distance**
- Distance To: Target Actor
- Filter Type: Min 500 / Max 800 (Donut 범위 재확인)
- Scoring: Linear Inverse (가까울수록 감점, 지나치게 붙는 것 방지)

**② Trace**
- Mode: Line of Sight
- Context: Target Actor
- Filter: Must Pass (시야 막힌 포인트 제거)

**③ Dot Product**
- Line A: `UEnvQueryContext_StrafeDirection`이 반환하는 좌/우 수직 벡터
- Line B: Querier → TestPoint 방향
- Scoring: Dot 값 1에 가까울수록 (측면일수록) 높은 점수
- `UEnvQueryContext_StrafeDirection`은 BB의 `bStrafeLeft`를 읽어 좌/우 결정

---

## 5. C++ 클래스 상세

### 5-1. `UEnvQueryContext_StrafeDirection`

```cpp
// Querier→Target 방향 벡터에 CrossProduct(FVector::UpVector)를 적용해
// 좌(bStrafeLeft=true) 또는 우 수직 벡터를 ContextData에 반환
void ProvideContext(FEnvQueryInstance& QueryInstance,
                   FEnvQueryContextData& ContextData) const override;
```

- BB의 `bStrafeLeft` Bool Key를 읽음
- `FVector::CrossProduct(ForwardToTarget, FVector::UpVector)` → 좌측 벡터
- `bStrafeLeft == false`면 부호 반전 → 우측 벡터

### 5-2. `UBTTask_CircleStrafe`

`UBTTaskNode` 상속 (MoveTo 내장하지 않음, 직접 AIController->MoveTo 호출)

```
ExecuteTask:
  1. BB에서 Target Actor 읽기 (없으면 Failed)
  2. CanMove() 체크 (SKGameplayTags::Character_State_Death, Lock_Input_Move)
  3. FMath::RandBool() → BB bStrafeLeft 저장
  4. UEnvQueryManager::RunEQSQuery 비동기 실행
     → OnQueryFinished 콜백:
        a. 전체 결과 포인트 DrawDebug (유효=초록, 탈락=빨강, 점수 텍스트)
        b. 최적 포인트 DrawDebug (노랑 구체 + 선)
        c. BB StrafeLocation 저장
        d. AIController->SetFocus(Target)
        e. AIController->MoveTo(StrafeLocation, AcceptanceRadius=50)
        f. WaitForMessage(BTTaskMessages::MoveFinished)
  5. MoveTo 완료 → Succeeded / 실패 → Failed
```

DrawDebug는 `#if ENABLE_DRAW_DEBUG` 매크로로 감싸 Ship 빌드 제외.

설정 가능한 UPROPERTY:
- `float StrafeAcceptanceRadius = 50.f`
- `float DebugDrawDuration = 2.f`
- `FBlackboardKeySelector TargetKey`
- `FBlackboardKeySelector StrafeLocationKey`
- `FBlackboardKeySelector StrafeLeftKey`
- `TSoftObjectPtr<UEnvQuery> EQSQuery`

### 5-3. `SpyAnimManagerComponent` 수정

```cpp
// AI의 Velocity를 ActorRotation 기준 로컬 공간으로 변환 후 방향각 계산
float CalcDirectionAngle() const;
// 반환값: -180 ~ 180도 (전방=0, 좌=-90, 우=+90)
// 구현: FMath::Atan2(LocalVel.Y, LocalVel.X) * (180.f / PI)
```

ABP(`ABP_SpyCharacter_Base`)의 Event Graph에서 이 함수를 호출해 `Direction` 변수에 바인딩.  
기존 `BS_Movement_Targeting` 블렌드스페이스(SurvivalAnimationPack L/R/F/B)에 Direction 값을 그대로 연결.

---

## 6. Behavior Tree 구성

```
Selector
 ├─ [Decorator: IsOnCooldown] Sequence
 │    ├─ BTTask_CircleStrafe   ← EQS 실행 + 공전 이동
 │    └─ Wait (0.1s)           ← 연속 실행 방지
 └─ BTTask_MoveToTarget        ← 기존 접근 로직
```

---

## 7. CharacterMovementComponent 설정

AIController 또는 캐릭터 BeginPlay에서 설정:

```cpp
GetCharacterMovement()->bOrientRotationToMovement = false;
GetCharacterMovement()->bUseControllerDesiredRotation = false;
```

회전은 `AIController->SetFocus(Target)`이 전담.  
`AController::GetControlRotation()`이 Focus 방향을 따라가고, Pawn이 이를 추종.

---

## 8. 디버그 시각화 요약

| 시각 요소 | 색상 | 조건 |
|-----------|------|------|
| EQS 유효 포인트 구체 (r=30) | 초록 | Trace 통과 |
| EQS 탈락 포인트 구체 (r=30) | 빨강 | Trace 실패 |
| 포인트별 점수 텍스트 | 흰색 | 항상 |
| 최적 선택 포인트 구체 (r=50) | 노랑 | 선택된 위치 |
| AI → 목적지 선 | 노랑 | 선택된 위치 |

빌트인 EQS 디버그: 런타임 콘솔 `ShowDebug AI` 또는 에디터 AI Debug 탭에서 추가 확인 가능.

---

## 9. 공통 베이스 클래스

`BTTask_MoveToTarget`의 `CanMove()` / `CanTargetAttack()` 헬퍼가 `BTTask_CircleStrafe`에서도 동일하게 필요하다.  
중복 제거를 위해 `UBTTask_SpyBase : public UBTTaskNode`를 신규 작성하고 두 Task가 상속하도록 한다.  
`BTTask_MoveToTarget`은 `UBTTask_MoveTo` → `UBTTask_SpyBase`로 부모 변경 필요 여부를 구현 시 검토.  
(두 클래스의 부모가 달라 단순 상속이 어려우면 헬퍼를 static 유틸 함수로 분리)
