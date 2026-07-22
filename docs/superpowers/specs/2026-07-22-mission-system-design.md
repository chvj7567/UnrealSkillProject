# 미션 시스템 설계

- **작성일**: 2026-07-22
- **범위**: 순차 진행 미션 6종 (적 처치 / 파쿠르 / 벽타기 / 그래플링 / 콤보 / 레벨 달성) + 완료 시 경험치 보상 + MainHUD 표시
- **승인 상태**: 사용자 승인 완료 (접근안 A — 태그 기반 범용 트래커)
- **선행 시스템**: 경험치·레벨 (`docs/superpowers/specs/2026-07-21-experience-level-design.md`, 구현 완료)

---

## 1. 목표

정해진 순서대로 하나씩 제시되는 미션을 수행하고, 완료할 때마다 경험치를 받는다. 현재 진행 중인 미션과 진행도를 MainHUD에 표시한다.

**비목표 (이번 범위 밖)**
- 미션 수락/포기 — 순차 자동 진행이므로 수락 개념이 없다
- 미션 진행도의 세이브/로드 영속화 — 세션 내에서만 유지 (프로젝트에 세이브 시스템 자체가 없다)
- NPC 미션 제공자, 미션 분기·선택지
- 경험치 외의 보상 (아이템·스탯·어빌리티 해금)
- 미션 완료 연출 (파티클·사운드·UI 애니메이션)
- 미션 실패·제한시간

---

## 2. 조사로 확정된 사실

설계의 전제이므로 먼저 기록한다. 전부 코드·에셋에서 직접 확인했다.

### 2-1. 모든 GA가 서버에서도 실행된다 (MCP로 CDO 확인)

| GA | NetExecutionPolicy | AbilityTags |
|---|---|---|
| `GA_Vault` | LocalPredicted | `Skill.Move.Vault` |
| `GA_WallClimb` | LocalPredicted | `Skill.Move.Climb` |
| `GA_HanpUp` | LocalPredicted | `Skill.Move.HangUp` |
| `GA_GrappleHook` | LocalPredicted | **(비어 있음)** |
| `GA_SkillA` ~ `GA_SkillF` | LocalPredicted | `Skill.Action.A` ~ `F` |
| `GA_Death` / `GA_Hit` | ServerInitiated | `Skill.Util.Death` / `Skill.Hit` |

`LocalOnly` 인 GA 가 **하나도 없다.** 따라서 서버도 GA 를 실행하고 `USKAbilitySystemComponent::NotifyAbilityActivated`(`SKAbilitySystemComponent.h:25` 에 이미 오버라이드 존재)가 서버에서 호출된다 → **서버 권한 카운팅이 성립한다.**

동시에 이 정책은 **클라이언트에서도 같은 콜백이 발화**한다는 뜻이므로 권한 게이트가 필수다.

### 2-2. `GA_GrappleHook` 에 어빌리티 태그가 없다

`SpyGameplayTags.h:90` 에 `Skill_Move_GrappleHook` 이 선언돼 있으나 GA 에셋의 `AbilityTags` 가 비어 있다. 태그 매칭 방식에서는 **그래플링 미션이 영원히 0 에 머문다.** 선행 수정 대상(§6-1).

### 2-3. 콤보는 별도 GA 가 아니다

`SpyGameplayAbility_SkillAction.cpp:67-85` — 스킬 GA 종료 시 ASC 에 `Character_State_Combo` 루스 태그가 있으면 `SpyComboAssetData` 에서 다음 콤보 태그를 찾아 `HandleGameplayEvent` 로 다음 GA 를 발동한다. 실제로 활성화되는 것은 여전히 `Skill.Action.X` GA 다.

따라서 **"콤보 성공"은 어빌리티 태그로 구분할 수 없고**, 위 지점에 명시적 진행 신호를 심어야 한다. 선행 수정 대상(§6-2).

### 2-4. 킬 예산은 세션당 6 이 상한이다

맵의 `SpawnEnemy` 태그 액터가 6개(`SpawnBot_TargetPoint_A`~`F`), `USpySpawnBotManagerComponent::ServerCreateBots` 가 태그 액터당 1기를 1회만 스폰하고 리스폰 경로가 없다. 플레이어 리스폰 경로도 없다.

