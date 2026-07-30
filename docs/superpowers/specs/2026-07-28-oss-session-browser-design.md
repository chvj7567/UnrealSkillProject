# OSS 세션 브라우저 (방 목록) — 설계 스펙

- 작성일: 2026-07-28
- 상태: 사용자 승인 완료 (brainstorming 산출물)
- 선행: `docs/superpowers/specs/2026-07-23-networked-loading-transition-design.md` (로딩 → 접속 전환), 그 구현
- 다음 단계: `writing-plans` → 구현 플랜

## 1. 목적

로딩이 끝난 뒤 **접속 가능한 방(세션) 목록**을 띄우고, 사용자가 방을 만들거나 고른 방에 들어가 **인게임 맵에서 함께 플레이**하도록 한다. Unreal OnlineSubsystem(OSS)을 도입하되, 세션 로직을 **프로젝트 비의존 재사용 플러그인 `SKOnline`** 으로 분리해 게임 모듈과의 결합을 최소화한다. 백엔드는 현재 **OSS Null(LAN)** 이고, 코드 재작업 없이 **Steam 으로 전환**할 수 있는 이음매를 함께 설계한다.

## 2. 현재 상태 (변경 대상의 출발점)

로딩 → 접속 파이프라인은 **이미 구현돼 있다.** 이번 작업은 새 파이프라인을 만드는 게 아니라 **전환 지점 하나를 갈아끼우는 것**이다.

| 기존 구현 | 위치 |
|---|---|
| 로딩맵 부팅 → 에셋 프리로드 → "접속" 버튼 대기 | `SpyLoadingSubsystem.cpp:121` `StartLoading` / `Tick` |
| "접속" 클릭 → 맵 패키지 로드 → 전환 | `SpyLoadingSubsystem.cpp:305` `EnterGameplay` |
| 고정 주소 `ClientTravel` / 오프라인 폴백 | `SpyLoadingSubsystem.cpp:337` `TransitionToGameplayMap` |
| 접속 감시·타임아웃·네트워크/트래블 실패·재시도 | `StartConnectWatch` / `TickConnectWatch` / `RetryConnect` |
| 트래블 넘어 생존하는 로딩 UI, 도착 시 종료 | `OpenPersistentSpyUI(Loading)` / `HandlePostLoadMap` |

**핵심 관찰**: 조인은 결국 "주소가 다른 `ClientTravel`" 이다. 따라서 위 기계 전부를 그대로 재사용할 수 있고, 바꿔야 하는 것은 **주소가 어디서 오는가** 뿐이다.

## 3. 확정 결정 (사용자)

| # | 결정 | 내용 |
|---|---|---|
| D1 | **호스트 모델 = 리슨 서버** | 플레이어가 "방 만들기"를 누르면 그 프로세스가 서버가 된다. 데디케이티드 서버 경로는 제거하지 않고 남긴다 |
| D2 | **모듈 배치 = 신규 플러그인 `SKOnline`** | 세션 로직만. UI 없음. 게임 모듈 역참조 없음 |
| D3 | **Steam = 구조만** | 이번 사이클은 Null/LAN 으로 검증. Steam 전환은 ini + `.uproject` 만으로 되게 설계 |
| D4 | **직접 IP 입력 = 제외** | 방 목록만 |
| D5 | **방 만들기 = 즉시 생성** | 옵션 입력 화면 없음. 이름·최대 인원은 설정 기본값 |
| D6 | **기존 고정 주소 경로 = 유지** | `ServerAddress` 가 채워져 있으면 기존 자동 접속, 비어 있으면 방 목록 |

### 3-1. D1 의 배경 — 왜 리슨인가

`SkillProject/Source/` 에 `SkillProjectServer.Target.cs` 가 **없다**(확인 완료 — `SkillProject.Target.cs`, `SkillProjectEditor.Target.cs` 둘뿐). 즉 독립 서버 exe 를 빌드할 수 없어 데디 모델은 오늘 두 PC 간 검증이 불가능하다. 리슨 서버는 현재 게임 exe 하나로 즉시 성립하며, Steam 로비도 리슨 호스팅이 표준 경로다.

