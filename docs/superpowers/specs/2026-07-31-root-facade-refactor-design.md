# §13 루트 파사드 준수 리팩터링 — 설계 spec

- **작성일**: 2026-07-31
- **성격**: 동작 보존 리팩터링. 새 게임플레이 기능 없음.
- **근거 룰**: `.claude/rules/cpp-style.md` §13(루트 파사드) · §12(공용 Interface) · §8(종속성 최소화)
- **파이프라인**: `/start-develop` (범위 1~4 전부, public getter 정리 제외 — 사용자 확정)

---

## 1. 문제

> **표기**: `①~⑤` 는 cpp-style §13 **체크리스트 항목**, `1~4` 는 이 spec 의 **작업 항목**이다. 서로 대응하지 않는다.

`ASpyCharacter` 도메인을 §13 체크리스트로 검사한 결과 5개 중 3개가 미준수다.

| 체크 항목 | 결과 |
|---|---|
| ① 외부 진입 클래스가 하나인가 | ❌ 루트를 건너뛴 하위 컴포넌트 직접 접근 12곳 |
| ② 캐싱 핸들이 private/protected 인가 | △ (이번 범위 밖 — public getter 6개) |
| ③ 루트에 대응 인터페이스가 있는가 | ❌ 없음 (`Character/CommonInterface.h` 부재) |
| ④ 하위가 소유자/형제를 탐색하지 않는가 | ❌ 형제→형제 2곳 |
| ⑤ 루트가 Tick 순서를 쥐는가 | ✅ 현재 순서 의존 없음 |

### 위반 지점 전수

**③ 인터페이스 부재** — `ASpyCharacter` 는 `IAbilitySystemInterface` 만 구현.

**① 외부가 하위 컴포넌트를 직접 탐색 (12곳)**

| 파일:줄 | 대상 컴포넌트 |
|---|---|
| `SpyGA_SkillMove_Vault.cpp:38,71,100` | Parkour |
| `SpyGA_SkillMove_Vault.cpp:120` | MotionWarping(엔진) |
| `SpyGA_SkillMove_HangUp.cpp:37,56,84` | Parkour |
| `SpyGA_SkillMove_HangUp.cpp:95` | MotionWarping(엔진) |
| `SpyGA_WallClimb.cpp:86,145` | Parkour |
| `SpyGA_GrappleHook.cpp:33,84,136` | GrappleTargeting |
| `SpyGA_Targeting.cpp:27` · `SpyGA_Death.cpp:20` | Targeting |
| `SpyCharacterAnimInstance.cpp:124` | Targeting |
| `SpyPlayerController.cpp:70,77` | Targeting |

**④ 형제→형제 탐색 (2곳)**

- `SpyCharacterMovementComponent.cpp:80` — `PhysicsRotation`, **매 프레임 회전 경로**. §13 + §8(Tick 경로 탐색 금지) 이중 위반. **최우선.**
- `SpyGrappleUIComponent.cpp:26` — `BeginPlay` 에서 형제 `USpyGrappleTargetingComponent` 탐색 후 델리게이트 구독.

---

## 2. 설계 결정 — 인터페이스 핸들 (프록시 아님)

### 결정

루트가 **하위 컴포넌트의 인터페이스 핸들을 넘겨주고**, 소비자는 그 핸들을 통해 기존 프로토콜을 그대로 수행한다. 루트가 모든 호출을 의도 API 로 재수출(프록시)하지 않는다.

### 근거

파쿠르 상호작용은 게터 호출이 아니라 **상태 프로토콜**이다 (`SpyGA_SkillMove_Vault.cpp`):

```
CanVaultAction() → OnVaultMotionWarpingData.AddDynamic() → SetVaultMotionWarpingData()
  → 콜백에서 SetFreeMoveMode(true) + 워프 타깃 2개 → EndAbility 에서 SetFreeMoveMode(false)
```

