# 네트워크 로딩 전환 — 설계 스펙

- 작성일: 2026-07-23
- 상태: 사용자 승인 대기 (brainstorming 산출물)
- 선행: `docs/superpowers/specs/2026-07-22-loading-scene-design.md` (부팅 로딩 씬), 그 구현(Task 1~13)
- 다음 단계: `writing-plans` → 구현 플랜

## 1. 목적

데디케이티드 서버 모델에서 **클라이언트가 로딩 화면을 거쳐 서버에 접속·진입**하도록 로딩 전환 메커니즘을 바로잡는다. 현재 구조는 각 인스턴스가 독립적으로 `OpenLevel` 하여, 네트워크 클라이언트가 서버에서 떨어져 나가 검은 화면에 로딩바 100%로 멈춘다.

## 2. 확정된 근본 원인 (재조사로 규명)

증상: **standalone 2인 실행 시 한 인스턴스(서버/호스트)는 정상 진입하고, 다른 인스턴스(클라이언트)는 검은 화면 + 로딩바 100% 고정.**

코드에 매핑한 원인:

1. `USpyLoadingSubsystem::TransitionToGameplayMap`(`SpyLoadingSubsystem.cpp:227`)이 **authority 체크 없이** `UGameplayStatics::OpenLevel(GetGameInstance(), MapPackageName)`을 호출한다.
2. 서버/호스트에서 `OpenLevel`은 **서버 트래블**이라 클라이언트를 함께 데려간다 → 정상.
3. 네트워크 **클라이언트**에서 `OpenLevel`은 **클라 단독 absolute travel**이라 서버 연결을 끊는다(로그 증거: `Browse: /Game/Spy/Maps/LoadingMap?closed`). 서버 없이 로컬로 DevMap을 열려다 실패 → **검은 화면.**
4. 클라가 전환을 시작(`bTransitionStarted=true`, 바 100% 강제)했지만 접속이 끊겨 `PostLoadMapWithWorld(DevMap)`가 정상 완료되지 않음 → `HandlePostLoadMap`이 UI를 못 닫음 → **100% 바가 검은 화면 위에 잔존.**

이는 선행 로딩 씬 스펙이 **부팅(단일 authority) 화면**으로만 설계돼 네트워크 클라이언트 진입을 다루지 않은 데서 비롯한 범위 공백이다.

## 3. 목표 네트워크 모델 (사용자 확정)

- **데디케이티드 서버.** 서버는 헤드리스로 항상 DevMap에 상주(`ServerDefaultMap=/Game/Spy/Maps/DevMap`, 설정 완료). 서버에는 로딩 서브시스템이 생성되지 않는다(`ShouldCreateSubsystem`이 `IsRunningDedicatedServer()`에서 `false`, 구현 완료).
- **클라이언트**는 부팅 시 `LoadingMap`으로 뜨고(`GameDefaultMap=/Game/Spy/Maps/LoadingMap`, 설정 완료), **config에 지정된 서버 주소로 자동 접속**한다. 메인 메뉴·서버 브라우저는 범위 밖(§9).
- 접속 실패 시 **에러 메시지 + 수동 재시도.**

`DefaultEngine.ini`의 맵 설정은 이미 이 모델에 맞다 — ini 변경 없음.

## 4. 클라이언트 흐름 (목표)

```
부팅 → LoadingMap (클라 로컬, 아직 서버 없음 — 이 맵에선 클라가 authority라 GameMode 정상 동작)
     → 로딩 UI 표시 (뷰포트 레이어 persistent UI — 구현 완료, 트래블 넘어 생존)
     → 1단계: 에셋 프리로드 (캐시 워밍 — 구현 완료, 유지)
     → 2단계: config 서버 주소로 ClientTravel(접속)   ← [변경 핵심] OpenLevel 자기전환 제거
     → 서버가 자신의 맵(DevMap)을 클라에 지시 → 클라가 합류·로드
     → PostLoadMapWithWorld(비-로딩 월드) → 로딩 UI 종료 (구현 완료)
```

## 5. 설계 (A안 — 최소 리타겟)

선행 구현의 대부분을 유지하고 **전환 메커니즘·2단계 진행률·접속 실패 처리**만 바꾼다.

### 5-1. 전환 메커니즘 — `OpenLevel` → 접속

`TransitionToGameplayMap`을 아래 분기로 교체한다.

