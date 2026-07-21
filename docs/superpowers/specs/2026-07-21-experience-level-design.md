# 경험치 · 레벨 시스템 설계

- **작성일**: 2026-07-21
- **범위**: 경험치 획득 → 레벨업 → 스탯 성장 → MainHUD 표시
- **승인 상태**: 사용자 승인 완료 (접근안 A — GAS 어트리뷰트 + 전용 컴포넌트)

---

## 1. 목표

적을 처치해 경험치를 얻고, 임계치를 넘으면 레벨이 오르며 `MaxHealth`/`MaxMana` 가 성장한다. 현재 레벨과 경험치 진행도를 MainHUD 에 표시한다.

**비목표 (이번 범위 밖)**
- 데미지 기여도 분배 — 킬 판정만 사용
- 레벨업에 따른 어빌리티 해금
- 레벨업 연출(파티클·사운드·UI 애니메이션)
- 경험치의 세이브/로드 영속화 — 세션 내에서만 유지

---

## 2. 접근 방식

경험치·레벨을 **GAS 어트리뷰트**로 두고, 판정 로직은 `SpyHealthComponent` 와 동일한 형태의 전용 컴포넌트가 맡는다.

**선택 근거**: 이 프로젝트의 모든 스탯이 `AttributeSet → OnXChanged → UI` 로 흐른다. 경험치를 같은 경로에 태우면 레플리케이션이 공짜로 따라오고, 경험치 부여가 서버 권한 GameplayEffect 로 자연스럽게 처리된다.

**기각한 대안**
- **PlayerState 레플리케이트 프로퍼티** — 구현은 짧지만 스탯 성장을 GAS 밖에서 처리해야 해 어트리뷰트 흐름과 이원화된다.
- **ExecCalculation 기반 레벨업** — 데이터 주도적이나 현 규모에 과하고 디버깅 비용이 크다.

---

## 3. 데이터 계층

### 3-1. `USpyCharacterAttributeSet` 확장

`SkillProject/Source/SkillProject/Character/SpyCharacterAttributeSet.h|.cpp`

기존 `Health` 와 **동일 패턴**으로 어트리뷰트 3개를 추가한다 — `UPROPERTY(ReplicatedUsing = OnRep_X, BlueprintReadOnly, Category = "Attributes")` + `ATTRIBUTE_ACCESSORS` + `GetLifetimeReplicatedProps` 등록 + `OnRep_X` 구현 + `mutable FSKAttributeEvent OnXChanged`.

| 어트리뷰트 | 의미 |
|---|---|
| `Experience` | 현재 레벨 구간 내 누적 경험치 |
| `MaxExperience` | 다음 레벨까지 필요한 경험치 |
| `Level` | 현재 레벨 (1 시작) |

`MaxExperience` 를 어트리뷰트로 두는 이유: 클라이언트가 레벨 커브를 알지 못해도 `Experience / MaxExperience` 만으로 HUD 진행도를 계산할 수 있다. 서버가 레벨업 시 갱신하면 복제된다.

`PostGameplayEffectExecute` 에서 `Experience` 는 음수로 내려가지 않게 클램프한다. 레벨업 판정 자체는 AttributeSet 이 아니라 `USpyLevelComponent` 가 한다 (AttributeSet 이 Config 를 알 필요가 없게).

### 3-2. `USpyLevelConfig` (신규 DataAsset)

`SkillProject/Source/SkillProject/Data/SpyLevelConfig.h|.cpp` — `UDataAsset` 상속, `USpyMovementConfig` 와 같은 형태.

| 프로퍼티 | 타입 | 기본값 | 의미 |
|---|---|---|---|
| `ExperienceToNextLevel` | `TArray<float>` | (에디터 입력) | 인덱스 `i` = 레벨 `i+1` → `i+2` 승급에 필요한 경험치. **배열 길이 + 1 = 최대 레벨** |
| `MaxHealthPerLevel` | `float` | `10.f` | 레벨업 1회당 `MaxHealth` 증가량 |
| `MaxManaPerLevel` | `float` | `5.f` | 레벨업 1회당 `MaxMana` 증가량 |
| `bFullHealOnLevelUp` | `bool` | `true` | 레벨업 시 `Health`/`Mana` 를 최대치로 회복 |
| `ExperienceRewardPerLevel` | `float` | `20.f` | 처치 보상 = 처치당한 대상의 `Level` × 이 값 |

접근 방식은 `USpyCharacterConfig` / `USpyMovementConfig` 선례를 따른다 — `USpyLevelComponent` 의 `UPROPERTY(EditDefaultsOnly)` 로 물리고 캐릭터 BP 기본값에서 지정한다.

### 3-3. 게임플레이 태그

