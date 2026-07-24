# 로딩 씬 — 설계 스펙

- 작성일: 2026-07-22
- 상태: 사용자 승인 완료 (brainstorming 산출물)
- 다음 단계: `writing-plans` → 구현 플랜

## 1. 목적

게임 부팅 시 전용 로딩 레벨로 진입해 **로딩바 + 퍼센트 표기**를 보여주고, 로딩이 끝나면 게임플레이 맵으로 전환한다.

## 2. 사전 조사로 확정된 제약

구현 전 코드 조사에서 확인한, 설계를 규정하는 사실들이다.

1. **`USKAssetManager::OnLoadProgress` 는 진행률 소스로 쓸 수 없다.**
   `SKAssetManager.cpp:9-14` 의 `StartInitialLoading()` → `LoadAllPrimaryAssetsSync()` 에서만 호출된다. 엔진 초기화 시점이라 World·UMG 가 없고, `WaitUntilComplete` 로 게임 스레드를 블로킹한다. 로딩 레벨의 위젯이 이 값을 받아 그릴 방법이 없다.
   → 로딩 레벨이 **자체 비동기 로드**를 돌린다.

2. **`LoadAllPrimaryAssetsSync` / `LoadPrimaryAssetsAsync` 는 `protected`** (`SKAssetManager.h:42-43`). 게임 모듈에서 호출 불가.

3. **`USKAssetData::AssetNameToPath` 는 `private` 이고 열거 API 가 없다** (`SKAssetData.h:48-53`). "등록된 에셋 전부 프리로드" 수단이 없다.

4. **게임플레이 맵은 `/Game/Spy/Maps/DevMap` 하나뿐**이고, `DefaultEngine.ini` 의 `GameDefaultMap`·`EditorStartupMap` 도 DevMap 이다.

5. **`USpyUIManager::OpenSpyUI` 는 `ESpyUIType` 의 enum 항목명을 그대로 위젯 등록명으로 쓴다** (`SpyUIManager.cpp:26`).

## 3. 확정된 설계 결정

| 항목 | 결정 |
|---|---|
| 형태 | 전용 로딩 Level 신규 생성 |
| 진행률 | 2단계 — 1단계 에셋 프리로드 + 2단계 맵 비동기 스트리밍 |
| 진입 시점 | 게임 시작 맵 (`GameDefaultMap`·`EditorStartupMap` 모두 LoadingMap) |
| 완료 동작 | `MinDisplaySeconds` 보장 후 자동 전환 |
| 로직 위치 | `USpyLoadingSubsystem` (GameInstanceSubsystem) |
| 에셋 작업 | unreal-mcp 로 위젯·맵·DataAsset 생성 |
| 플러그인 | SKAssetCore 최소 확장 (부팅 동기 로드는 유지) |

## 4. 아키텍처

### 4-1. SKAssetCore 플러그인 확장

프로젝트 비의존 범용 API 만 추가한다. 부팅 시 `LoadAllPrimaryAssetsSync` 동작은 **변경하지 않는다.**

| 대상 | 변경 |
|---|---|
| `USKAssetData` | `void GetAllAssetPaths(TArray<FSoftObjectPath>& OutPaths) const` — `AssetNameToPath` 값 전체를 public 노출 |
| `USKAssetManager` | `DECLARE_DELEGATE_TwoParams(FSKAssetBatchProgressDelegate, int32 /*Loaded*/, int32 /*Total*/)` |
| `USKAssetManager` | public `void LoadAssetsAsync(const TArray<FSoftObjectPath>& AssetPaths, const FSKAssetBatchProgressDelegate& OnProgress, const FSimpleDelegate& OnComplete)` |

`LoadAssetsAsync` 요구사항:
- 이미 메모리에 있는 경로(`ResolveObject()` 성공)는 즉시 완료로 카운트한다.
- 배열이 비어 있으면 `OnProgress(0, 0)` 없이 즉시 `OnComplete` 를 호출한다.
- 개별 로드 실패해도 완료 카운트는 증가시킨다 (진행률이 멈추면 안 된다).
- 로드된 에셋은 `AddLoadedAsset` 으로 GC 참조를 유지한다.