- **접속 모드** (`ServerAddress`가 비어 있지 않음): 로컬 PlayerController로 `ClientTravel(ServerAddress, TRAVEL_Absolute)`를 호출해 서버에 접속한다. 클라는 자기 월드를 독립적으로 열지 않는다 — 서버의 authoritative 월드에 합류한다. 이로써 §2의 연결 끊김·검은 화면·바 잔존이 구조적으로 사라진다.
- **오프라인 폴백** (`ServerAddress`가 비어 있음): 서버 없이 혼자 개발·테스트하는 경로. 기존 `OpenLevel(GameplayMap)`을 쓰되 **`NM_Standalone`일 때만** 실행한다(authority가 있는 단일 인스턴스에서만). 네트워크 클라이언트가 이 경로에 도달하면 아무것도 하지 않고 `Error` 로그 — **클라가 절대 `OpenLevel` 하지 않는다** (defense in depth).

`ClientTravel` 대상 PlayerController는 `GetGameInstance()->GetFirstLocalPlayerController()`로 얻는다. 널이면 `Error` 로그 후 실패 상태로 전이(§5-3).

### 5-2. 진행률 2단계 의미 변경

접속 경로에서는 `GetAsyncLoadPercentage(MapPackageName)`가 무의미하다 — 맵 로드가 로컬 `LoadPackageAsync`가 아니라 접속 상태 머신으로 구동되기 때문이다. 2단계를 아래로 재정의한다.

- **1단계 (에셋 프리로드)**: 개수 비율 실측(`Loaded/Total`) — **변경 없음.** 캐시 워밍 이득(측정: DevMap 폐포 ⊂ 프리로드 폐포) 유지.
- **2단계 (접속)**: 정밀 %가 없으므로 **접속 상태 마일스톤 + 시간 페이싱.** 접속 시작 시 2단계 진행률의 바닥값(예: 0)에서 시작해, `MinDisplaySeconds` 시간 클램프가 바를 부드럽게 채운다. `PostLoadMapWithWorld`(도착) 시 2단계 100%. 진행률을 시간으로 페이싱하는 것은 선행 스펙 §5의 정직성 원칙과 일치한다.
  - `GetMapPercent()`를 두 갈래로: **접속 모드**면 `bMapLoadComplete`(도착 여부) 기준 0/100, **오프라인 폴백**이면 기존 `GetAsyncLoadPercentage` 경로 유지.
  - 순수 함수 `CombineProgress`/`ClampDisplayed`/`ShouldTransition`은 **변경하지 않는다** — 기존 20개 테스트 그대로 통과. 바뀌는 것은 `GetMapPercent`의 소스뿐.

**전환 트리거 재정의**: 접속 모드에서 "전환"은 이제 "게임 진입 완료"가 아니라 "접속 개시"다. `ShouldTransition`(Raw≥1.0 && Elapsed≥MinDisplaySeconds)이 만족되면 `ClientTravel`을 **한 번** 시작하고, 이후 진행률은 도착까지 시간 페이싱으로 유지한다. 즉 접속 모드에서는 1단계 프리로드가 끝나고 최소 표시 시간을 채우면 접속을 시작하는 구조다.

**도착 판정 (조기 종료 방지)**: `HandlePostLoadMap(UWorld*)`는 현재 `bTransitionStarted` 기준으로 UI를 닫는데, 접속 중 트랜지션/펜딩 월드 로드에서 이 콜백이 먼저 와 **조기 종료**될 수 있다. 판정을 강화한다 — **로드된 월드의 맵 이름이 `LoadingMap`이 아닐 때만** 도착으로 보고 UI를 닫는다. `LoadingMap`(또는 그 PIE 복제본)에 대한 콜백은 무시한다. 이로써 접속 개시 전 로딩맵 자체 로드도, 접속 중 중간 월드도 UI를 닫지 못한다.

### 5-3. 접속 실패 처리 (신규)

- 서브시스템이 전환 시작 시 `GEngine->OnNetworkFailure()`와 `GEngine->OnTravelFailure()`에 구독한다.
- **접속 타임아웃**: config `ConnectTimeoutSeconds`(기본값 기획 단계에서 확정). `ClientTravel` 개시 후 이 시간 안에 `PostLoadMapWithWorld`(도착)가 오지 않으면 실패로 간주한다. 서브시스템 틱에서 경과 시간으로 판정.
- 실패(네트워크 오류 / 트래블 오류 / 타임아웃) 시:
  - 로딩 진행을 멈추고 **실패 상태**로 전이한다.
  - `OnConnectionFailed`(멀티캐스트 델리게이트, 실패 사유 문자열 전달)를 브로드캐스트한다.
  - 위젯이 이를 받아 **에러 메시지 + 재시도 버튼**을 노출한다(§5-4).
- **재시도**: 서브시스템 `RetryConnect()` — 실패 상태를 해제하고 접속을 다시 개시한다. 위젯 버튼이 호출한다.
- 델리게이트 구독은 성공 도착·`Deinitialize`·재시도 시 정확히 해제/재설정한다(수명 안전).

