# plugin-modulargameplayactors — ModularGameplayActors 규칙

> GameFeature/GameFramework 컴포넌트 확장을 지원하는 모듈형 액터 베이스 모음.
> 캐릭터·컨트롤러 등 게임 액터의 베이스로 사용하고, 그 위에서 InitState 흐름으로 ASC 를 초기화한다.
> 관련: [unreal-infra.md](unreal-infra.md) · [plugin-skgas.md](plugin-skgas.md)

---

## 1. 제공 클래스

| 클래스 | 베이스 | 용도 |
|--------|--------|------|
| `AModularCharacter` | `ACharacter` | 확장 가능 캐릭터 베이스 |
| `AModularPawn` | `APawn` | 확장 가능 폰 베이스 |
| `AModularPlayerController` | `APlayerController` | 플레이어 컨트롤러 베이스 |
| `AModularPlayerState` | `APlayerState` | 플레이어 스테이트 베이스 |
| `AModularGameMode` / `AModularGameState` | 각 표준 베이스 | 게임 모드/스테이트 베이스 |
| `AModularAIController` | `AAIController` | AI 컨트롤러 베이스 |

- 이들은 `PreInitializeComponents`/`BeginPlay`/`EndPlay` 에서 `UGameFrameworkComponentManager` 에 자신을 등록해 GameFeature 컴포넌트 확장을 받는다.

---

## 2. 액터 베이스 사용 규칙

- 게임 캐릭터/컨트롤러 등은 표준 `ACharacter`/`APlayerController` 대신 대응하는 `Modular*` 베이스를 상속한다.
- 컴포넌트는 `BeginPlay` 하드코딩이 아니라 데이터(Character 에셋 등) 목록을 읽어 `NewObject & RegisterComponent` 로 런타임 추가한다 (unreal-infra.md §2).
- `Super::` 호출 누락 주의 — `PreInitializeComponents`/`BeginPlay`/`EndPlay` 오버라이드 시 반드시 base 호출 (cpp-style.md).

---

## 3. InitState 초기화 흐름 (권장 소비 패턴)

이 플러그인은 `Modular*` 베이스와 `UGameFrameworkComponentManager` 등록만 제공한다. 아래 InitState 흐름은 플러그인이 강제하는 게 아니라, 이 베이스 위에서 **게임이 구현해야 하는 권장 패턴**이다 (Pawn 확장 컴포넌트는 게임 모듈에서 작성).

ASC·GAS 초기화는 액터 스폰 직후가 아니라 **InitState 단계**에서 한다.

- Pawn 확장 컴포넌트(`IGameFrameworkInitStateInterface` 구현, 게임 모듈 작성)가 초기화 단계를 관장하게 한다:
  `Spawned → DataAvailable → DataInitialized → GameplayReady`.
- 캐릭터 데이터 레플리케이트 완료 + Controller 연동 후 `DataInitialized` 단계에서 `InitAbilityActorInfo` 를 호출한다.
- **GA 추가·ASC 조작 코드는 반드시 이 흐름(InitAbilityActorInfo) 이후에 실행한다.** 그 전에 부여하면 ActorInfo 미설정으로 깨진다 (plugin-skgas.md §6-3).

체크리스트:
- [ ] 게임 액터가 `Modular*` 베이스를 상속하는가?
- [ ] ASC 초기화·GA 부여가 `DataInitialized`(InitAbilityActorInfo) 이후에 실행되는가?
- [ ] 오버라이드에서 `Super::` 를 호출했는가?