GA 쪽에 `bFreeMoveEngaged` 상태까지 남는다. 이걸 루트 메서드로 감싸면 `ASpyCharacter` 가 델리게이트·트리거·모드세터·쿼리를 파쿠르 액션(Vault/HangUp/WallClimb)마다 재수출해야 하고 Grapple·Targeting 까지 붙는다 — §8 "알아야 하는 외부 타입 0~3개" 체크리스트를 룰 자신이 위반하는 결과가 된다.

§13 「금지」가 실제로 금지하는 것은 **탐색과 구체 타입 결합**이다("하위끼리 서로를 구체 클래스로 직접 찾아가기 — 루트가 인터페이스로 주입한다"). 처방은 *인터페이스 주입*이지 *전 호출 프록시*가 아니다. 인터페이스 핸들 방식은 `FindComponentByClass` 와 구체 타입 의존을 둘 다 제거하면서 프로토콜 순서를 건드리지 않으므로, 회귀 위험이 낮고 ①③④를 모두 만족한다.

### 기각된 대안

- **전 호출 프록시** — 캡슐화는 가장 강하지만 루트 비대화 + 프로토콜 재설계라 180° 스핀급 회귀 위험이 실질적.
- **하이브리드**(단순 건은 프록시, 프로토콜은 핸들) — 두 규칙이 섞여 매번 판단이 필요. 일관성 손실 대비 이득 없음.

---

## 3. 아키텍처

### 3-1. 인터페이스 신설 — 도메인 폴더별 2파일

§12 는 "도메인 폴더별 `CommonInterface.h`" 다. 루트는 `Character/`, 매니저 컴포넌트는 `ManagerComponent/` 에 살므로 두 파일로 나눈다. 같은 모듈 내이므로 `Character/CommonInterface.h` 가 `ManagerComponent/CommonInterface.h` 를 include 한다 (역방향 없음).

| 파일 | 정의 | implement |
|---|---|---|
| `ManagerComponent/CommonInterface.h` | `ISpyParkourHost` · `ISpyTargetProvider` · `ISpyGrappleHost` | 각 매니저 컴포넌트 |
| `Character/CommonInterface.h` | `ISpyCharacterRoot` | `ASpyCharacter` |

**순수 가상으로 정의한다** — `UFUNCTION(BlueprintNativeEvent)` + `Execute_` 패턴을 쓰지 않는다. cpp-style §12/§13 예제가 순수 가상 형태이고, 델리게이트 참조 반환(`FSyncMotionWarpingDataDelegate&`)은 BP 노출로 표현할 수 없다.

**이름 충돌 주의** — UHT 는 접두사를 떼고 등록하므로 `ASpyCharacter` 와 `USpyCharacter` 는 둘 다 `SpyCharacter` 가 되어 중복 이름 에러가 난다. 루트 인터페이스는 반드시 접미사(`...Root`)를 유지한다 (cpp-style §13).

### 3-2. 인터페이스 표면 (실제 호출부에서 도출)

```cpp
//# ISpyCharacterRoot — 도메인 유일 진입점
virtual TScriptInterface<ISpyParkourHost>    GetParkourHost() const = 0;
virtual TScriptInterface<ISpyTargetProvider> GetTargetProvider() const = 0;
virtual TScriptInterface<ISpyGrappleHost>    GetGrappleHost() const = 0;

//# 이미 루트에 있던 의도 API — 인터페이스로 승격
virtual void PushCameraCollisionSuppress() = 0;
virtual void PopCameraCollisionSuppress() = 0;

//# 엔진 MotionWarpingComponent 는 우리 인터페이스를 implement 할 수 없다.
//# 탐색 제거를 위해 루트가 얇은 의도 API 하나만 노출한다 (호출부 4곳).
virtual void AddMotionWarpTarget(FName Name, const FVector& Loc, const FRotator& Rot) = 0;
```

