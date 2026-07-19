# SKAssetCore Plugin — Design Spec
Date: 2026-07-19

## 개요

현재 `SkillProject` 게임 모듈 안에 있는 `USpyAssetManager`(에셋 로딩 매니저)와 `USKAssetData`(이름→경로 룩업 베이스)를 **프로젝트 비의존 런타임 플러그인 `SKAssetCore`**로 분리한다. 목적은 이 AssetManager 인프라를 다른 UE 프로젝트에 폴더 복사 + 플러그인 Enable만으로 재사용하는 것이다.

`SpyAssetManager`는 이미 거의 프로젝트 비의존 상태다. 유일한 실질 결합은 `GetAssetData()`가 반환하는 `USpyAssetData`인데, 이는 `USKAssetData`의 빈 서브클래스일 뿐이고 실제 룩업 로직은 이미 `USKAssetData`(SK 접두사, 재사용 의도)에 있다. 따라서 분리는 "코드 대이동"이 아니라 두 클래스를 플러그인으로 옮기고 프로젝트 고유 부분만 얇은 서브클래스로 남기는 작업이다.

이미 존재하는 `SKGAS`(프로젝트 비의존 GAS 래퍼 모듈)와 동일한 `SK` 관례를 따른다.

---

## 1. 결정 사항 (확정)

| 항목 | 결정 | 비고 |
|------|------|------|
| 패키징 형태 | **플러그인** (Runtime 모듈 1개) | 폴더 복사 + `.uproject` Enable로 드롭인. 소스 모듈보다 초기 셋업 약간 더 들지만 타 프로젝트 재사용 최적 |
| 로딩 기능 범위 | **Spy 기반 + 진행률 콜백 훅** | Spy의 동기/비동기/언로드/캐싱 유지 + `OnLoadProgress` 신규 virtual 훅. Lyra StartupJob 시스템은 도입하지 않음 (YAGNI) |
| 소비 패턴 | **얕은 서브클래스 유지** | `USpyAssetManager : USKAssetManager`, `USpyAssetData : USKAssetData` 를 SkillProject에 남김. 프로젝트 확장점 + ini 무변경 확보 |
| 진행률 훅 방식 | **신규 virtual `OnLoadProgress`** | 엔진 `UAssetManager`에는 없는 완전 신규 함수. `USKAssetManager`에서 신규 선언(빈 구현), `USpyAssetManager`에서 override |

---

## 2. 플러그인 구조

### 신규 플러그인: `SKAssetCore` (Runtime)

```
SkillProject/Plugins/SKAssetCore/
├── SKAssetCore.uplugin
└── Source/SKAssetCore/
    ├── SKAssetCore.Build.cs
    ├── Public/
    │   ├── SKAssetManager.h
    │   └── SKAssetData.h
    └── Private/
        ├── SKAssetManager.cpp
        └── SKAssetData.cpp
```

### SKAssetCore.uplugin

- `"Type": "Runtime"`, `"LoadingPhase": "Default"`
- 모듈 1개: `SKAssetCore` (Type: Runtime)
- `"Installed": false`, 프로젝트 플러그인으로 배치

### SKAssetCore.Build.cs 의존 모듈

```
PublicDependencyModuleNames:
    "Core", "CoreUObject", "Engine"
```

> **DataValidation 추가 불필요**: `SKAssetData.cpp`의 `#if WITH_EDITOR` 검증은 `Misc/DataValidation.h`(`FDataValidationContext`)를 쓰지만, 이 헤더는 `Core/CoreUObject/Engine` 세트로 해결된다. 근거: 현재 `SkillProject.Build.cs`에 `DataValidation`/`UnrealEd` 모듈이 없는데도 `SKAssetData.cpp`가 정상 컴파일된다. 동일 base 세트를 가진 플러그인도 컴파일된다. (빌드 시 unresolved 발생하면 그때 `DataValidation` 모듈을 `bBuildEditor` 조건부로 추가.)
>
> 참고: `NativeGameplayTags`는 현재 `SpyAssetManager.h`가 include하지만 매니저 본문에서 사용하지 않는다 → 이동 시 제거 대상. GAS 관련 의존성은 플러그인에 넣지 않는다.

