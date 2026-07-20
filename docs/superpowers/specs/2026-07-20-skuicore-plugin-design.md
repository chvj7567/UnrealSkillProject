# SKUICore 플러그인 분리 — Design Spec
Date: 2026-07-20

## 개요

SkillProject 게임 모듈에 있는 `USpyUIManager`(UI 오픈/캐싱/재사용 서브시스템)와 `USKUserWidget`(UI 위젯 베이스)을 **프로젝트 비의존 UI 플러그인 `SKUICore`**로 분리한다. 목적은 SKAssetCore와 동일 — 이 UI 인프라를 다른 프로젝트에 폴더 복사 + 플러그인 Enable로 재사용.

**이 분리의 핵심 가치는 "UI 캐싱·재사용" 로직**이다. `USpyUIManager`는 UI를 열 때 (1) 이미 열린 UI(`OpenUIList`)인지, (2) 캐싱된 UI(`CashingUIList`)인지 먼저 확인해 **중복 생성을 피하고 재사용**하며, 닫을 때 파괴하지 않고 `CashingUIList`에 넣어 다음 오픈 때 재활용한다(LRU 유사, `MaxCashingUICount`). 이 캐싱/재사용 엔진이 곧 재사용 인프라의 알맹이이며, 플러그인 베이스 `USKUIManager`에 **동작 그대로** 옮겨야 한다.

`USKUserWidget`(SK 접두사, 프로젝트 비의존)과 `USpyUserWidget : USKUserWidget`은 이미 SKAssetData/SpyAssetData와 같은 SK-베이스/프로젝트-서브 계층으로 나뉘어 있어, 분리는 이 경계를 그대로 활용한다.

---

## 1. 결정 사항 (확정)