**리슨 선택이 서버 권한 아키텍처를 바꾸지 않는다.** `HasAuthority()`·`Replicated`·GAS 서버 처리는 두 모델에서 동일하다. 데디 전용 코드는 로딩 흐름의 분기 3곳(`ShouldCreateSubsystem` / `NM_DedicatedServer` ServerTravel / `ServerDefaultMap`)뿐이고 전부 그대로 남는다.

## 4. 아키텍처

### 4-1. `SKOnline` 플러그인 (신규)

```
SkillProject/Plugins/SKOnline/
├── SKOnline.uplugin                    EnabledByDefault: true
└── Source/SKOnline/
    ├── SKOnline.Build.cs               Core, CoreUObject, Engine,
    │                                    OnlineSubsystem, OnlineSubsystemUtils
    ├── Public/
    │   ├── SKOnlineTypes.h             FSKSessionInfo, ESKSessionOp, ESKSessionError
    │   ├── SKOnlineSettings.h          UDeveloperSettings — 세션 플래그
    │   └── SKOnlineSessionSubsystem.h  UGameInstanceSubsystem
    └── Private/ (대응 .cpp)
```

**의존 방향**: `SkillProject → SKOnline → OnlineSubsystem/OnlineSubsystemUtils`. SKOnline 은 `SKGAS`·`SKAssetCore`·`SKUICore`·게임 모듈 **어느 것에도 의존하지 않는다** (unreal-infra §1 준수). 게임 모듈 헤더를 include 하지 않는다.

**책임 경계** — SKOnline 이 하는 것:
- 세션 생성 / 검색 / 조인 / 파괴
- 검색 결과를 표시용 struct 로 변환
- 진행 중 op 중복 가드
- 결과를 델리게이트로 통지 (**조인 결과는 "접속 문자열"까지**)

**하지 않는 것** (전부 게임 모듈 몫):
- 트래블 (`ServerTravel` / `ClientTravel`)
- 위젯·UI
- 로딩 진행률·persistent UI

이 경계 덕분에 SKOnline 은 다른 프로젝트에 그대로 옮겨도 동작한다.

### 4-2. `USKOnlineSessionSubsystem` API

```cpp
//# 명령
void HostSession();                      //# D5 — 설정 기본값으로 즉시 생성
void FindSessions();
void JoinSessionByIndex(int32 Index);
void DestroyCurrentSession();

//# 통지 (멀티캐스트 델리게이트)
FOnSKSessionsFound      OnSessionsFound;   //# (const TArray<FSKSessionInfo>&)
FOnSKHostReady          OnHostReady;       //# () — 생성 성공, 게임 모듈이 ServerTravel
FOnSKJoinReady          OnJoinReady;       //# (const FString& ConnectString)
FOnSKSessionError       OnSessionError;    //# (ESKSessionOp, ESKSessionError, const FString& Detail)
```

- `OnHostReady` 는 **주소를 넘기지 않는다** — 호스트는 자기 월드를 `?listen` 으로 여는 것이라 접속 문자열이 필요 없다.
- `OnJoinReady` 는 `GetResolvedConnectString` 결과를 넘긴다. 게임 모듈은 이 문자열을 기존 `ClientTravel` 경로에 그대로 흘린다.
- op 진행 중에는 같은 종류의 새 요청을 무시한다(더블클릭 가드). 상태는 `ESKSessionOp` 하나로 관리.

### 4-3. `FSKSessionInfo` (표시용)

| 필드 | 출처 |
|---|---|
| `RoomName` | 세션 커스텀 세팅 키(문자열) |
| `HostName` | `FOnlineSessionSearchResult::Session.OwningUserName` |
| `CurrentPlayers` / `MaxPlayers` | `NumPublicConnections` − `NumOpenPublicConnections` / `NumPublicConnections` |
| `PingMs` | `Session.PingInMs` |
| `SearchResultIndex` | 조인 시 원본 결과를 되찾는 인덱스 |

UI 는 `FOnlineSessionSearchResult` 를 **직접 만지지 않는다** — 게임 모듈이 OSS 타입에 물리지 않게 하는 차단막이다.

### 4-4. `USKOnlineSettings : UDeveloperSettings` — 백엔드 이음매

**규칙 이탈을 명시한다.** 프로젝트 규칙(plugin-skassetcore)은 설정값을 Config DataAsset 에 두라고 한다. 여기서는 `UDeveloperSettings`(`config=Game`, 프로젝트 설정 노출)를 쓴다. 근거:

1. SKOnline 이 DataAsset 을 읽으려면 `SKAssetCore` 에 의존해야 한다 — D2(의존성 최소화)와 정면 충돌.
2. **Null↔Steam 전환은 본질적으로 ini 작업이다.** `DefaultPlatformService`·NetDriver 는 ini 로만 바꿀 수 있다. 세션 플래그만 DataAsset 에 두면 **하나의 스위치가 두 파일로 쪼개져** 전환 시 한쪽을 빠뜨리기 쉽다.

| 필드 | Null 기본값 | Steam 값 | 적용 지점 |
|---|---|---|---|
| `bIsLanMatch` | `true` | `false` | 생성(`bIsLANMatch`) + **검색**(`bIsLanQuery`) |
| `bUsesPresence` | `false` | `true` | **생성 전용** (`bUsesPresence` · `bAllowJoinViaPresence`) |
| `bUseLobbiesIfAvailable` | `false` | `true` | 생성 + **검색**(`SEARCH_LOBBIES` 질의 게이트) |
| `bShouldAdvertise` | `true` | `true` | 생성 전용 |
| `MaxPlayers` | (게임 기획 확정) | 동일 |
| `MaxSearchResults` | (게임 기획 확정) | 동일 |
| `SearchTimeoutSeconds` | (게임 기획 확정) | 동일 |
| `DefaultRoomNameFormat` | (게임 기획 확정) | 동일 |

수치·문구 기본값은 `game-designer` 가 확정한다(§8).

### 4-5. 게임 모듈 변경 — 최소 침습

| 대상 | 변경 | 성격 |
|---|---|---|
| `USpyLoadingSubsystem::EnterGameplay()` | `EnterGameplay(const FString& OverrideAddress = TEXT(""))` — 빈 값이면 **현행 동작 그대로** | 시그니처 1개 |
| `USpyLoadingSubsystem` | `HostAndEnter()` 신규 — phase2 맵 로드 후 `ServerTravel(Map + "?listen")` | 메서드 1개 |
| `USpyLoadingWidget::HandleReadyToEnter` | `ServerAddress` 비었으면 접속 버튼 대신 방 목록 UI 오픈 | 분기 1개 |
| `USpySessionBrowserWidget` (신규) | `USKUserWidget` 상속. 목록·새로고침·방 만들기·에러 표시 | 신규 |
| `USpySessionRowWidget` (신규) | 방 1행. 클릭 시 인덱스 통지 | 신규 |
| `ESpyUIType` | `SessionBrowser` 추가 | enum 1개 |
| `USpyAssetData` | `SessionBrowser` UI 엔트리 등록 | 데이터 |
| `DA_SpyLoadingConfig.ServerAddress` | `127.0.0.1:7777` → **빈 값** | 데이터 |
| `SkillProject.Build.cs` | `SKOnline` 의존 추가 | 1줄 |

`TransitionToGameplayMap` 의 접속 감시·타임아웃·실패·재시도, persistent 로딩 UI, `HandlePostLoadMap` 도착 처리는 **변경하지 않는다.**

행 위젯은 UI 매니저로 여는 화면이 아니라 목록 안에 반복 생성되는 조각이므로, **브라우저 위젯의 `TSubclassOf<USpySessionRowWidget>` 프로퍼티**로 참조한다(스킬바 슬롯 위젯과 같은 패턴). `ESpyUIType` 에도 `USpyAssetData` 엔트리에도 추가하지 않는다.

> **`ServerAddress` 를 비우는 것이 방 목록을 켜는 스위치다.** 채워 두면 D6 대로 기존 자동 접속이 그대로 돈다(데디서버 붙일 때·CI 용). 이 이중 경로는 의도된 것이며 §7-2 에서 검증한다.