1단계 프리로드 대상은 `AssetNameToPath` 에 등록된 **secondary 에셋**(메시·몽타주·위젯 BP 등)이다. Primary 에셋은 부팅 시 이미 동기 로드가 끝나 있다.

### 4-2. 게임 모듈 신규 클래스

| 클래스 | 위치 | 역할 |
|---|---|---|
| `USpyLoadingConfig : UPrimaryDataAsset` | `Source/SkillProject/Data/` | 로딩 설정. 맵 이름 하드코딩 제거 |
| `USpyLoadingSubsystem : UGameInstanceSubsystem, FTickableGameObject` | `Source/SkillProject/Manager/` | 로직 소유주 — 파이프라인·진행률 합성·전환 |
| `ASpyLoadingGameMode : AModularGameMode` | `Source/SkillProject/System/` | 로딩 맵 GameMode Override. 킥오프 전용 |
| `USpyLoadingWidget : USKUserWidget` | `Source/SkillProject/UI/` | 순수 뷰 |
| `ESpyUIType::Loading` | `Source/SkillProject/Util/DefineEnum.h` | enum 항목 추가 |

**`USpyLoadingConfig` 필드**

| 필드 | 타입 | 기본값 | 설명 |
|---|---|---|---|
| `GameplayMap` | `TSoftObjectPtr<UWorld>` | (에디터 지정) | 전환 대상 맵 |
| `MinDisplaySeconds` | `float` | `2.f` | 로딩 화면 최소 표시 시간 |
| `AssetPhaseWeight` | `float` | `0.5f` | 1단계 가중치. 2단계 가중치는 `1 - AssetPhaseWeight` |

**`USpyLoadingSubsystem` 공개 API**

- `void StartLoading()` — 파이프라인 시작
- `FOnLoadingProgressChanged OnProgressChanged` — `DECLARE_MULTICAST_DELEGATE_OneParam(..., float /*Displayed*/)`
- `float GetDisplayedProgress() const`
- `static float CombineProgress(int32 Loaded, int32 Total, float MapPercent, float Weight)` — 순수 함수, 테스트 대상

**`ASpyLoadingGameMode`** — `BeginPlay` 에서 두 가지만 한다: 서브시스템 `StartLoading()` 호출, `USpyUIManager::OpenSpyUI(ESpyUIType::Loading)`.

**`USpyLoadingWidget`** — `UPROPERTY(meta=(BindWidget)) TObjectPtr<UProgressBar> LoadingBar`, `TObjectPtr<UTextBlock> PercentText`. `NativeConstruct` 에서 `OnProgressChanged` 구독, `NativeDestruct` 에서 해제.

### 4-3. 의존 방향

```
USpyLoadingWidget  ──구독──▶  USpyLoadingSubsystem  ──▶  USKAssetManager (1단계)
                                                    ──▶  LoadPackageAsync (2단계)
ASpyLoadingGameMode ──킥오프──▶ USpyLoadingSubsystem
```

서브시스템은 위젯을 알지 못한다. 따라서 위젯 없이 진행률 파이프라인을 Automation 으로 검증할 수 있다.

### 4-4. 데디케이티드 서버

- `DefaultEngine.ini` 에 `ServerDefaultMap=/Game/Spy/Maps/DevMap` 을 명시해 서버 빌드는 로딩 맵을 거치지 않는다.
- `USpyLoadingSubsystem::ShouldCreateSubsystem` 에서 `IsRunningDedicatedServer()` 이면 `false` 를 반환해 생성 자체를 막는다.

## 5. 데이터 흐름 — 진행률 합성

```
Phase1Ratio = Total > 0 ? Loaded / Total : 1.0         // 에셋 개수
Phase2Ratio = max(GetAsyncLoadPercentage(MapPackage), 0) / 100  // 미시작 시 -1 → 0
W           = clamp(AssetPhaseWeight, 0, 1)
Raw         = clamp(Phase1Ratio * W + Phase2Ratio * (1 - W), 0, 1)
Displayed   = min(Raw, Elapsed / MinDisplaySeconds)
```

`GetAsyncLoadPercentage` 는 해당 패키지가 로드 중이 아닐 때 `-1` 을 반환한다. 반드시 0 으로 바닥을 잡는다. `MinDisplaySeconds <= 0` 이면 시간 클램프를 건너뛴다(`Displayed = Raw`).