레벨 커브가 `[20,40,60]`(최대 레벨 4)이고 킬당 20 이므로 **Lv4 도달에 정확히 6킬이 필요하다.** 즉 처치 미션과 레벨 미션은 **같은 6킬 예산을 나눠 쓴다.** §7 참조.

---

## 3. 배치 — `ASpyPlayerState`

`USpyMissionComponent` 는 `ASpyPlayerState` 에 부착한다. `USpyLevelComponent`(`ASpyCharacter` 부착)와 다른 선택이며, 근거는 **6종 신호가 전부 PlayerState 의 ASC 에 도달**한다는 점이다.

| 미션 | 신호 소스 | 소유 액터 |
|---|---|---|
| 파쿠르 / 벽타기 / 그래플 | `NotifyAbilityActivated` 의 어빌리티 태그 | ASC = PlayerState |
| 적 처치 | 피격자 → 킬러에게 GameplayEvent | ASC = PlayerState |
| 콤보 | SkillAction → 자기 ASC 에 GameplayEvent | ASC = PlayerState |
| 레벨 달성 | `USpyCharacterAttributeSet::OnLevelChanged` | AttributeSet = PlayerState |

폰을 경유하지 않으므로 "클라이언트에서 폰이 아직 도착하지 않았다" 류의 바인딩 타이밍 문제가 구조적으로 발생하지 않는다 (경험치 시스템의 HUD 바인딩에서 실제로 겪은 문제).

### 3-1. 왜 별도 미션 매니저를 두지 않는가

**진행도는 플레이어별이다** (사용자 확정). 세션 공유 협동 목표가 아니다.

이 프로젝트에서 "매니저"는 두 형태다 — `Manager/` 의 GameInstance 서브시스템(`USpyAssetManager`, `USpyUIManager`: 전역 서비스, 플레이어별 상태 없음)과 `ManagerComponent/` 의 액터 부착 컴포넌트(`USpySpawnBotManagerComponent` 는 `ASpyGameState` 에 부착). 즉 세션 전역 상태를 다루더라도 결국 액터 컴포넌트로 구현된다.

미션 진행도는 **플레이어별 상태**이므로 서브시스템(전역 싱글턴)에 두면 PlayerState 를 키로 하는 맵을 직접 관리해야 하고, `GameState` 에 두면 각 플레이어 ASC 의 이벤트를 GameState 로 라우팅하는 배선이 추가된다. PlayerState 컴포넌트는 이 두 비용이 모두 0 이다.

**이 결정을 뒤집는 조건**: 미션을 협동 목표(모두가 하나의 체인을 함께 진행)로 바꾸면 `ASpyGameState` 부착 컴포넌트가 맞다. 그때는 진행 이벤트 라우팅과 보상 XP 분배 규칙이 함께 필요하다.

### 3-2. 플레이어별 진행이 낳는 킬 예산 제약

봇은 세션당 6기이고 리스폰이 없다(§2-4). 진행도가 플레이어별이므로 **다인 세션에서는 킬을 나눠 갖는다** — 2인이면 평균 3킬씩이다.

따라서 **처치 미션의 목표 수는 다인 세션에서도 달성 가능한 값이어야 한다.** 예를 들어 "5마리 처치" 는 2인 세션에서 양쪽 다 완주 불가능하고, 순차 체인이라 그 뒤 미션이 전부 막힌다. game-designer 는 목표 수를 정할 때 이 점을 명시적으로 다뤄야 한다 (1인 기준으로 잡을지, 다인까지 고려할지).

초기화는 ASC 가 준비된 이후여야 한다 — `ASpyPlayerState` 의 ASC 생성·`InitAbilityActorInfo` 흐름 이후 시점에 `InitializeByAbilitySystem(ASC)` 를 호출한다 (plugin-modulargameplayactors §InitState).

---

## 4. 데이터 — `USpyMissionConfig`

`SkillProject/Source/SkillProject/Data/SpyMissionConfig.h|.cpp` — `UDataAsset` 상속. `USpyLevelConfig` 와 같은 패턴으로 컴포넌트의 `EditDefaultsOnly` 프로퍼티에 물린다.