> **오프라인 폴백(`NM_Standalone` → `OpenLevel`)은 기본 설정에서 도달 불가가 된다.** 주소가 비면 방 목록이 뜨고 "접속" 버튼이 숨겨지므로 사용자가 `EnterGameplay("")` 를 유발할 경로가 없다. **기능이 사라지는 것은 아니다** — "방 만들기"가 혼자 도는 리슨 서버를 열어 주고, 이는 폴백이 제공하던 "서버 없이 혼자 게임플레이 반복"과 같은 목적을 더 나은 형태로 대체한다(`networked-loading-transition.md` §6-2 의 존치 근거가 소멸). 코드는 제거하지 않고 남기되(`ServerAddress` 를 채운 Standalone 실행에서 여전히 유효), **호스팅이 폴백을 대체했다**는 사실을 여기 기록한다.

## 5. 흐름

```
LoadingMap (ASpyLoadingGameMode::BeginPlay)
  └ USpyLoadingSubsystem::StartLoading()          [기존, 무변경]
      └ 에셋 프리로드 → OnReadyToEnter            [기존, 무변경]
          ├ ServerAddress 채워짐 → "접속" 버튼 → EnterGameplay("")   [기존 경로]
          └ 비어 있음 → OpenSpyUI(SessionBrowser)
                ├ [새로고침] FindSessions()
                │     ├ OnSessionsFound(Infos) → 행 갱신 (0건이면 "방 없음" 안내)
                │     └ OnSessionError → 메시지, 목록 유지
                ├ [방 만들기] HostSession()
                │     └ OnHostReady → HostAndEnter()
                │           └ phase2 맵 로드 → ServerTravel("<GameplayMap>?listen")
                └ [방 행 클릭] JoinSessionByIndex(i)
                      └ OnJoinReady(ConnectString) → EnterGameplay(ConnectString)
                            └ 기존 ClientTravel + 접속 감시/타임아웃/재시도
```

양쪽 모두 도착 시 기존 `HandlePostLoadMap` 이 persistent 로딩 UI 를 내린다.

> **⚠ 호스팅 순서 정정 (2026-07-28 PIE 검증 후, 사용자 결정)** — 위 그림의 `HostSession() → OnHostReady → HostAndEnter()` 순서는 **틀렸다.** 실제 순서는 **`HostAndEnter()`(트래블) → 도착 → `HostSession()`** 이다.
>
> 이유: OSS Null 은 `FOnlineSessionInfoNull::Init`(`OnlineSessionInterfaceNull.cpp:47`)에서 **`CreateSession` 시점의 NetDriver 로부터 포트를 가져온다.** 트래블 전에 세션을 만들면 NetDriver 가 없어 **포트 0** 이 박히고, 그 사이 검색한 클라이언트는 `RemoteAddr: <ip>:0` 으로 조인해 **20초 타임아웃**난다(실측 확인). 세션을 도착 후에 만들면 방이 목록에 뜨는 순간 이미 접속 가능한 상태라 이 틈이 사라진다.
>
> 귀결: 도착 후 `CreateSession` 이 실패하면 사용자는 이미 인게임이고 브라우저가 없다 — **되돌리지 않고 로그만 남긴다**(방이 목록에 안 뜰 뿐 혼자 플레이는 된다).

### 5-1. 호스트가 리슨 서버가 되는 지점

호스트는 로딩맵에서 `NM_Standalone`(권한 보유)이므로 `UWorld::ServerTravel("<맵>?listen")` 로 자기 월드를 리슨 서버로 연다. 이후 조인하는 클라이언트는 그 서버에 합류한다. 기존 데디 분기(`StartLoading` 의 `NM_DedicatedServer` → `ServerTravel`)와 같은 API 를 쓰되 `?listen` 옵션만 다르다.

### 5-2. UI 계층

방 목록은 **로딩맵에서만 사는 일반 UI**(`OpenSpyUI`)다 — persistent 가 아니다. 접속을 시작하면 브라우저를 닫고, 그 아래 깔려 있던 persistent 로딩 UI 가 접속 진행률을 이어 그린다(트래블을 넘어 생존). 두 UI 의 역할이 겹치지 않는다.

세부 규약 (기존 `HandleReadyToEnter` / `OnEnterClicked` 의 동작을 그대로 물려받는다):