### 5-4. 위젯 확장 — 에러 상태

`USpyLoadingWidget`에 에러 UI를 추가한다(기존 `LoadingBar`·`PercentText`는 유지).

- `BindWidget`: 에러 메시지 `TextBlock`(예: `ErrorText`) + 재시도 `Button`(예: `RetryButton`). 초기 숨김(Collapsed).
- `NativeConstruct`에서 `OnConnectionFailed` 구독. 실패 수신 시 바·퍼센트를 흐리게/멈추고 `ErrorText`·`RetryButton`을 표시.
- `RetryButton` 클릭 → 서브시스템 `RetryConnect()` 호출 → 에러 UI 숨김, 로딩 UI 복귀.
- 순수 뷰 원칙 유지 — 위젯은 상태를 소유하지 않고 서브시스템 신호만 반영한다.

### 5-5. Config 확장

`USpyLoadingConfig`에 필드 추가(기존 `GameplayMap`·`MinDisplaySeconds`·`AssetPhaseWeight` 유지).

| 필드 | 타입 | 용도 |
|---|---|---|
| `ServerAddress` | `FString` | 자동 접속 대상. 예 `"127.0.0.1:7777"`. 비어 있으면 오프라인 폴백 |
| `ConnectTimeoutSeconds` | `float` | 접속 타임아웃(기본값 기획 확정) |

`GameplayMap`은 오프라인 폴백 전용으로 격하된다.

## 6. 재사용 (변경 없이 유지)

- persistent UI(뷰포트 레이어) — `OpenPersistentSpyUI`/`ClosePersistentSpyUI`.
- 순수 함수 `CombineProgress`/`ClampDisplayed`/`ShouldTransition` 및 20개 Automation 테스트.
- `MinDisplaySeconds` 시간 페이싱, 단조 증가 브로드캐스트.
- 1단계 에셋 프리로드(`LoadAssetsAsync` + `GetAllAssetPaths`).
- `LoadingMap`·`ASpyLoadingGameMode`(킥오프)·`HandlePostLoadMap`(도착 종료).
- `ShouldCreateSubsystem`의 데디서버 차단.

## 7. 예외 처리

| 상황 | 동작 |
|---|---|
| `ServerAddress` 있음 + 접속 성공 | 서버 합류 → `PostLoadMapWithWorld` → UI 종료 |
| `ServerAddress` 있음 + 접속 실패/타임아웃 | 실패 상태 → 에러 메시지 + 재시도 버튼 |
| `ServerAddress` 비어 있음 + `NM_Standalone` | 오프라인 폴백 `OpenLevel(GameplayMap)` (기존 동작) |
| `ServerAddress` 비어 있음 + 네트워크 클라 | `OpenLevel` 금지 — `Error` 로그 후 무시 (검은 화면 방지) |
| PlayerController 없음(접속 개시 시) | `Error` 로그 → 실패 상태 |
| Config 없음 / 미설정 | 기존과 동일 — `Error` 후 전환 중단 (`ApplyConfig`) |

## 8. 테스트

- **Automation(위젯·월드 불필요)**: 기존 20개 유지·통과. 접속 상태→진행률 매핑을 순수 함수로 추출하면 그 경계값 테스트 추가(추출 여부는 플랜에서 결정).
- **수동 — 올바른 데디서버 검증** (PIE "2인"은 데디서버의 실제 실행 방식이 아니므로 사용하지 않는다):
  1. 데디서버를 별도로 실행(DevMap 상주).
  2. 클라이언트 실행 → LoadingMap → 자동 접속 → 로딩바 진행 → DevMap 합류 → UI 종료 → 게임플레이. **검은 화면·바 잔존 없음.**
  3. 접속 실패: 서버 없이 클라 실행 → 에러 메시지 + 재시도 버튼 → 서버 켬 → 재시도 → 정상 접속.
  4. 오프라인 폴백: `ServerAddress` 비운 채 단일 클라 실행 → `OpenLevel(GameplayMap)` 정상.
- **회귀**: 기존 로딩 씬 순수 함수·persistent UI 동작 불변 확인.

## 9. 범위 밖 (YAGNI)

- 메인 메뉴, 서버 브라우저, 매치메이킹, 서버 주소 입력 UI.
- Seamless Travel(데디서버 클라 합류로 충분 — 불필요).
- MoviePlayer(T3 트래블 구간 0.147초로 짧음 — 불필요).
- 로딩 화면 페이드·연출(선행 스펙 §8-2).
- 서버 측 로딩/트래블 연출(서버는 헤드리스).