```cpp
UENUM()
enum class ESpyMissionMode : uint8
{
    Accumulate,   //# 이벤트 발생 횟수를 누적해 TargetCount 도달 시 완료
    Threshold,    //# 이벤트가 전달한 값이 TargetCount 이상이면 완료
};

USTRUCT()
struct FSpyMissionEntry
{
    FGameplayTag       MatchTag;          //# Skill.Move.Vault 등
    ESpyMissionMode    Mode;
    int32              TargetCount;
    float              ExperienceReward;
    FText              DisplayName;
};

UCLASS()
class USpyMissionConfig : public UDataAsset
{
    TArray<FSpyMissionEntry> Missions;    //# 배열 인덱스 = 진행 순서
};
```

**`Mode` 가 두 개인 이유**: 파쿠르·처치·콤보는 "N회 수행"(Accumulate)이지만 **레벨 달성만 "Lv4 도달"(Threshold)** 로 성격이 다르다. 레벨을 누적으로 처리하면 "레벨업 횟수"를 세게 되어 의미가 어긋난다.

수치(순서·목표 수·보상 XP·표시 이름)는 이 spec 에서 정하지 않는다 — game-designer 가 기획서에서 확정한다.

---

## 5. 진행 판정 — 순수 함수

`USpyMissionConfig` 의 const 함수로 둔다. 부수효과 없음.

```cpp
USTRUCT()
struct FSpyMissionProgressResult
{
    int32 MissionIndex;     //# 판정 후 진행 중인 미션 인덱스
    int32 Count;            //# 판정 후 누적치
    bool  bCompletedNow;    //# 이번 판정으로 미션이 완료됐는가
    bool  bAllCompleted;    //# 마지막 미션까지 끝났는가
};

FSpyMissionProgressResult ResolveMissionProgress(
    int32 InIndex, int32 InCount, FGameplayTag InEventTag, int32 InAmount) const;
```

동작:
- `InIndex` 가 배열 범위를 벗어나면 전체 완료 상태로 그대로 반환 (더 이상 진행 없음)
- 현재 미션의 `MatchTag` 와 `InEventTag` 가 일치하지 않으면 아무 변화 없이 반환
- **`MatchTag` 비교는 `MatchesTag`(계층 매칭)를 쓴다** — `Skill.Move` 로 파쿠르 계열 전체를 묶는 미션을 데이터만으로 만들 수 있다
- `Accumulate`: `Count += InAmount`, `Count >= TargetCount` 면 완료
- `Threshold`: `Count = InAmount`(누적하지 않고 대치), `Count >= TargetCount` 면 완료
- 완료 시 `MissionIndex + 1`, `Count = 0`. 다음 인덱스가 범위를 벗어나면 `bAllCompleted = true`
- **한 번의 판정으로 최대 1개 미션만 완료한다** — 남은 수량을 다음 미션으로 이월하지 않는다 (미션 종류가 서로 달라 이월이 의미 없다)

`ResolveLevelUp` 과 동일한 패턴이며, Automation 테스트의 대상이다.

---

## 6. 서버 흐름

### 6-0. `USpyMissionComponent`

`SkillProject/Source/SkillProject/System/SpyMissionComponent.h|.cpp` — `UGameFrameworkComponent` 상속. `USpyLevelComponent` 와 대칭 구조.

**복제 상태** (`Replicated` + `GetLifetimeReplicatedProps` 등록)
- `int32 CurrentMissionIndex`
- `int32 CurrentCount`

**공개 델리게이트** (HUD 용)
- `OnMissionProgressChanged(int32 Index, int32 Count, int32 Target)`
- `OnMissionCompleted(int32 CompletedIndex)`
- `OnAllMissionsCompleted()`

**진입점** — 네 소스가 전부 여기로 정규화되어 들어온다.

```cpp
void AddProgress(FGameplayTag InEventTag, int32 InAmount);
```

`AddProgress` 는 **`HasAuthority()` 가 아니면 즉시 반환**한다. 어빌리티 활성화 콜백은 클라이언트에서도 발화하므로(§2-1) 이 게이트가 없으면 중복 카운트된다.