- 브라우저가 뜬 동안 로딩 UI 의 **바·퍼센트는 숨긴다.** 호스팅/조인을 시작하는 순간 복원해 접속 진행률을 그린다 — 기존 "접속" 버튼 경로와 동일한 연출 규약.
- 브라우저를 열 때 **마우스 커서 표시 + `GameAndUI` 입력 모드**로 전환한다. 패키지 standalone 은 기본이 `GameOnly`·커서 숨김이라 이 처리가 없으면 클릭이 UI 로 가지 않는다(기존 `HandleReadyToEnter` 가 같은 이유로 하고 있다).
- 호스팅/조인 개시 시 **브라우저를 명시적으로 닫고** `GameOnly` 로 되돌린다. 트래블로 월드와 함께 소멸하는 데 의존하지 않는다(전환 전 잔상·클릭 방지).

**렌더 순서 (기획 단계에서 발견된 블로커 — 사용자 결정 반영)**

로딩 persistent UI 는 `OpenPersistentUI` 로 **ZOrder 100** 에 얹히는데, 일반 UI 는 `AddToViewport()` 를 인자 없이 호출해 **ZOrder 0** 이다(`SKUIManager.cpp:110`·`122`·`262`). 로딩 위젯이 전체 화면 불투명 배경을 가지므로 **방 목록이 그 뒤에 깔려 보이지 않는다.**

- **결정: `USKUIManager::OpenUI` 에 `int32 ZOrder = 0` 파라미터를 추가한다.** 호출부에서 우회하지 않고 원인(두 API 의 ZOrder 비대칭)을 없애는 쪽이다. 기본값 0 이라 기존 호출부는 무영향이고, `OpenPersistentUI` 와 API 가 대칭이 된다.
- 방 목록은 **ZOrder 200** 으로 연다 — 로딩 UI(100)보다 크되, 그 사이(101~199)를 다른 UI 가 쓸 여지를 남긴 간격이다.
- 기각한 대안: 브라우저를 persistent UI 로 여는 방식(전환 시 닫기 규약이 복잡해짐), 브라우저 표시 중 로딩 UI 를 내리는 방식(접속 진행률 표시가 끊기고 persistent UI 의 존재 이유를 훼손).

## 6. 에러 처리

| 상황 | 처리 |
|---|---|
| 검색 결과 0건 | 에러 아님. "방이 없습니다" 안내 + 새로고침 유지 |
| Create / Find / Join 실패 | `OnSessionError` → 브라우저에 메시지, 목록·버튼 유지(재시도 가능) |
| 조인 성공 후 접속 실패 | **기존 실패 경로 재사용** — 로딩 UI 의 에러 문구 + 재시도 버튼 |
| 조인 실패 후 세션 잔재 | `DestroyCurrentSession` 으로 정리 후 브라우저 복귀 |
| op 진행 중 중복 입력 | 서브시스템이 무시(가드) |
| `IOnlineSubsystem::Get()` 이 null | 브라우저에 "온라인 기능을 사용할 수 없습니다" 표시, 크래시 금지 |
| 데디케이티드 서버 프로세스 | 브라우저 UI 를 만들지 않는다(뷰포트 없음) |

사용자에게는 **고정 한국어 문구**를 보이고, OSS 원문 사유는 로그로만 남긴다 — 기존 접속 실패 처리(`networked-loading-transition.md` §4-1)와 같은 규약.

## 7. 검증

### 7-1. 자동화 테스트 (`test-engineer`)

OSS 인터페이스 자체는 엔진·플랫폼 의존이라 자동화가 어렵다. **순수 로직을 떼어내 테스트**한다.

| 대상 | 케이스 |
|---|---|
| 검색 결과 → `FSKSessionInfo` 변환 | 인원 계산(`Max − Open`), 인덱스 보존, 빈 결과 |
| op 중복 가드 상태머신 | 진행 중 재요청 무시, 완료 후 재요청 허용, 실패 후 복구 |
| `EnterGameplay` 주소 우선순위 | override > config `ServerAddress` > 오프라인 폴백 |
| 설정 기본값 | Null 프로필이 `bIsLanMatch=true` 로 나오는지 |

### 7-2. 수동 검증 (PIE / 패키지)

1. 클라이언트 2개 실행 → A 에서 "방 만들기" → A 가 인게임 진입
2. B 에서 "새로고침" → A 의 방이 목록에 뜸 → 클릭 → B 가 A 의 월드에 합류
3. 둘이 서로 보이고 움직임이 동기화되는지
4. `ServerAddress` 를 다시 채우면 기존 자동 접속이 그대로 도는지 (D6 회귀)
5. 서버 없는 상태에서 목록 비었을 때 안내가 뜨는지