---

## 3. 플러그인에 들어가는 클래스 (프로젝트 비의존)

### 3.1 `USKAssetData : public UPrimaryDataAsset`

현재 `SkillProject/Source/SkillProject/Data/SKAssetData.{h,cpp}`를 그대로 이동. 내용 거의 무변경.

- 구조체 `FAssetEntry`, `FAssetSet` 동반 이동
- `GetAssetPathByName(const FName&) const`
- `#if WITH_EDITOR`: `PreSave`(룩업 맵 빌드), `IsDataValid`(빈 이름/경로 검증)
- API 매크로: `SKILLPROJECT_API` → **`SKASSETCORE_API`**로 교체

**변경 필요 1곳 — 역참조 제거:**

```cpp
// 현재 (SKAssetData.cpp:13) — SkillProject 모듈로의 역결합
const USKAssetData& USKAssetData::Get()
{
    return USpyAssetManager::Get().GetAssetData();
}

// 변경 후 — 베이스 매니저 참조 (플러그인 자기완결)
const USKAssetData& USKAssetData::Get()
{
    return USKAssetManager::Get().GetAssetData();
}
```

`USKAssetManager::Get()`은 실제 등록된 인스턴스(`USpyAssetManager`)를 base로 캐스팅해 반환하고, `GetAssetData()`는 `const USKAssetData&`를 반환하므로 정상 동작한다.

### 3.2 `USKAssetManager : public UAssetManager`

현재 `USpyAssetManager`의 로직 전체를 이 베이스로 이동. `UCLASS(Config=Game)`.

**public 정적/템플릿 API (무변경 이동):**
- `static USKAssetManager& Get()` — 로그 문구만 `[SKAssetManager]`로
- `GetAssetByPath<T>`, `GetAssetByName<T>`, `GetSubclassByPath<T>`, `GetSubclassByName<T>` (헤더 인라인 템플릿)
- `static UObject* LoadAssetSync(const FSoftObjectPath&)`
- `static void LoadAssetAsync(const FSoftObjectPath&, const FSKAssetAndDelegate&)`
- `static void UnloadAsset(const FSoftObjectPath&)`
- 델리게이트 `DECLARE_DELEGATE_OneParam(FSKAssetAndDelegate, UObject*)` (기존 `FSpyAssetAndDelegate` 개명)

**protected:**
- `virtual void StartInitialLoading() override` → `LoadAllPrimaryAssetsSync()` 호출
- `LoadPrimaryAssetSync(...)`, `LoadAllPrimaryAssetsSync()`, `LoadPrimaryAssetsAsync(...)`
- `UnloadAllPrimaryAssets()`, `UnloadAllAssets()`
- `LogProgress`, `AddLoadedAsset`, `RemoveLoadedAsset`, `AddPrimaryAsset`, `RemovePrimaryAsset`
- `template<typename GameDataClass> const GameDataClass& GetOrLoadTypedGameData(const TSoftObjectPtr<GameDataClass>&)` — 서브클래스가 타입별 게터를 추가할 확장점
- **신규**: `virtual void OnLoadProgress(int32 Loaded, int32 Total) {}` — 빈 기본 구현

**public:**
- `const USKAssetData& GetAssetData()` — `GetOrLoadTypedGameData<USKAssetData>(AssetDataPath)` (반환 타입이 `USpyAssetData`→`USKAssetData`로 변경됨)

**멤버:**
- `TSet<TObjectPtr<const UObject>> LoadedAssets`
- `TMap<TObjectPtr<UClass>, TObjectPtr<UPrimaryDataAsset>> GameDataMap`
- `FCriticalSection LoadedAssetsCritical`
- `UPROPERTY(Config) TSoftObjectPtr<USKAssetData> AssetDataPath`

**진행률 훅 연결 (신규):**

```cpp
void USKAssetManager::LogProgress(int32 Loaded, int32 Total)
{
    const int32 Percent = FMath::Clamp(FMath::RoundToInt((float)Loaded / (float)Total * 100.f), 1, 100);
    UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Loading %d%% (%d / %d)"), Percent, Loaded, Total);

    OnLoadProgress(Loaded, Total);   //# ← 신규 훅 호출
}
```