### 6-1. 이동 소스 (파쿠르 / 벽타기 / 그래플)

**⚠ 정정 (인게임 확인에서 결함 발견, 2026-07-22)**

초안은 엔진 `UAbilitySystemComponent::AbilityActivatedCallbacks` 에 바인딩해 활성화된 GA 의 태그로 진행을 올리는 **범용 훅**을 지시했다. **이 방식은 "활성화"와 "실제 수행"을 구분하지 못한다.**

Vault / WallClimb GA 는 **벽이 없어도 활성화된 뒤** 조건 검사에 실패하면 `EndAbility` 한다(§2 조사에서 이미 확인된 사실). 따라서 범용 훅에서는 **허공에 키만 눌러도 미션이 올라갔다.** 인게임 확인에서 사용자가 이 동작을 결함으로 지적했다.

**확정 방식**: 범용 훅을 **제거**하고, 세 이동 GA 가 각자의 **실제 수행 지점**에서 `AddProgress(태그, 1)` 를 직접 밀어 넣는다. 처치·콤보·레벨과 동일한 명시적 신호 방식으로 통일된다.

| GA | 신호 지점 | 서버 발화 근거 |
|---|---|---|
| `SpyGA_SkillMove_Vault` | `OnSyncMotionWarpingData` 의 `HasAuthority` 블록 | `SpyParkourManagerComponent::SetVaultMotionWarpingData` 가 `HasAuthority()` 분기 안에서 `OnRep_VaultMotionWarpingData()` 를 직접 호출 |
| `SpyGA_WallClimb` | `StartWallClimb` 말미, `HasAuthority` 게이트 | `TryToggleClimbAction` 이 트레이스 적중 + `HasAuthority()` 일 때 `OnRep_ClimbWallData()` 를 직접 호출 |
| `SpyGA_GrappleHook` | `ActivateAbility` 의 서버 분기, 케이블 스폰 직후 | 해당 블록이 서버 전용이며 직전에 타겟 유효성을 검사 |

파쿠르 컴포넌트가 `OnRep_*` 를 **서버에서도 직접 호출**하는 패턴(REPNOTIFY 는 클라이언트에서만 자동 호출되므로 서버는 스스로 부른다)이라 세 지점 모두 서버 발화가 코드로 확정된다.

**Vault 는 `CanVaultAction()` 통과 시점이 아니라 워핑 데이터 콜백을 쓴다** — `CanVaultAction()` 은 벽 검출까지만 보장하고, 실제 몽타주·이동은 워핑 데이터가 산출된 뒤 시작되기 때문이다.

**WallClimb 은 `TryToggleClimbAction()` 반환값을 쓰지 않는다** — 그 값은 진행 신호가 아니라 트레이스 결과이고 권한과 무관하게 실행된다. `StartWallClimb` 은 `AddUniqueDynamic` 등록 + `EndWallClimb` 해제라 1회 등반당 정확히 1회 호출된다.

**범용 훅을 남기지 않는 이유**: 활성화와 실행을 구분할 수 없는 경로가 남아 있으면, 이후 `Skill.Move.*` 계열 미션을 추가하는 사람이 같은 함정에 다시 빠진다.

**`GA_GrappleHook` 의 `AbilityTags` 부여는 미션 동작에 더 이상 필요하지 않다** (§2-2 는 범용 훅 전제의 지적이었다). 태그 자체는 미션 `MatchTag` 로 계속 쓰이지만 GA 에셋 쪽 부여와는 무관해졌다. 이미 부여해 두었으므로 되돌리지 않는다.

### 6-2. 콤보 소스

**⚠ 정정 (gameplay-programmer 조사 + MCP 실측으로 확정)**

초안은 `SpyGameplayAbility_SkillAction::InputPressed` 의 콤보 체인 확정 지점(67-85행)에 훅을 두라고 지시했다. **이 위치는 데디케이티드 서버에서 동작하지 않는다.**