### 7-3. 이 단계에서 검증되지 않는 것

- **인터넷 접속** — Null 은 LAN 브로드캐스트 전용이라 원리상 불가. Steam 전환 후에만 확인 가능.
- **Steam 경로 전체** — §9 로 이월.

## 8. `game-designer` 가 확정할 값

구조는 이 스펙이 확정한다. 아래 **값·문구**는 기획 단계에서 정한다.

- `MaxPlayers` 기본값 (DevMap PlayerStart 수와 정합)
- `MaxSearchResults`, `SearchTimeoutSeconds`
- 방 이름 기본 포맷 (D5 — 옵션 입력이 없으므로 자동 생성 규칙 필요)
- 방 목록 화면 레이아웃·색·서체 (`loading-scene.md` §5 톤과 정합), 행에 표시할 항목 순서
- 문구: "방 만들기" / "새로고침" / 방 없음 안내 / 각 에러 메시지

## 9. 명시 제외

- 메인 메뉴, 방 옵션 입력 화면, 비밀번호 방
- 직접 IP 입력 (D4)
- Steam 실제 활성화·검증 (D3 — 이음매만)
- 데디케이티드 서버 빌드 타깃 (`SkillProjectServer.Target.cs`)
- 인원 꽉 참·강퇴·방장 위임·게임 중 재입장
- Seamless Travel, 로비 맵, 매치메이킹
- 게임 종료 후 방 목록 복귀

## 10. Steam 이행 경로 (설계만, 구현 안 함)

전환 시 **C++ 재작업 0** 을 목표로 한다.

| 파일 | 변경 |
|---|---|
| `SkillProject.uproject` | `OnlineSubsystemSteam` 플러그인 활성화 |
| `DefaultEngine.ini` | `[OnlineSubsystem] DefaultPlatformService=Steam` |
| `DefaultEngine.ini` | `[OnlineSubsystemSteam] bEnabled=true`, `SteamDevAppId=480` |
| `DefaultEngine.ini` | `NetDriverDefinitions` → `SteamNetDriver` |
| `DefaultGame.ini` | `SKOnlineSettings`: `bIsLanMatch=false`, `bUsesPresence=true`, `bUseLobbiesIfAvailable=true` |
| 실행 디렉터리 | `steam_appid.txt` (내용 `480`) |

검증에는 각자 Steam 계정을 가진 테스터 2명이 필요하다. AppID 480 은 전 세계 개발자 공용이라 **검색 결과에 남의 테스트 방이 섞인다** — 방 이름/커스텀 키로 필터링해야 한다. 이 필터 키를 §4-3 의 `RoomName` 커스텀 세팅으로 미리 심어 둔다.

> **검색 측 게이팅이 필요하다 (구현 중 엔진 소스로 확인된 사실).**
> 세션 **생성** 측 플래그(`bUsesPresence`·`bUseLobbiesIfAvailable`)만으로는 Steam 에서 목록이 채워지지 않는다. Steam 의 `FindSessions` 는 `QuerySettings` 의 **`SEARCH_LOBBIES`** 키로 로비 검색과 서버 브라우저 검색을 가르기 때문이다(`OnlineSessionInterfaceSteam.cpp:778`). 이 키가 없으면 `CreateSession` 은 로비를 만드는데 `FindSessions` 는 서버 브라우저를 뒤져 **항상 빈 목록**이 된다.
> 따라서 `USKOnlineSessionSubsystem::FindSessions` 는 `bUseLobbiesIfAvailable` 이 켜졌을 때 `SEARCH_LOBBIES` 를 질의에 싣는다. **이 코드는 Null 단계에서 이미 들어가 있으므로**(Null 은 `bIsLanQuery` 로 라우팅돼 이 키와 무관), Steam 전환 시 §10 표의 ini 변경만으로 검색이 정상 동작한다 — **C++ 재작업 0 이라는 목표가 유지된다.**
>
> 참고: UE 5.7 에는 `SEARCH_PRESENCE` 매크로가 **없다**(`OnlineSessionNames.h` 에 부재). 초기 설계에서 이 키를 가정했으나 존재하지 않아 `SEARCH_LOBBIES` 로 대체했다.