**제거 대상 include:**
- `#include "Util/DefineEnum.h"` (미사용)
- `#include "NativeGameplayTags.h"` (미사용)
- `#include "Blueprint/UserWidget.h"` (cpp, 미사용)
- `#include "Data/SpyAssetData.h"` → `#include "SKAssetData.h"`

---

## 4. SkillProject에 남는 클래스 (소비 예시 = 타 프로젝트 표준 패턴)

위치: `SkillProject/Source/SkillProject/Manager/SpyAssetManager.{h,cpp}`, `.../Data/SpyAssetData.h`

```cpp
//# SpyAssetManager.h
#include "SKAssetManager.h"
#include "SpyAssetManager.generated.h"

UCLASS(Config=Game)
class SKILLPROJECT_API USpyAssetManager : public USKAssetManager
{
    GENERATED_BODY()
protected:
    //# 프로젝트 확장점: 진행률 → 로딩스크린 UI 연동 지점
    virtual void OnLoadProgress(int32 Loaded, int32 Total) override;
};
```

```cpp
//# SpyAssetData.h (무변경 유지)
UCLASS()
class SKILLPROJECT_API USpyAssetData : public USKAssetData { GENERATED_BODY() };
```

- `USpyAssetManager::OnLoadProgress` 초기 구현은 최소(로그 또는 빈 본문)로 두고, 추후 로딩스크린 위젯 연동 지점으로 사용.
- `USpyAssetData`는 `PrimaryAssetTypesToScan`이 참조하는 구체 타입이므로 반드시 유지.

---

## 5. 마이그레이션 영향 및 처리

### 5.1 `GetAssetData()` 반환 타입 변경 (호출부 3곳)

베이스 `GetAssetData()`가 `const USpyAssetData&` → `const USKAssetData&`를 반환하므로, 로컬 변수를 `const USKAssetData&`로 바인딩하는 호출부를 수정한다. `USpyAssetData`가 빈 클래스라 호출부는 상속된 `GetAssetPathByName`만 사용 → 동작 무변경.

| 파일:라인 | 변경 |
|-----------|------|
| `SpyCharacter.cpp:300` | `const USpyAssetData& SKAssetData` → `const USKAssetData& SKAssetData` |
| `SpyUIManager.cpp:43` | `const USpyAssetData& AssetData` → `const USKAssetData& AssetData` |
| `SpyUIManager.cpp:143` | 동일 |

> 대안(참고): `USpyAssetManager`에 covariant 반환(`virtual const USpyAssetData& GetAssetData()`)을 두면 호출부 무변경 가능하나, 얕은 서브클래스 취지상 3줄 수정이 더 단순 → 3줄 수정 채택.

### 5.2 Build.cs 의존성 추가

| 파일 | 변경 |
|------|------|
| `SkillProject.Build.cs` | `PublicDependencyModuleNames`에 `"SKAssetCore"` 추가 |
| `SpyDataEditorTool.Build.cs` | 의존성에 `"SKAssetCore"` 추가 (SSpyAssetsTab이 `USKAssetData`/`USpyAssetData` 참조) |

### 5.3 uproject 등록

| 파일 | 변경 |
|------|------|
| `SkillProject.uproject` | `Plugins` 배열에 `{ "Name": "SKAssetCore", "Enabled": true }` 추가 |

### 5.4 ini — **무변경**

`USpyAssetManager`·`USpyAssetData`를 SkillProject 모듈에 그대로 남기므로 클래스 경로 문자열이 불변 → 아래 세 설정 모두 손대지 않는다.

- `DefaultEngine.ini:66` — `AssetManagerClassName=/Script/SkillProject.SpyAssetManager`
- `DefaultGame.ini` `[/Script/Engine.AssetManagerSettings]` — `PrimaryAssetTypesToScan`의 `SpyAssetData` 항목 (`AssetBaseClass=/Script/SkillProject.SpyAssetData`)
- `DefaultGame.ini:22-23` — `[/Script/SkillProject.SpyAssetManager]` / `AssetDataPath=/Game/Spy/Data/SpyAssetData.SpyAssetData`