`InputPressed` 가상 함수가 원격 클라이언트의 입력으로 서버 인스턴스에서 실행되려면 어빌리티의 `bReplicateInputDirectly` 가 참이어야 한다(엔진 기본값 거짓). MCP 로 `GA_SkillA` ~ `GA_SkillF` 6개의 CDO 를 조회한 결과 **전부 `bReplicateInputDirectly = False`** 이고, C++ 에도 이 값을 설정하는 코드가 없다.

결과: 데디케이티드 서버에서는 원격 플레이어의 콤보 `AddProgress` 가 **아예 호출되지 않는다.** 리슨 서버에서는 호스트 플레이어만 동작한다. 순차 체인이므로 미션3 이 막히면 4·5·6 이 화면에 뜨지도 않는다.

**확정 방식**: 훅을 `USpyGameplayAbility_SkillAction::ActivateAbility`(이미 오버라이드돼 있음)로 옮긴다.

콤보 연결은 `HandleGameplayEvent(콤보태그)` 로 다음 스킬 GA 를 발동시키는데, 이 트리거 활성화는 GAS 가 `ServerTryActivateAbilityWithEventData` 로 **TriggerEventData 와 함께 서버에 전달**한다. 따라서 서버의 `ActivateAbility` 에서 확정적으로 잡힌다.

판정 조건:
- `HasAuthority(&ActivationInfo)` 이고
- `TriggerEventData != nullptr` 이며 (최초 입력 활성화는 트리거 이벤트가 없다)
- `TriggerEventData->EventTag` 가 **콤보 태그 5종 중 하나**일 때

→ `AddProgress(Event_Mission_Combo, 1)`

**부모 태그 매칭을 쓰면 안 된다.** 콤보 태그는 `Skill.Util.Combo1` ~ `Combo5` 이고 형제로 `Skill.Util.Death` 가 있어(`SpyGameplayTags.cpp:79-84`), 부모 `Skill.Util` 로 매칭하면 사망까지 콤보로 집계된다. 5종을 담은 `FGameplayTagContainer` 로 명시 판정한다.

- **콤보 1회의 정의는 유지된다**: 최초 활성화는 트리거 이벤트가 없어 세지 않고 연결만 센다. 3연타면 2회.

**게이트 2 는 문제없음이 확인됐다**: 콤보 창을 여는 `SpyAnimNotify_State_Combo` 가 서버에서 실행되려면 스켈레탈 메시가 pose 를 tick 해야 하는데, `BP_SpyCharacter` 의 메시가 `VisibilityBasedAnimTickOption = AlwaysTickPoseAndRefreshBones`(MCP 확인)라 서버에서도 항상 틱한다.

### 6-3. 처치 소스

피격자 측에서 킬러를 해석해 **킬러의 ASC 에 GameplayEvent 를 보낸다.** 경험치 시스템의 `USpyLevelComponent::HandleDeath` 가 이미 킬러 ASC 를 해석하고 있으므로(PlayerState → Pawn 폴백, 자살 제외 포함) **같은 지점에서 이벤트를 함께 발신**한다.

새 태그 `Event_Mission_Kill` 을 등록한다. 킬러의 미션 컴포넌트가 받아 `AddProgress(Event_Mission_Kill, 1)`.

- 이미 `bDeathRewardGranted` 로 1회 지급이 보장되는 블록 안이므로 중복 발신이 없다.
- 자살·환경 피해는 기존 로직이 걸러낸다.

### 6-4. 레벨 소스

**⚠ 정정 (design-reviewer R1 지적, 코드 재확인 완료)**

초안은 `USpyCharacterAttributeSet::OnLevelChanged` 구독을 지시했으나 **이 델리게이트는 서버에서 발화하지 않는다.** 확인 결과:

| 브로드캐스트 지점 | 실행 조건 |
|---|---|
| `SpyCharacterAttributeSet.cpp:43` | `PostGameplayEffectExecute` 의 Level 분기 — **`Level` 을 수정하는 GE 가 없어 실행되지 않는다** |
| `SpyCharacterAttributeSet.cpp:72` | `OnRep_Level` — **클라이언트 전용** |