```cpp
//# ISpyParkourHost — USpyParkourManagerComponent
virtual bool CanVaultAction() = 0;
virtual void SetVaultMotionWarpingData() = 0;
virtual void SetHangUpMotionWarpingData(const FVector& HitVector) = 0;
virtual bool TryToggleClimbAction() = 0;
virtual void SetFreeMoveMode(bool bInFreeMoveMode) = 0;
virtual FSyncMotionWarpingDataDelegate& OnVaultMotionWarping() = 0;
virtual FSyncMotionWarpingDataDelegate& OnHangUpMotionWarping() = 0;
virtual FSyncClilmbDataDelegate&        OnClimb() = 0;

//# ISpyTargetProvider — USpyTargetingManagerComponent
virtual TWeakObjectPtr<AActor> GetTarget() const = 0;
virtual bool IsTargetValid() const = 0;
virtual void SetCurrentTarget(AActor* NewTarget) = 0;

//# ISpyGrappleHost — USpyGrappleTargetingComponent
virtual AActor* GetLocalCachedTarget() const = 0;
virtual AActor* GetCurrentGrappleTarget() const = 0;
virtual FOnGrappleTargetChanged& OnGrappleTargetChanged() = 0;
```

> 위 목록은 현재 호출부에서 도출한 것이다. 구현 중 HangUp GA 등에서 추가로 필요한 메서드가 나오면 **그 호출부가 실제로 쓰는 것만** 인터페이스에 추가한다. 컴포넌트의 public 메서드를 전부 옮기지 않는다.

### 3-3. 조립점 — `USpyPawnExtensionComponent` 의 `DataInitialized`

InitState 인프라는 이미 존재한다. **새로 만들지 않는다.** 루트가 이 콜백에서 1회 캐싱하고 형제 간 주입까지 수행한다.

```
DataInitialized
  ├─ 루트: CachedParkourHost / CachedTargetProvider / CachedGrappleHost / CachedMotionWarping 캐싱
  ├─ 주입: USpyCharacterMovementComponent ← ISpyTargetProvider    (작업 2)
  └─ 주입: USpyGrappleUIComponent        ← ISpyGrappleHost        (작업 4)
```

캐싱 핸들은 전부 `private` + `UPROPERTY(Transient)` (§13 체크리스트).

### 3-4. 소비자 변경 (작업 3)

`FindComponentByClass<구체타입>` → 루트 인터페이스 경유. **프로토콜 순서·분기·권한 판정은 손대지 않는다.**

- GA 6종 11곳: `Cast<ACharacter>` + 탐색 → `TScriptInterface<ISpyCharacterRoot>` 에서 핸들 획득
- Vault GA 의 `Cast<ASpyCharacter>` → `PushCameraCollisionSuppress` 도 인터페이스 경유로 통일
- `SpyPlayerController.cpp:70,77` · `SpyCharacterAnimInstance.cpp:124` 동일

---

## 4. 회귀 방어 — 유일한 실질 위험

### 위험

**주입은 `DataInitialized` 이후에 성립하지만 `PhysicsRotation` 은 그 전에도 돈다.** 주입 핸들이 널인 프레임에 기존과 다르게 행동하면 "공격 중 BT 가 회전을 놓는 순간 180° 스핀"(`SpyCharacterMovementComponent.cpp:68-72` 주석) 이 재현된다.

### 방어 — "널 → break" 는 틀렸다 (플랜 작성 중 정정)

`PhysicsRotation` 전문(`SpyCharacterMovementComponent.cpp:49-113`)을 확인한 결과, 단순히 널을 기존 `nullptr` 경로에 매핑하면 **새 회귀가 생긴다**.