| 항목 | 결정 | 비고 |
|------|------|------|
| 패키징 | **신규 플러그인 `SKUICore`** (SKAssetCore에 합치지 않음) | UI는 UMG/SlateCore 의존 → 에셋 전용 SKAssetCore를 오염시키지 않음. 의존: SKAssetCore + UMG + SlateCore |
| 소비 패턴 | **서브클래스 유지** `USpyUIManager : USKUIManager`, `USpyUserWidget : USKUserWidget` | 호출부 무변경. AssetManager와 동일 원칙 |
| 서브시스템 접근자 | **`ShouldCreateSubsystem`(파생 있으면 false) + base `Get()`=`GetSubsystemArray`** | GameInstanceSubsystem은 비-abstract 서브클래스를 전부 자동 생성 → 인스턴스 분리 방지 (§6 #1 리스크) |
| 이넘 처리 | **`ESpyUIType`는 SkillProject에 유지, 플러그인 경계 안 넘음** | 플러그인 베이스는 **FName API**만. 이넘→FName 변환 오버로드는 서브클래스에 |
| 캐싱/재사용 | **`OpenUIList`/`CashingUIList` + 재사용 로직 전부 base로, 동작 보존** | 이 분리의 핵심. §3.2 |
| 위젯 리스트 타입 | `TArray<USpyUserWidget>` → **`TArray<USKUserWidget>`** 일반화 | 모든 사용 메서드(GetUIName/AddToViewport/RemoveFromParent/Close)가 USKUserWidget 소속 → 클린 |

---

## 2. 플러그인 구조

```
SkillProject/Plugins/SKUICore/
├── SKUICore.uplugin                 # SKAssetCore 플러그인 의존 명시
└── Source/SKUICore/
    ├── SKUICore.Build.cs            # Core, CoreUObject, Engine, UMG, SlateCore, SKAssetCore
    ├── Public/
    │   ├── SKUIManager.h
    │   └── SKUserWidget.h
    └── Private/
        ├── SKUICoreModule.cpp       # IMPLEMENT_MODULE
        ├── SKUIManager.cpp
        └── SKUserWidget.cpp
```

### SKUICore.uplugin
- `"Type": "Runtime"`, `"LoadingPhase": "Default"`, 모듈 1개 `SKUICore`
- `"Plugins": [ { "Name": "SKAssetCore", "Enabled": true } ]` (플러그인-간 의존)

### SKUICore.Build.cs
```
PublicDependencyModuleNames:
    "Core", "CoreUObject", "Engine", "UMG", "SlateCore", "SKAssetCore"
```
> UMG: `UUserWidget`/`CreateWidget`/`UWidgetComponent`/`EWidgetSpace`/`AddToViewport`. SlateCore: `FReply`/`FGeometry`/`FPointerEvent`(USKUserWidget 입력 오버라이드). SKAssetCore: `USKAssetManager`/`USKAssetData`.

---

## 3. 플러그인에 들어가는 클래스 (프로젝트 비의존)

### 3.1 `USKUserWidget : public UUserWidget`

현재 `SkillProject/Source/SkillProject/UI/SKUserWidget.{h,cpp}` 를 이동. 내용 거의 무변경.
- 유지: 입력 소비(`bConsumePointerInput` + 8개 Native*Input 오버라이드), `GetUIName`/`SetUIName`, `SetConsumePointerInput`, `virtual Close()`, `UIName` 프로퍼티.
- API 매크로: `SKILLPROJECT_API` → **`SKUICORE_API`**.
- **변경 1곳 (역참조 제거):** `SKUserWidget.cpp`
  - `#include "Manager/SpyUIManager.h"` → `#include "SKUIManager.h"`
  - `void USKUserWidget::Close() { USpyUIManager::Get(this)->CloseUI(UIName); }` → `USKUIManager::Get(this)->CloseUI(UIName);`

### 3.2 `USKUIManager : public UGameInstanceSubsystem` — 캐싱/재사용 엔진 (핵심)

`USpyUIManager`의 **FName 기반 로직 전체**를 이 베이스로 이동. `USpyUserWidget`→`USKUserWidget`, `USpyAssetManager`→`USKAssetManager` 일반화.

**public (FName API — 프로젝트 비의존):**
- `static USKUIManager* Get(const UObject* WorldContextObject)` — §6 가드된 구현
- `void OpenUI(FName InUIName)` / `void CloseUI(FName InUIName)` / `void CloseLastUI()`
- `void OpenSubUI(FName InUIName, UWidgetComponent*, EWidgetSpace)`
- `void AddCashingUI(USKUserWidget* UserWidget)`

**protected 멤버 (캐싱/재사용 상태 — 동작 그대로 보존):**
- `const int MaxCashingUICount = 5;`
- `UPROPERTY() TArray<TObjectPtr<USKUserWidget>> OpenUIList;`
- `UPROPERTY() TArray<TObjectPtr<USKUserWidget>> CashingUIList;`

**보존해야 할 캐싱/재사용 로직 (OpenUI 내부 — 동작 불변):**
1. 에셋 이름→경로 룩업(`USKAssetManager::Get().GetAssetData().GetAssetPathByName`) + `_C` 접미사 처리 + `LoadAssetAsync`(비동기).
2. 로드 콜백에서 **재사용 판정 3단계**:
   - `OpenUIList`에 같은 `GetUIName()` 있으면 → **중복 오픈 무시** (로그 "Already Opening UI").
   - `CashingUIList`에 있으면 → **캐시에서 꺼내 재사용**: `OpenUIList.Add` + `AddToViewport` (로그 "Cashing Opening UI").
   - 둘 다 없으면 → **신규 생성**: `CreateWidget<USKUserWidget>(World, UI)` + `SetUIName` + `OpenUIList.Add` + `AddToViewport`.
3. `CloseUI`: `OpenUIList`에서 찾아 → 제거 + `AddCashingUI`(파괴 대신 캐싱) + `RemoveFromParent`.
4. `AddCashingUI`: 동일 이름 중복 방지, `CashingUIList.Add`, `Num >= MaxCashingUICount`면 가장 오래된 것(`RemoveAt(0)`) 제거 (LRU 유사).
5. `OpenSubUI`: `WidgetComponent`에 위젯 클래스 세팅(`SetWidgetClass`/`SetWidgetSpace`/`InitWidget`) — WidgetComponent 경로.

> 위 로직은 `USpyUserWidget`을 `USKUserWidget`으로 바꾸는 것 외에 **한 줄도 동작을 바꾸지 않는다.** `GetSubclassByName<USKUserWidget>`도 그대로.

**Initialize/Deinitialize:** 현재 비어있음(Super 호출만) — 그대로 이동.

**제거 대상 include (일반화로 불필요):** `UI/SpyUserWidget.h`, `Character/SpyCharacter.h`(미사용), `Util/DefineEnum.h`(이넘은 서브클래스로).
**추가 include:** `SKUserWidget.h`(같은 플러그인), `SKAssetManager.h`/`SKAssetData.h`(SKAssetCore), UMG 관련.

**API 매크로:** `SKUICORE_API`.

---

## 4. SkillProject에 남는 클래스 (소비 예시)

### 4.1 `USpyUIManager : public USKUIManager`
```cpp
UCLASS()
class SKILLPROJECT_API USpyUIManager : public USKUIManager
{
    GENERATED_BODY()
public:
    //# leaf 서브시스템 인스턴스 접근 (호출부가 쓰는 형태)
    static USpyUIManager* Get(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable) void OpenSpyUI(ESpyUIType UIType);
    UFUNCTION(BlueprintCallable) void CloseSpyUI(ESpyUIType UIType);
    UFUNCTION(BlueprintCallable) void OpenSubSpyUI(ESpyUIType UIType, UWidgetComponent* WidgetComponent, EWidgetSpace Space);
};
```
- `#include "Util/DefineEnum.h"`(ESpyUIType), `#include "SKUIManager.h"`.
- `Get()` = `World→GameInstance→GetSubsystem<USpyUIManager>()` (leaf 정확 클래스, `USpyUIManager*` 반환).
- 오버로드 구현: `StaticEnum<ESpyUIType>()->GetNameStringByValue(...)` → `OpenUI(FName)` 등 (base 호출). **현재 구현 그대로.**

### 4.2 `USpyUserWidget : public USKUserWidget` / `USpyHPBar : public USKUserWidget`
- 코드 무변경. `#include "UI/SKUserWidget.h"` → `#include "SKUserWidget.h"` (플러그인) 만.

---

## 5. 마이그레이션 영향 및 처리

### 5.1 include 경로 스왑 (3곳)
| 파일:라인 | 변경 |
|---|---|
| `UI/SpyUserWidget.h:6` | `#include "UI/SKUserWidget.h"` → `#include "SKUserWidget.h"` |
| `UI/SpyHPBar.h:6` | 동일 |
| (이동된) `SKUserWidget.cpp:5` | `#include "Manager/SpyUIManager.h"` → `#include "SKUIManager.h"` (+ Close()의 `USpyUIManager::Get`→`USKUIManager::Get`) |

### 5.2 호출부 — **무변경 (4곳)**
`USpyUIManager`가 `Manager/SpyUIManager.h` 동일 경로에 남고 `ESpyUIType` 오버로드·`Get()`를 서브클래스가 유지하므로:
- `SpyCharacter.cpp:137` `USpyUIManager::Get(this)->OpenSubSpyUI(ESpyUIType::HpBar, ...)`
- `SpyPlayerController.cpp:39` `OpenSpyUI(ESpyUIType::MainHUD)`
- `SpyMainHUD.cpp:33` `OpenSpyUI(ESpyUIType::Menu)`
- `SpyUserWidget.cpp:30` `CloseSpyUI(...)`
모두 그대로 컴파일된다. (해당 파일들의 `#include "Manager/SpyUIManager.h"`, `#include "UI/SpyUserWidget.h"`도 무변경.)

### 5.3 Build 배선
| 파일 | 변경 |
|---|---|
| `SkillProject.Build.cs` | `PublicDependencyModuleNames`에 `"SKUICore"` 추가 |
| `SkillProject.uproject` | `Plugins[]`에 `{ "Name":"SKUICore","Enabled":true }`; `Modules[0].AdditionalDependencies`에 `"SKUICore"` |

> SpyDataEditorTool 등은 UI 매니저를 참조하지 않으므로 무변경.

### 5.4 순환 의존 방지 (검증됨)
`SKUICore`로 이동하는 파일(`SKUIManager`, `SKUserWidget`)이 참조하는 SkillProject 헤더:
- `SKUserWidget.cpp` → `Manager/SpyUIManager.h` (유일 역참조) → §3.1에서 `SKUIManager.h`로 교체.
- `SKUIManager` 로직 → `UI/SpyUserWidget.h`(일반화로 제거), `Character/SpyCharacter.h`(미사용 제거).
잔여 역참조 0. 의존 방향: `SkillProject` → `SKUICore` → `SKAssetCore`/UMG (단방향).

---

## 6. 서브시스템 인스턴스 분리 — #1 리스크 (컴파일로 못 잡음)

`USKUIManager`(base)와 `USpyUIManager`(leaf)가 둘 다 비-abstract `UGameInstanceSubsystem`이면 **UE가 둘 다 자동 생성**한다. 그러면 `OpenUI`는 `USpyUIManager::Get()`이 준 인스턴스의 `OpenUIList`에 담기는데, `USKUserWidget::Close()`가 `GetSubsystemArray<USKUIManager>()[0]`로 **다른 인스턴스**(빈 리스트)를 잡으면 → **UI가 열리는데 안 닫히는 무증상 버그**.

**해결 (safe-by-construction):**
- `USKUIManager::ShouldCreateSubsystem(UObject* Outer)` 오버라이드:
  ```cpp
  TArray<UClass*> Derived;
  GetDerivedClasses(GetClass(), Derived, false);
  return Derived.Num() == 0;   //# 파생 클래스가 있으면 base 는 생성하지 않음 → leaf(USpyUIManager)만 생성
  ```
- `USKUIManager::Get(WorldContext)`:
  ```cpp
  if (UGameInstance* GI = ...)
  {
      const TArray<USKUIManager*>& Arr = GI->GetSubsystemArray<USKUIManager>();
      checkf(Arr.Num() <= 1, TEXT("USKUIManager 인스턴스가 2개 이상 — ShouldCreateSubsystem 확인"));
      return Arr.Num() > 0 ? Arr[0] : nullptr;
  }
  ```
  → base·leaf 모두 이 하나의 leaf 인스턴스를 가리킨다. `USpyUIManager::Get()`(GetSubsystem<USpyUIManager>)와 동일 인스턴스.

---

## 7. 검증 (Verification)

빌드/실행은 사용자가 **에디터 닫고 VS 풀 빌드**로 수행(메모리 제약 + 구조 변경이라 Live Coding 불가 — [[skillproject-build-constraints]]). 완료 판정:

1. **컴파일**: `SKUICore` 플러그인 + `SkillProject` 에러 없이 빌드.
2. **서브시스템 단일 인스턴스**: 실행 시 `USKUIManager` 인스턴스가 1개(leaf `USpyUIManager`)뿐. `Get()`의 `checkf(Num<=1)` 미발동.
3. **캐싱/재사용 왕복 (핵심 검증)** — 게임에서:
   - UI 열기 → 닫기 → **다시 열기**: 두 번째 오픈이 "Cashing Opening UI" 로그(재생성 아님)로 뜨는지 → **캐싱 재사용 동작 확인**.
   - 같은 UI 두 번 연속 열기 → "Already Opening UI"(중복 무시) 확인.
   - `MaxCashingUICount`(5) 초과로 여러 UI 캐싱 → 가장 오래된 것 제거 확인.
4. **`USKUserWidget::Close()` 경로**: 위젯 자체 Close()로 닫아도 정상 닫힘(인스턴스 분리 없음) — #1 리스크 직접 검증.
5. **OpenSpyUI/OpenSubSpyUI(이넘 경로)**: MainHUD/Menu/HpBar 등 기존과 동일 동작.

---

## 8. 리스크 및 대응

| 리스크 | 대응 |
|--------|------|
| 서브시스템 인스턴스 분리(무증상) | `ShouldCreateSubsystem` + 가드된 `Get()`(§6), open→close 왕복 검증(§7.3~4) |
| 캐싱/재사용 동작 회귀 | 로직 한 줄도 안 바꾸고 타입만 일반화. §7.3에서 로그로 재사용 확인 |
| 순환 의존 | 역참조 1곳(SKUserWidget.cpp) 교체 후 0 (§5.4) |
| UMG/SlateCore 누락 빌드 에러 | `SKUICore.Build.cs`에 명시(§2) |
| generated.h/모듈 인식 | 신규 플러그인 → `Refresh VS Project` 재생성 필요 |

---

## 9. 범위 외 (Non-Goals)

- `SpyUserWidget`/`SpyHPBar`/`SpyMainHUD`/`SpyMenu` 등 프로젝트 UI 위젯 이동 (프로젝트 전용 — 잔류)
- `ESpyUIType` 이동 (프로젝트 UI 목록 — 잔류)
- UI 매니저 기능 확장(새 캐싱 정책·애니메이션 등) — 순수 이동, 동작 보존
- SpyDataEditorTool 등 다른 모듈 변경