서버는 `USpyLevelComponent::TryLevelUp`(`SpyLevelComponent.cpp:217`)에서 `SetNumericAttributeBase` 로 레벨을 바꾸는데, 이 경로는 GE 가 아니므로 `PostGameplayEffectExecute` 를 타지 않는다. 그대로 두면 **미션2(레벨)가 어떤 세션 구성에서도 진행되지 않고, 순차 체인이므로 그 뒤 미션이 전부 막힌다.**

**확정 방식**: 서버에서 실제로 발화하는 `USpyLevelComponent::OnLevelChanged`(3-param 컴포넌트 델리게이트, `SpyLevelComponent.cpp:243` 에서 승급 시 브로드캐스트)를 신호원으로 쓴다. 다만 `USpyLevelComponent` 는 **캐릭터**에, `USpyMissionComponent` 는 **PlayerState** 에 있으므로, 처치 신호(§6-3)와 동일하게 **레벨 컴포넌트가 미션 컴포넌트를 찾아 진행을 밀어 넣는다.**

- `TryLevelUp` 의 승급 블록(이미 서버 권한이 보장된 지점)에서 소유 폰의 `GetPlayerState()` → `USpyMissionComponent::FindMissionComponent` → `AddProgress(Event_Mission_Level, 새 레벨)`.
- `Threshold` 모드이므로 누적하지 않고 현재 레벨로 대치된다.
- 이 방식은 §6-3(처치)과 같은 배선 패턴이라 일관적이고, PlayerState 쪽에서 폰 수명에 바인딩할 필요가 없다는 §3 의 장점도 유지된다.
- 새 태그 `Event_Mission_Level` 등록.

**기각한 대안**: 미션 컴포넌트가 폰의 `USpyLevelComponent` 를 찾아 구독하는 방식 — 폰 스폰·교체 타이밍에 바인딩해야 해서 §3 에서 제거한 폰 의존성이 되살아난다.

### 6-5. 완료 보상

미션 완료 시 **기존 `USpyGE_ExperienceGain` 을 재사용**해 자기 ASC 에 적용한다. 매그니튜드는 `SetByCaller(Data_Experience_Gain)` 로 `ExperienceReward` 를 전달한다. **새 GE 클래스를 만들지 않는다.**

### 6-6. 재진입 가드

**경로가 실재한다**: 미션 완료 → 경험치 GE → 레벨업 → `OnLevelChanged` → `AddProgress(Level)` → 레벨 미션 완료 → 또 경험치 GE → …

`bProcessingProgress` 가드로 `AddProgress` 의 중첩 실행을 막는다. 가드에 걸린 이벤트는 **버리지 않고 큐에 쌓아 현재 처리가 끝난 뒤 순차 처리**한다 — 그냥 버리면 레벨 미션 완료가 유실될 수 있다.

---

## 7. 킬 예산 제약 (기획 단계 필수 고려)

§2-4 의 사실로부터:

- 세션당 획득 가능 킬 = **6**
- 6킬 = 120 경험치 = 정확히 Lv4(최대 레벨)
- **처치 미션과 레벨 미션은 같은 예산을 나눠 쓴다**

따라서 "적 3마리 처치" 미션과 "Lv4 달성" 미션을 같은 체인에 넣으면, 3킬을 처치 미션에 쓰더라도 레벨은 킬 총량에만 의존하므로 6킬 전부를 소진해야 Lv4 에 닿는다. **미션 완료 보상 경험치가 이 계산을 바꾼다** — 미션 XP 가 더해지면 6킬 이전에 Lv4 에 도달할 수 있다.

game-designer 는 미션 순서·목표 수·보상 XP 를 정할 때 이 상호작용을 **수치로 검산**해야 한다. 순차 체인 전체가 6킬 안에서 완주 가능해야 한다.

---

## 8. UI

`SkillProject/Source/SkillProject/UI/SpyMainHUD.h|.cpp` 확장.

- `meta = (BindWidgetOptional)` 로 `TObjectPtr<UTextBlock> Txt_MissionName`, `TObjectPtr<UTextBlock> Txt_MissionProgress`
- 순차 진행이라 **한 번에 하나만 표시**하면 되므로 리스트/repeater 위젯이 필요 없다
- 표시: 이름 = `DisplayName`, 진행도 = `{Count} / {TargetCount}`
- 전체 완료 시 완료 문구로 대치
- 바인딩은 경험치 HUD 와 동일한 방식(로컬 PlayerState 의 미션 컴포넌트 구독 + 즉시 1회 갱신 + 재시도 상한)을 따른다