### 5.5 모듈 순환 의존 방지 (검증 완료)

`SKAssetCore`로 이동하는 파일은 `SkillProject` 모듈 헤더를 include하지 않는다. 유일한 역참조였던 `SKAssetData.cpp`의 `USpyAssetManager::Get()`은 §3.1에서 `USKAssetManager::Get()`으로 교체하여 제거된다. 의존 방향: `SkillProject` → `SKAssetCore` (단방향).

---

## 6. 타 프로젝트 적용 절차 (산출물 사용법)

1. `Plugins/SKAssetCore` 폴더를 대상 프로젝트의 `Plugins/`로 복사
2. 대상 `.uproject`의 `Plugins`에 `{ "Name": "SKAssetCore", "Enabled": true }` 추가
3. 대상 게임 모듈에 얇은 서브클래스 2개 작성:
   - `UMyAssetManager : public USKAssetManager` (필요 시 `OnLoadProgress` override + 타입별 게터 추가)
   - `UMyAssetData : public USKAssetData`
4. 대상 게임 모듈 Build.cs에 `"SKAssetCore"` 의존성 추가
5. ini 3설정 지정:
   - `DefaultEngine.ini`: `AssetManagerClassName=/Script/<Module>.MyAssetManager`
   - `DefaultGame.ini` `[/Script/Engine.AssetManagerSettings]`: `PrimaryAssetTypesToScan`에 `MyAssetData` 항목
   - `DefaultGame.ini` `[/Script/<Module>.MyAssetManager]`: `AssetDataPath=...`

---

## 7. 검증 (Verification)

빌드는 Unreal Editor / Visual Studio에서 수행(본 리포는 CLI 컴파일 루프 없음). 완료 판정 기준:

1. **컴파일**: `SKAssetCore` 플러그인 + `SkillProject` + `SpyDataEditorTool` 모듈이 에러 없이 빌드
2. **런타임 부트**: 에디터 실행 시 `USpyAssetManager::Get()` 정상 반환, `StartInitialLoading` 로그(`# [SKAssetManager] Scan AssetType: SpyAssetData ...` → `... All Primary Asset Sync Load Complete`) 출력
3. **에셋 접근**: `GetAssetByName` 경로(예: SpyCharacter 로드)가 기존과 동일하게 동작 — 캐릭터/UI 정상 로드
4. **진행률 훅**: `USpyAssetManager::OnLoadProgress`가 로딩 중 호출됨(로그로 확인)
5. **데이터 검증**: 에디터에서 `SpyAssetData` 저장 시 `PreSave` 룩업 맵 빌드 및 `IsDataValid` 정상 동작
6. **ini 무변경 확인**: `AssetDataPath`가 런타임에 null로 resolve되지 않음(부팅 로그에 에셋 스캔/로드 정상)

---

## 8. 리스크 및 대응

| 리스크 | 대응 |
|--------|------|
| `AssetDataPath` 런타임 null resolve | ini 무변경이지만 서브클래스 유지가 전제 → 부팅 로그로 로드 성공 확인(검증 6) |
| `SpyDataEditorTool` 링크 실패 | Build.cs에 `SKAssetCore` 추가 누락 여부 우선 점검 |
| covariant/shadow 혼동 | `GetAssetData()`는 베이스 단일 정의, 서브클래스 override 없음. 호출부 3줄만 타입 변경 |
| generated.h 경로 | 클래스 이동 후 `Refresh Visual Studio Project` 재생성 필요 (CLAUDE.md 규칙) |

---

## 9. 범위 외 (Non-Goals)

- Lyra `FLyraAssetManagerStartupJob` 가중치 큐 시스템 도입 (YAGNI — 진행률 훅으로 충분)
- `DumpLoadedAssets` 콘솔 명령, `PreBeginPIE` 프리로드 (필요 시 별도 작업)
- `SpyUIManager` 등 다른 매니저의 모듈 분리 (본 스펙은 AssetManager 한정)
- 기존 로딩 동작/성능 변경 (동작 보존이 원칙)