- 오늘 `FindComponentByClass` 는 컴포넌트 등록 시점(**DataAvailable**)부터 항상 non-null 이다. 즉 `TargetingComp == nullptr → break` 는 **타깃팅 컴포넌트가 아예 없는 캐릭터**에서만 발화한다.
- 주입은 **DataInitialized** 에 성립하므로, 그 사이 구간에서는 컴포넌트를 가진 플레이어도 핸들이 널이다. 이때 `break` 하면 회전이 통째로 사라진다 — 오늘은 같은 구간에서 "타깃 없음" 경로(`bOrientRotationToMovement = true` + `Super::PhysicsRotation`)를 탄다.

두 상황이 널 하나로는 구분되지 않으므로 **해결 플래그로 분리한다.**

| 상태 | 조건 | 행동 | 오늘의 대응 경로 |
|---|---|---|---|
| 주입 전 | `bTargetProviderResolved == false` | "타깃 없음" 경로 (`bOrientRotationToMovement = true` + `Super`) | 컴포넌트 존재 + 타깃 없음 (신규 스폰은 타깃을 가질 수 없음) |
| 주입됨·유효 | 핸들 유효 | 기존 타깃팅 분기 그대로 | 동일 |
| 주입됨·널 | `bTargetProviderResolved == true` + 핸들 널 | `break` | 컴포넌트 자체가 없는 캐릭터 |

`bTargetProviderResolved` 는 주입 함수가 **핸들 유효성과 무관하게** true 로 세팅한다.

### 나머지 소비자

핸들 널일 때의 행동은 기존 "컴포넌트를 못 찾았을 때"와 1:1 대응한다. GA 들이 컴포넌트 부재 시 `EndAbility` 를 부르던 경로는 그대로 `EndAbility`. GA 는 활성화 시점이 항상 `GameplayReady` 이후라 위와 같은 주입 전 구간 문제가 없다.

### 테스트

- `Character/Tests/SpyCharacterAIRotationTests.cpp` **유지**. 삭제·약화 금지.
- 추가 케이스: 주입 전(핸들 널) 프레임에서 `PhysicsRotation` 이 기존과 동일 경로를 타는지.
- 추가 케이스: `DataInitialized` 이후 루트가 3개 핸들을 모두 채우는지, 형제 2곳에 주입이 도달하는지.

---

## 5. 범위

### 포함

1. `Character/CommonInterface.h` + `ManagerComponent/CommonInterface.h` 신설, 4개 인터페이스 정의 및 implement
2. `SpyCharacterMovementComponent.cpp:80` 매 프레임 탐색 제거 (주입)
3. GA 11곳 + PlayerController 2곳 + AnimInstance 1곳을 루트 인터페이스 경유로
4. `SpyGrappleUIComponent` 주입

### 제외 (사용자 명시)

- `GetSpyWeapon` / `GetSpyHealthComponent` / `GetCameraBoom` / `GetFollowCamera` / `GetSpyCharacterMovementComponent` / `GetCharacterConfig` — public getter 6개를 의도 API 로 접는 작업.
- 위 목록 외 주변 코드의 §6(`auto`)·§8 위반. 룰 도입 이전 코드이며 cpp-style 「적용 범위」가 변경 hunk 로 한정한다.
- `SpyGA_GrappleHook.cpp` 의 `UE_LOG` 디버그 출력 — 이번 리팩터링과 무관.

---

## 6. 성공 기준

- §13 체크리스트 ①③④ 통과 (②는 범위 밖으로 남음, ⑤는 이미 통과)
- `SkillProject/Source/SkillProject` 에서 GA·컨트롤러·AnimInstance·형제 컴포넌트의 `FindComponentByClass<USpy*>` 호출 0건
  - 예외: 루트 자신이 자기 컴포넌트를 찾는 `SpyCharacter.cpp:241`, 정적 헬퍼(`FindHealthComponent` 등)는 대상 아님
- 기존 자동화 테스트 전부 통과, 특히 `SpyCharacterAIRotationTests`
- 파쿠르(Vault/HangUp/WallClimb)·그래플·타깃팅 동작 변화 없음