`Util/SpyGameplayTags.h|.cpp` 에 SetByCaller 용 데이터 태그를 등록한다 (문자열 리터럴 금지, cpp-style·plugin-skgas §2).

- `Data_Experience_Gain`
- `Data_Level_MaxHealthGrowth`
- `Data_Level_MaxManaGrowth`

### 3-4. GameplayEffect 클래스 (C++ 정의, 에디터 에셋 없음)

`SkillProject/Source/SkillProject/AbilitySystem/Effect/` 신규 폴더.

| 클래스 | Duration | 모디파이어 |
|---|---|---|
| `USpyGE_ExperienceGain` | Instant | `Experience` `Add`, `SetByCaller(Data_Experience_Gain)` |
| `USpyGE_LevelGrowth` | Instant | `MaxHealth` `Add` + `MaxMana` `Add`, 각 `SetByCaller` |

프로젝트에 SetByCaller 선례가 없어 새로 도입한다. BP 에셋을 만들 필요가 없도록 생성자에서 모디파이어를 구성한다.

---

## 4. 서버 흐름

### 4-1. `USpyLevelComponent` (신규)

`SkillProject/Source/SkillProject/Character/SpyLevelComponent.h|.cpp` — `UGameFrameworkComponent` 상속. `USpyHealthComponent` 와 대칭 구조로 작성한다 (`FindLevelComponent` 정적 헬퍼, `InitializeByAbilitySystem` / `UnInitializeByAbilitySystem`, `OnUnregister` 정리).

**생성·초기화 지점** — `ASpyCharacter` 생성자에서 `CreateDefaultSubobject`(HealthComponent 바로 아래), `OnAbilitySystemInitialized()` 에서 `InitializeByAbilitySystem`, `OnAbilitySystemUninitialized()` 에서 해제. 즉 ASC 초기화(InitState) 이후에만 동작한다 (plugin-modulargameplayactors §InitState).

**공개 델리게이트** (HUD·연출용, `DECLARE_DYNAMIC_MULTICAST_DELEGATE`)
- `OnExperienceChanged(USpyLevelComponent*, float OldValue, float NewValue)`
- `OnLevelChanged(USpyLevelComponent*, int32 OldLevel, int32 NewLevel)`

### 4-2. 경험치 획득

1. `InitializeByAbilitySystem` 에서 자기 소유 캐릭터의 `USpyHealthComponent::OnDeath` 를 구독한다.
2. `HandleDeath(OwningActor, CauserActor)` 는 **`HasAuthority()` 가 아니면 즉시 반환**한다 (`OnDeath` 는 클라이언트의 `OnRep` 경로에서도 발화하므로 필수).
3. 킬러 ASC 해석 — `OwningActor`(= DamageInstigator) 가 `PlayerState` 일 수 있어 `GetPawn()` 폴백을 둔다. `SpyHealthComponent::HandleHealthChanged` 의 기존 폴백 코드와 동일한 방식.
4. 킬러가 자기 자신이면 무시한다 (자살·환경 피해로 경험치를 얻지 못하게).
5. 킬러 ASC 에 `USpyGE_ExperienceGain` 을 적용한다. 매그니튜드 = **처치당한 쪽(자신)의 `Level` × `ExperienceRewardPerLevel`**.
6. 중복 지급 방지 — `bDeathRewardGranted` 플래그를 두고 1회만 지급한다 (`Health` 가 0 이하로 여러 번 갱신되며 `OnDeath` 가 반복 발화할 수 있음).

### 4-3. 레벨업 판정

`OnExperienceChanged` (AttributeSet 델리게이트)를 구독하고, **서버에서만** 다음을 수행한다.

```
while (Level < MaxLevel && Experience >= MaxExperience)
{
    Experience -= MaxExperience        //# 초과분 이월
    Level      += 1
    MaxExperience = 커브[Level - 1]
    USpyGE_LevelGrowth 적용 (MaxHealthPerLevel, MaxManaPerLevel)
    if (bFullHealOnLevelUp) Health = MaxHealth, Mana = MaxMana
    OnLevelChanged 브로드캐스트
}
```

- **최대 레벨 도달** 시 루프를 빠져나오고 `Experience` 를 `MaxExperience` 로 클램프한다 (진행도 바가 꽉 찬 상태로 고정). 이때 `MaxExperience` 는 마지막으로 유효했던 커브값을 유지한다 — `커브[Level - 1]` 은 최대 레벨에서 배열 범위를 벗어나므로 인덱스가 유효할 때만 갱신한다.
- **재진입 방지** — 루프 안에서 어트리뷰트를 바꾸면 `OnExperienceChanged` 가 다시 들어온다. `bProcessingLevelUp` 가드 플래그로 중첩 실행을 막는다.
- `ExperienceToNextLevel` 배열이 비었거나 Config 가 `nullptr` 이면 레벨업을 시도하지 않고 경고 로그만 남긴다.