**위젯 배치는 사용자가 디자이너에서 직접 수행한다.** 스크립트로 위젯을 생성하면 GUID 가 부여되지 않아 별도 보정이 필요하다(경험치 HUD 작업에서 확인).

---

## 9. 변경 파일 목록

**신규**
- `Data/SpyMissionConfig.h|.cpp`
- `System/SpyMissionComponent.h|.cpp`
- `System/Tests/SpyMissionTests.cpp`

**수정**
- `System/SpyPlayerState.h|.cpp` — 컴포넌트 생성 + ASC 초기화 연결
- `Util/SpyGameplayTags.h|.cpp` — `Event_Mission_Kill` / `Event_Mission_Combo` / `Event_Mission_Level`
- `AbilitySystem/Skill/SpyGameplayAbility_SkillAction.cpp` — 콤보 성공 이벤트 발신
- `AbilitySystem/Skill/Move/SpyGA_SkillMove_Vault.cpp` — 넘기 수행 신호 (§6-1)
- `AbilitySystem/Movement/SpyGA_WallClimb.cpp` — 벽타기 수행 신호 (§6-1)
- `AbilitySystem/Movement/SpyGA_GrappleHook.cpp` — 그래플 수행 신호 (§6-1)
- `System/SpyGameMode.cpp` · `Data/SpyAssetNames.h` — `PlayerStateClass` 를 에셋 이름 룩업으로 전환 (BP 기본값이 런타임에 반영되지 않던 문제)
- `Character/SpyLevelComponent.cpp` — 처치 이벤트 발신(`HandleDeath` 안) **+ 레벨 승급 이벤트 발신**(`TryLevelUp` 승급 블록, §6-4)
- `UI/SpyMainHUD.h|.cpp` — 미션 표시

**에셋 (사용자 / MCP)**
- `GA_GrappleHook` 에 `Skill.Move.GrappleHook` 태그 부여
- `SpyMissionConfig` DataAsset 생성 + 미션 배열 입력
- `BP_SpyPlayerState` 의 미션 컴포넌트에 Config 지정
- `WBP_MainHUD` 에 `Txt_MissionName` / `Txt_MissionProgress` 배치

---

## 10. 테스트

**Unreal Automation** — `System/Tests/SpyMissionTests.cpp`, `#if WITH_DEV_AUTOMATION_TESTS` 로 감싸고 `"SkillProject.System.Mission.<케이스>"` 로 등록 (기존 `SpyAICircleStrafeTests.cpp` 스타일, `EditorContext | ProductFilter`).

대상은 `ResolveMissionProgress` 순수 함수.

| 케이스 | 기대 |
|---|---|
| 태그 불일치 | 변화 없음 |
| Accumulate 진행 중 | Count 증가, 미완료 |
| Accumulate 정확 도달 | 완료, 다음 인덱스로, Count 0 |
| Accumulate 초과 도달 | 완료, 초과분 이월 없음 |
| Threshold 미달 / 도달 | 값 대치, 도달 시에만 완료 |
| 마지막 미션 완료 | `bAllCompleted = true` |
| 전체 완료 후 추가 이벤트 | 변화 없음, 크래시 없음 |
| 빈 Config | 크래시 없이 전체 완료 상태 |
| 계층 태그 매칭 | `Skill.Move` 미션이 `Skill.Move.Vault` 이벤트에 반응 |

**커버 불가** (컴포넌트 측 — ASC·복제 의존): 권한 게이트, 재진입 가드와 큐, GameplayEvent 라우팅, 보상 GE 적용, 클라이언트 복제 표시. 인게임 확인 대상이다.

**인게임 확인**: 1인 PIE 로 순차 체인을 처음부터 완주. 각 미션 완료 시 HUD 문구 전환 + 경험치 증가 확인. 2인 PIE 로 클라이언트 표시 동기화 확인.