- `Displayed` 를 시간으로 클램프하므로 로드가 순식간에 끝나도 바가 `MinDisplaySeconds` 에 걸쳐 자연스럽게 찬다.
- `Displayed` 는 단조 증가하며 `1.0` 을 넘지 않는다.
- 퍼센트 표기는 `FMath::RoundToInt(Displayed * 100)`.
- **전환 조건**: `Raw >= 1.0 && Elapsed >= MinDisplaySeconds`. 충족 시 `UGameplayStatics::OpenLevel`.
- 1단계가 끝나기 전에는 `GetAsyncLoadPercentage` 가 유효하지 않으므로 `Phase2Ratio = 0` 으로 둔다. 즉 2단계 `LoadPackageAsync` 는 1단계 `OnComplete` 시점에 시작한다.

### 알려진 한계 — PIE 에서 2단계는 실측이 아니다

PIE 는 맵 패키지를 `/Temp/UEDPIE_0_...` 로 복제해 열기 때문에, 미리 로드해 둔 `/Game/Spy/Maps/DevMap` 이 그대로 재사용되지 않는다. 2단계 진행률과 프리로드 이득은 **Standalone/패키징 빌드에서만 유효**하며, PIE 에서는 사실상 `MinDisplaySeconds` 에 맞춰 흐르는 바를 보게 된다. `EditorStartupMap` 을 LoadingMap 으로 바꾸기로 했으므로 이 차이를 전제로 개발한다.

## 6. 예외 처리

| 상황 | 동작 |
|---|---|
| Config 없음 / `GameplayMap` 미설정 | `UE_LOG(Error)` 후 전환 중단. 임의 맵 이름 fallback 금지 (하드코딩 룰) |
| 개별 에셋 로드 실패 | 완료 카운트는 증가. 실패 경로는 `Warning` 로그 |
| 1단계 로드 대상 0개 | 1단계 즉시 100%, 곧바로 2단계 진행 |
| 맵 `LoadPackageAsync` 실패 | `Error` 로그 후에도 `OpenLevel` 은 시도 (엔진 동기 로드로 커버) |
| 서브시스템 미생성 (데디 서버) | GameMode 킥오프가 null 체크 후 조용히 무시 |

## 7. 테스트 (Unreal Automation, 위젯 불필요)

- `CombineProgress` 경계값: `Total == 0`, `Loaded == 0`, `W == 0`, `W == 1`, `MapPercent < 0`(미시작)
- `Displayed` 가 단조 증가하며 `1.0` 을 초과하지 않음
- `Elapsed < MinDisplaySeconds` 동안 전환 조건이 `false`
- `Raw < 1.0` 이면 `Elapsed` 가 아무리 커도 전환 조건이 `false`
- Config 누락 시 크래시 없이 전환 중단

## 8. MCP 로 생성할 에셋 & 설정

1. `/Game/Spy/Maps/LoadingMap` — 빈 레벨. World Settings 의 GameMode Override = `ASpyLoadingGameMode`
2. `/Game/Spy/UI/WBP_Loading` — `USpyLoadingWidget` 파생. `LoadingBar`(ProgressBar) + `PercentText`(TextBlock)
3. `/Game/Spy/Data/DA_SpyLoadingConfig` — `GameplayMap = /Game/Spy/Maps/DevMap`
4. `SpyAssetData` 등록: `Loading → /Game/Spy/UI/WBP_Loading`, `SpyLoadingConfig → DA_SpyLoadingConfig`
5. `SpyAssetNames.h` 에 `LoadingConfig` 이름 상수 추가
6. `DefaultEngine.ini`:
   - `GameDefaultMap=/Game/Spy/Maps/LoadingMap`
   - `EditorStartupMap=/Game/Spy/Maps/LoadingMap`
   - `ServerDefaultMap=/Game/Spy/Maps/DevMap`

## 9. 범위 밖 (YAGNI)

- 메인 메뉴 및 메뉴→로딩 전환 흐름
- 로딩 화면 팁/배경 이미지 로테이션
- 로딩 취소·재시도
- 부팅 시 `LoadAllPrimaryAssetsSync` 를 비동기로 이전하는 리팩터링