### 4-4. 초기 상태

`InitializeByAbilitySystem` 에서 서버 권한일 때 `Level = 1`, `Experience = 0`, `MaxExperience = 커브[0]` 을 세팅한다.

---

## 5. HUD

`SkillProject/Source/SkillProject/UI/SpyMainHUD.h|.cpp`

**위젯 바인딩** — `meta = (BindWidgetOptional)` 로 선언한다. Optional 이라 아직 위젯을 배치하지 않은 기존 `WBP_MainHUD` 가 깨지지 않는다.
- `TObjectPtr<UProgressBar> PB_Exp`
- `TObjectPtr<UTextBlock> Txt_Level`

**바인딩 타이밍이 이 설계의 핵심 리스크다.** ASC 와 AttributeSet 은 `ASpyPlayerState` 에 있어, 클라이언트에서는 MainHUD 의 `NativeConstruct` 시점에 아직 도착하지 않았을 수 있다. 구축 시점에 어트리뷰트가 있다고 가정하면 "리슨 서버에서는 되는데 클라이언트에서는 빈칸" 버그가 난다.

따라서:
1. `NativeConstruct` 에서 로컬 플레이어의 Pawn 에 대해 `UGameFrameworkComponentManager::AddExtensionHandler` 로 기존 `ASpyPlayerState::NAME_AbilityReady` 신호를 대기한다.
2. 준비되면 `USpyLevelComponent` 의 `OnExperienceChanged` / `OnLevelChanged` 를 구독하고, **즉시 현재값으로 1회 갱신**한다 (구독 전에 이미 변한 값을 놓치지 않게).
3. `NativeDestruct` 에서 확장 핸들과 델리게이트를 모두 해제한다.

**표시 규칙**
- `PB_Exp` = `Experience / MaxExperience`, `MaxExperience <= 0` 이면 `0` (`USpyHPBar::UpdateHP` 의 0 나눗셈 방어와 동일한 처리).
- `Txt_Level` = `Lv.{Level}`.

HP 는 현재 월드 공간 위젯(`USpyHPBar`)이 담당한다 — 이번 작업에서 MainHUD 로 옮기지 않는다.

---

## 6. 변경 파일 목록

**신규**
- `Data/SpyLevelConfig.h|.cpp`
- `Character/SpyLevelComponent.h|.cpp`
- `AbilitySystem/Effect/SpyGE_ExperienceGain.h|.cpp`
- `AbilitySystem/Effect/SpyGE_LevelGrowth.h|.cpp`
- `SkillProject/Source/SkillProject/Character/Tests/SpyLevelTests.cpp`

**수정**
- `Character/SpyCharacterAttributeSet.h|.cpp` — 어트리뷰트 3개
- `Character/SpyCharacter.h|.cpp` — 컴포넌트 생성 + 초기화/해제 연결
- `UI/SpyMainHUD.h|.cpp` — 위젯 바인딩 + 구독
- `Util/SpyGameplayTags.h|.cpp` — SetByCaller 태그 3개

**에디터 작업 (사용자 몫)**
- `DA_SpyLevelConfig` 생성 후 캐릭터 BP 의 `LevelComponent → LevelConfig` 에 지정
- `WBP_MainHUD` 에 `PB_Exp` / `Txt_Level` 배치

---

## 7. 테스트

**Unreal Automation** — `Character/Tests/SpyLevelTests.cpp`, `#if WITH_DEV_AUTOMATION_TESTS` 로 감싸고 `"SkillProject.Character.Level.<케이스>"` 로 등록한다 (기존 `SpyAICircleStrafeTests.cpp` 스타일).

레벨업 계산은 Config 와 현재값만 받는 순수 함수로 분리해 테스트 가능하게 만든다.

| 케이스 | 기대 |
|---|---|
| 임계치 미달 | 레벨 유지, 경험치 누적 |
| 임계치 정확히 도달 | 레벨 +1, 경험치 0 |
| 한 번에 2레벨 이상 | 레벨 +2, 잔여 경험치 정확히 이월 |
| 최대 레벨에서 추가 획득 | 레벨 유지, 경험치 `MaxExperience` 클램프 |
| Config `nullptr` / 빈 커브 | 크래시 없이 레벨 유지 |

**인게임 확인** — 봇 처치 → HUD 경험치 바 증가 → 레벨업 시 `Lv.` 숫자 증가 + `MaxHealth` 상승 + 풀회복. 데디케이티드 서버 2 클라이언트로 클라이언트 표시 동기화 확인.
