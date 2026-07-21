# unreal-infra — 재사용 인프라 아키텍처 규칙

> 프로젝트에 종속되지 않는 재사용 플러그인들과, 게임 모듈이 이들을 소비하는 방식·모듈 의존 방향·서버 권한 규약을 정의한다.
> 특정 프로젝트명에 의존하지 않는다 — 어느 게임 모듈이든 아래 플러그인을 동일하게 소비한다.

---

## 0. 재사용 플러그인 인덱스

| 플러그인 | 역할 | 세부 규칙 |
|----------|------|-----------|
| `SKAssetCore` | 에셋 매니저 + 이름→경로 룩업 DataAsset | [plugin-skassetcore.md](plugin-skassetcore.md) |
| `SKUICore` | UI 매니저(open/cache/reuse) + 위젯 베이스 | [plugin-skuicore.md](plugin-skuicore.md) |
| `SKGAS` | GAS 코어(ASC·AttributeSet·Ability·Cue·Tag) | [plugin-skgas.md](plugin-skgas.md) |
| `ModularGameplayActors` | 모듈형 액터 베이스(GameFeature 확장 대응) | [plugin-modulargameplayactors.md](plugin-modulargameplayactors.md) |

- 모든 재사용 로직은 위 플러그인에 두고 게임 모듈에서 중복 구현하지 않는다.
- 새 태그·에셋·어빌리티는 해당 플러그인 규칙 파일의 체크리스트를 따른다.

---

## 1. 모듈 의존 방향

```
게임 모듈 (Runtime)
   ├─→ SKGAS                 → GameplayAbilities / GameplayTags / GameplayTasks / AIModule
   ├─→ SKUICore              → SKAssetCore + UMG / SlateCore
   ├─→ SKAssetCore           → Core / CoreUObject / Engine
   └─→ ModularGameplayActors → ModularGameplay / AIModule
```

- **역방향 참조 금지** — 플러그인이 게임 모듈 헤더를 include 하지 않는다.
- `SKGAS` 는 다른 SK 플러그인에 의존하지 않는 독립 모듈이다 (엔진 GAS 모듈에만 의존).
- `SKUICore` 는 `SKAssetCore` 에 의존한다 (UI 에셋 로드 경유). 이 방향을 역전하지 않는다.
- 새 의존성은 해당 플러그인 `.Build.cs` 의 `PublicDependencyModuleNames` 에 명시한다.
- 게임 모듈이 플러그인을 쓰려면 게임 `.Build.cs` 의존성 + `.uproject` 에 플러그인 활성화가 필요하다 (SK 플러그인은 `.uplugin` 의 `EnabledByDefault: true` 로 자동 활성화).

체크리스트:
- [ ] 재사용 모듈이 게임 모듈을 include 하지 않는가?
- [ ] `.Build.cs` 의존성이 올바른 방향인가? (`SKUICore → SKAssetCore`, 역방향 없음)

---

## 2. 서버 권한 / 레플리케이션

- 게임플레이 상태 변경은 서버에서 실행하고 클라이언트에 레플리케이트한다.
- GA 내에서 `HasAuthority(&ActivationInfo)` 체크 후 서버 전용 로직. 클라 연출(카메라·사운드·큐)은 Authority 블록 밖.
- 레플리케이트 프로퍼티는 `Replicated`(또는 `ReplicatedUsing`) + `GetLifetimeReplicatedProps` 등록 필수.
- 런타임 컴포넌트 추가는 데이터(Character 에셋 등) 목록을 읽어 `NewObject & RegisterComponent` — `BeginPlay` 하드코딩 금지.

```cpp
void USKGameplayAbility_Example::ActivateAbility(...)
{
    Super::ActivateAbility(...);

    if (HasAuthority(&ActivationInfo))
    {
        //# 서버 전용 게임플레이 로직
    }

    //# 클라이언트 포함 연출
}
```

---

## 3. 게임 모듈의 플러그인 소비 패턴

플러그인은 **베이스**만 제공한다. 게임별 구체 타입은 게임 모듈에서 서브클래싱해 소비한다.

- `USKAssetManager` → 프로젝트 전용 매니저로 서브클래싱, `GetAssetData()` 오버라이드 (자세히는 plugin-skassetcore.md).
- `USKUIManager` → 프로젝트 전용 UI 매니저로 서브클래싱 시 leaf 인스턴스만 생성됨 (plugin-skuicore.md).
- `USKAbilitySystemComponent` / `USKAttributeSet` / `USKGameplayAbility` → 게임 캐릭터·어빌리티에서 상속·부여 (plugin-skgas.md).
- `AModularCharacter` / `AModularPlayerController` 등 → 게임 액터의 베이스로 사용, InitState 흐름 위에서 ASC 초기화 (plugin-modulargameplayactors.md).
- 커스텀 GAS globals/cue 등 config 등록이 필요한 항목은 각 플러그인 규칙 파일의 "Config 등록" 절을 따른다.

체크리스트:
- [ ] 게임별 로직을 플러그인 베이스 서브클래스로 구현했는가? (플러그인 본체 수정 지양)
- [ ] ASC 조작·GA 부여가 InitState 초기화 이후에 실행되는가? (plugin-modulargameplayactors.md §InitState)
