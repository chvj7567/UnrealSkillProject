# plugin-skgas — SKGAS 규칙

> 프로젝트 무관 Gameplay Ability System 코어. ASC·AttributeSet·Ability·Cue·Calculation·공통 태그를 제공.
> 엔진 GAS 모듈(GameplayAbilities/GameplayTags/GameplayTasks)에만 의존하는 독립 플러그인.
> 관련: [unreal-infra.md](unreal-infra.md) · [plugin-modulargameplayactors.md](plugin-modulargameplayactors.md)

---

## 1. 제공 클래스

| 클래스 | 베이스 | 역할 |
|--------|--------|------|
| `USKAbilitySystemComponent` | `UAbilitySystemComponent` | 프로젝트 ASC 베이스, 능력 부여/활성/실패 훅, `AbilityChangedDelegate` |
| `USKAttributeSet` | `UAttributeSet` | Health/MaxHealth/Mana/MaxMana + 변경 이벤트, `ATTRIBUTE_ACCESSORS` |
| `USKGameplayAbility` | `UGameplayAbility` | GA 베이스, 몽타주 콜백·Commit/Cancel/End·`MakeEffectContext` |
| `USKGameplayAbility_SkillAction` | `USKGameplayAbility` | 감지(Weapon/Sphere)·데미지 이펙트 액션 스킬 베이스 |
| `USKGameplayAbility_SkillMove` | `USKGameplayAbility` | 이동 스킬 베이스 |
| `USKCueManager` | `UGameplayCueManager` | 큐 프리로드/풀링 관리 |
| `USKAbilitySystemGlobals` | `UAbilitySystemGlobals` | 커스텀 EffectContext 등 globals |
| `FSKGameplayEffectContext` | `FGameplayEffectContext` | 확장 이펙트 컨텍스트 |
| `SKGameplayTags` (namespace) | — | 재사용 공통 게임플레이 태그 |

---

## 2. 게임플레이 태그

- 재사용 공통(부모/루트) 태그는 `SKGameplayTags` 네임스페이스에 `SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN` 로 선언하고 `.cpp` 에서 `UE_DEFINE_GAMEPLAY_TAG` 로 정의한다.
- **문자열 리터럴로 태그를 직접 참조하지 않는다.**
- 게임별 자식 태그는 각 프로젝트 네임스페이스에서 확장한다 (SKGAS 본체에 게임 전용 태그를 넣지 않는다).
- ASC 문맥에 의존하는 태그 조회는 헬퍼(`SKGameplayTags::GetSkillActionTag(ASC)` / ASC 의 `GetSkillActionTag()`)를 쓴다.

```cpp
//# SKGAS 본체 — 재사용 루트 태그
namespace SKGameplayTags
{
    SKGAS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Skill);
}
```

---

## 3. AttributeSet

- 어트리뷰트는 `FGameplayAttributeData` + `ATTRIBUTE_ACCESSORS(USKAttributeSet, PropertyName)` 매크로로 접근자를 생성한다.
- 레플리케이션: `UPROPERTY(ReplicatedUsing = OnRep_X, BlueprintReadOnly)` + `GetLifetimeReplicatedProps` 등록 + `OnRep_X` 구현 필수.
- 값 변경 반응은 `PostGameplayEffectExecute` 및 `OnXChanged` 델리게이트로 연결한다 (UI·사망 판정 등).
- 새 어트리뷰트는 게임 전용이면 게임 모듈의 `USKAttributeSet` 서브클래스에 추가한다 (공통 스탯만 SKGAS 본체).

---

## 4. GameplayAbility 작성 규칙

- `USKGameplayAbility` (또는 `_SkillAction`/`_SkillMove` 하위)를 상속한다.
- `ActivateAbility` 첫 줄에서 `Super::ActivateAbility(...)` 호출 후, 서버 전용 로직은 `HasAuthority(&ActivationInfo)` 블록 안에서.
- 클라이언트 연출(카메라·사운드·큐)은 `Authority` 블록 밖에서 (unreal-infra.md §2).
- 몽타주 기반 스킬은 base 의 `OnMontageCompleted/Interrupted/Cancelled/BlendOut` 콜백을 오버라이드해 흐름을 잇는다.
- 커스텀 이펙트 컨텍스트가 필요하면 `MakeEffectContext` 를 통해 `FSKGameplayEffectContext` 를 얹는다.

```cpp
void USKGameplayAbility_Example::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (HasAuthority(&ActivationInfo))
    {
        //# 서버 전용 게임플레이 로직 (데미지·상태 변경)
    }

    //# 클라이언트 포함 연출 (몽타주·큐)
}
```

---

## 5. GameplayCue

- 큐는 `USKCueManager` 가 프리로드/풀링을 관리한다. 자주 쓰는 큐는 GC 되지 않게 관리 리스트에 등록된다.
- 큐 실행은 GAS 표준 경로(`GameplayCue_Actor` / `GameplayCue_Static` 루트 태그 하위)를 따른다.
- `USKCueManager` 를 쓰려면 GAS 의 `GlobalGameplayCueManagerClass` config 지정이 필요하다 (§7).

---

## 6. 새 어빌리티 추가 체크리스트

새 GA 를 추가할 때 이 순서대로 작업한다.

### 6-1. 태그 등록
- 재사용 태그면 `SKGameplayTags.h` 에 `UE_DECLARE_GAMEPLAY_TAG_EXTERN`, `SKGameplayTags.cpp` 에 `UE_DEFINE_GAMEPLAY_TAG`.
- 게임 전용 태그면 프로젝트 태그 네임스페이스에 등록. 문자열 리터럴 금지.

### 6-2. GA 클래스 작성
- `USKGameplayAbility`(또는 `_SkillAction`/`_SkillMove`) 상속.
- `ActivateAbility` 에서 `HasAuthority()` 체크 후 서버 로직. 클라 연출은 Authority 밖.

### 6-3. 부여 & 해제
- ASC(`USKAbilitySystemComponent`)에 GA 를 부여하고, 부여 핸들을 트래킹해 장착 해제·사망 시 반드시 제거한다 (누수 금지).
- ASC 조작·GA 부여는 **InitState 초기화(InitAbilityActorInfo) 이후에만** 실행한다 (plugin-modulargameplayactors.md §InitState).

### 6-4. 입력 바인딩 (입력이 필요한 경우)
- InputAction → Gameplay Tag 매핑을 프로젝트 InputConfig 에 추가.
- `InputMappingContext` 에 키 바인딩 추가.
- Enhanced Input 이벤트를 ASC 의 `AbilityLocalInputPressed/Released` 로 연결.

### 6-5. 데이터 등록
- 어빌리티/이펙트/큐 참조는 하드코딩 경로 대신 SKAssetCore 이름 룩업으로 (plugin-skassetcore.md).

---

## 7. Config 등록 (커스텀 globals/cue)

SKGAS 의 커스텀 클래스는 `DefaultGame.ini` 등록이 있어야 실제로 사용된다.

```ini
[/Script/GameplayAbilities.AbilitySystemGlobals]
AbilitySystemGlobalsClassName=/Script/SKGAS.SKAbilitySystemGlobals
```

- 커스텀 EffectContext(`FSKGameplayEffectContext`)는 `USKAbilitySystemGlobals` 를 통해 활성화된다 — 위 등록이 없으면 엔진 기본으로 동작한다.
- `USKCueManager` 를 쓰려면 GAS globals 의 CueManager 클래스 지정도 함께 확인한다.
- `GameplayAbilities` 엔진 플러그인이 활성화돼야 한다 (SKGAS `.uplugin` 이 참조로 전이 활성화).
