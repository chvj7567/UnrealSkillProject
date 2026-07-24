# 로딩 UI 뷰포트 레이어 이전 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 로딩 위젯을 월드 소유에서 **뷰포트 레이어 소유**로 옮겨 `OpenLevel` 트래블을 넘어 살아남게 하고, 위젯이 늦게 뜨는 지연과 데디 서버에서 UI 를 여는 낭비를 함께 제거한다.

**선행 플랜:** `docs/superpowers/plans/2026-07-22-loading-scene.md` (Task 1~8). 그 산출물이 이미 구현·검증된 상태를 전제로 한다.

**Architecture:** `USKUIManager` 에 persistent UI 개념(`OpenPersistentUI` / `ClosePersistentUI`)을 추가한다. persistent UI 는 GameInstance 를 아우터로 생성하고 `UGameViewportClient::AddViewportWidgetContent` 로 Slate 뷰포트에 직접 얹어, 월드가 파괴돼도 살아남는다. 로딩 위젯의 소유·수명은 `ASpyLoadingGameMode` 에서 `USpyLoadingSubsystem`(GameInstance 수명) 으로 이관한다.

**Tech Stack:** Unreal Engine 5.7 / C++ / SKUICore / SKAssetCore / UMG (`UGameViewportClient`, `FCoreUObjectDelegates::PostLoadMapWithWorld`)

## Global Constraints

- `.claude/rules/cpp-style.md` 전항 준수: 주석 `//#`, 단항 `!` 금지(`== false` / `== nullptr`), `TObjectPtr`, include 순서(자기자신→UE→프로젝트→generated), `Super::` 누락 금지.
- 플러그인은 게임 모듈 헤더를 include 하지 않는다 (`unreal-infra.md` §1).
- `plugin-skuicore.md` §2 "직접 `CreateWidget`+`AddToViewport` 금지" 룰은 **유지된다** — 이 플랜은 룰을 깨는 게 아니라 매니저 API 에 persistent 경로를 추가해 룰 안에서 해결한다. 게임 모듈은 여전히 매니저만 호출한다.
- 기존 `OpenUI` / `CloseUI` / `OpenSubUI` 의 동작을 **바꾸지 않는다.** 기존 호출부(MainHUD·HpBar·Menu·GrapplePrompt)에 영향이 없어야 한다.
- `git commit` 금지 — `git add` 까지만 하고 커밋 메시지(안) 제시.
- 빌드·에디터 실행은 사람이 수행한다.

## 이 플랜이 해결하는 실측 문제 3건

Task 8 검증 로그에서 확인된 사실이다.

| # | 증상 | 로그 근거 | 원인 |
|---|---|---|---|
| 1 | 트래블 시 로딩 위젯 소멸 → 검은 구간 | `Took 0.147283s to LoadMap(DevMap)` 구간이 무보호 | `CreateWidget(GetWorld(), ...)` — 월드 소유 |
| 2 | 위젯이 로딩 시작보다 **1.23초 늦게** 표시 | `로딩 시작 16:17:16.244` → `New Opening UI: Loading 16:17:17.471` | `OpenUI` 가 위젯 클래스를 비동기 로드 |
| 3 | 데디 서버가 UI 를 열고 `WBP_Loading_C` 를 로드 | `No game viewport was found` + `New Opening UI: Loading` (server 로그) | GameMode 가 UI 를 열고, 서버엔 뷰포트가 없음 |

T3(트래블) = **0.147초**로 짧으므로 **MoviePlayer 는 도입하지 않는다.** 뷰포트 레이어 위젯으로 충분하다.

---

## File Structure

| 파일 | 변경 |
|---|---|
| `SkillProject/Plugins/SKUICore/Source/SKUICore/Public/SKUIManager.h` | persistent UI API 3개 + 보관 배열 |
| `SkillProject/Plugins/SKUICore/Source/SKUICore/Private/SKUIManager.cpp` | 구현. 기존 함수 무변경 |
| `SkillProject/Source/SkillProject/Manager/SpyUIManager.h/.cpp` | `ESpyUIType` 래퍼 2개 |
| `SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.h/.cpp` | UI 소유·수명 이관, 트래블 후 닫기 |
| `SkillProject/Source/SkillProject/System/SpyLoadingGameMode.cpp` | UI 오픈 제거 (킥오프만 남김) |
| `.claude/rules/plugin-skuicore.md` | persistent UI 규칙 추가 |

**테스트 가능성**: 이 변경은 전부 뷰포트·월드·에셋에 의존해 Automation 유닛 테스트 대상이 아니다. 기존 `SkillProject.Manager.Loading.*` 20건이 **회귀 감시** 역할을 하고(순수 함수·`ApplyConfig` 는 건드리지 않으므로 전부 통과해야 한다), 새 동작은 Task 13 수동 검증으로 확인한다. 이 구분을 숨기지 않는다.

---

## Task 9: SKUICore — persistent UI API

**Files:**
- Modify: `SkillProject/Plugins/SKUICore/Source/SKUICore/Public/SKUIManager.h`
- Modify: `SkillProject/Plugins/SKUICore/Source/SKUICore/Private/SKUIManager.cpp`

**Interfaces:**
- Consumes: `USKAssetManager::GetSubclassByName<T>` / `LoadAssetSync`, `USKUserWidget::SetUIName`
- Produces:
  - `USKUserWidget* USKUIManager::OpenPersistentUI(FName InUIName, int32 ZOrder = 100)` — 성공 시 위젯, 실패 시 `nullptr`
  - `void USKUIManager::ClosePersistentUI(FName InUIName)`
  - `bool USKUIManager::IsPersistentUIOpen(FName InUIName) const`

- [ ] **Step 1: 헤더에 persistent API 선언**

`SKUIManager.h` 의 `AddCashingUI` 선언 아래에 추가한다.

```cpp
	UFUNCTION(BlueprintCallable)
	void AddCashingUI(USKUserWidget* UserWidget);

public:
	//# 트래블(맵 전환)을 넘어 살아남는 UI 를 연다.
	//# GameInstance 를 아우터로 생성하고 뷰포트 Slate 레이어에 직접 얹으므로 월드가 파괴돼도 유지된다.
	//# 로딩 화면처럼 맵 전환 중에도 계속 보여야 하는 UI 전용. 일반 UI 는 OpenUI 를 쓴다.
	//# 위젯 클래스를 동기 로드하므로 호출 즉시 화면에 뜬다.
	UFUNCTION(BlueprintCallable)
	USKUserWidget* OpenPersistentUI(FName InUIName, int32 ZOrder = 100);

	UFUNCTION(BlueprintCallable)
	void ClosePersistentUI(FName InUIName);

	UFUNCTION(BlueprintCallable)
	bool IsPersistentUIOpen(FName InUIName) const;

protected:
	//# persistent UI 는 OpenUIList 와 분리해 관리한다 (CloseLastUI 등 스택 동작에 섞이면 안 됨)
	UPROPERTY()
	TArray<TObjectPtr<USKUserWidget>> PersistentUIList;
```

- [ ] **Step 2: cpp 에 include 추가**

`SKUIManager.cpp` 상단 include 블록을 아래로 교체한다.

```cpp
#include "SKUIManager.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Blueprint/UserWidget.h"
#include "SKUserWidget.h"
#include "SKAssetManager.h"
#include "SKAssetData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKUIManager)
```

- [ ] **Step 3: `OpenPersistentUI` 구현**

`AddCashingUI` 구현 아래에 추가한다. 기존 `OpenUI` 와 달리 **동기 로드**한다 — 로딩 화면은 "지금 당장" 떠야 하고, 위젯 BP 자체는 작다(실측 30KB).

```cpp
USKUserWidget* USKUIManager::OpenPersistentUI(FName InUIName, int32 ZOrder)
{
	//# 이미 열려 있으면 그것을 돌려준다 (중복 생성 금지)
	for (const TObjectPtr<USKUserWidget>& Existing : PersistentUIList)
	{
		if (IsValid(Existing) && Existing->GetUIName() == InUIName)
		{
			UE_LOG(LogTemp, Warning, TEXT("Already Opening Persistent UI: %s"), *InUIName.ToString());
			return Existing;
		}
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance == nullptr)
	{
		return nullptr;
	}

	//# 데디케이티드 서버 등 뷰포트가 없는 환경에서는 아무것도 하지 않는다
	if (GEngine == nullptr || GEngine->GameViewport == nullptr)
	{
		UE_LOG(LogTemp, Log, TEXT("# [SKUIManager] 뷰포트가 없어 Persistent UI 를 열지 않습니다: %s"), *InUIName.ToString());
		return nullptr;
	}

	//# 위젯 클래스 동기 로드 — 비동기로 하면 표시가 지연된다
	TSubclassOf<USKUserWidget> WidgetClass = USKAssetManager::GetSubclassByName<USKUserWidget>(InUIName);
	if (WidgetClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("# [SKUIManager] Persistent UI 클래스를 찾을 수 없습니다: %s"), *InUIName.ToString());
		return nullptr;
	}

	//# GameInstance 를 아우터로 생성 — 월드가 파괴돼도 위젯이 살아남는다
	USKUserWidget* UserWidget = CreateWidget<USKUserWidget>(GameInstance, WidgetClass);
	if (UserWidget == nullptr)
	{
		return nullptr;
	}

	UserWidget->SetUIName(InUIName);

	//# AddToViewport 가 아니라 Slate 뷰포트 콘텐츠로 직접 얹는다 (월드 비의존)
	GEngine->GameViewport->AddViewportWidgetContent(UserWidget->TakeWidget(), ZOrder);

	PersistentUIList.Add(UserWidget);

	UE_LOG(LogTemp, Log, TEXT("# [SKUIManager] New Persistent UI: %s (ZOrder %d)"), *InUIName.ToString(), ZOrder);

	return UserWidget;
}
```

- [ ] **Step 4: `ClosePersistentUI` / `IsPersistentUIOpen` 구현**

persistent UI 는 캐시 풀(`AddCashingUI`)에 넣지 않는다 — 캐시는 `OpenUI` 경로의 재사용 장치이고, 뷰포트 콘텐츠로 얹힌 위젯을 섞으면 `AddToViewport` 와 이중 부착이 된다.

```cpp
void USKUIManager::ClosePersistentUI(FName InUIName)
{
	for (int32 Index = PersistentUIList.Num() - 1; Index >= 0; --Index)
	{
		TObjectPtr<USKUserWidget> UserWidget = PersistentUIList[Index];

		if (IsValid(UserWidget) == false)
		{
			PersistentUIList.RemoveAt(Index);
			continue;
		}

		if (UserWidget->GetUIName() != InUIName)
		{
			continue;
		}

		if (GEngine != nullptr && GEngine->GameViewport != nullptr)
		{
			GEngine->GameViewport->RemoveViewportWidgetContent(UserWidget->TakeWidget());
		}

		PersistentUIList.RemoveAt(Index);

		UE_LOG(LogTemp, Log, TEXT("# [SKUIManager] Close Persistent UI: %s"), *InUIName.ToString());
	}
}

bool USKUIManager::IsPersistentUIOpen(FName InUIName) const
{
	for (const TObjectPtr<USKUserWidget>& UserWidget : PersistentUIList)
	{
		if (IsValid(UserWidget) && UserWidget->GetUIName() == InUIName)
		{
			return true;
		}
	}

	return false;
}
```

- [ ] **Step 5: `Deinitialize` 에서 정리**

기존 `Deinitialize` 를 아래로 교체한다. GameInstance 종료 시 뷰포트에 남은 persistent UI 를 걷어낸다.

```cpp
void USKUIManager::Deinitialize()
{
	//# 뷰포트에 얹어 둔 persistent UI 를 모두 걷어낸다
	if (GEngine != nullptr && GEngine->GameViewport != nullptr)
	{
		for (const TObjectPtr<USKUserWidget>& UserWidget : PersistentUIList)
		{
			if (IsValid(UserWidget))
			{
				GEngine->GameViewport->RemoveViewportWidgetContent(UserWidget->TakeWidget());
			}
		}
	}

	PersistentUIList.Empty();

	Super::Deinitialize();
}
```

- [ ] **Step 6: 컴파일 검증 (사람 수행)**

Visual Studio 빌드.
Expected: `SKUICore` 컴파일 성공. 기존 `OpenUI`/`CloseUI`/`OpenSubUI` 는 한 줄도 안 바뀌었으므로 기존 UI 호출부에 영향이 없어야 한다.

- [ ] **Step 7: Stage + 커밋 메시지(안)**

```bash
git add SkillProject/Plugins/SKUICore/Source/SKUICore/Public/SKUIManager.h \
        SkillProject/Plugins/SKUICore/Source/SKUICore/Private/SKUIManager.cpp
```

```
[Feature] SKUIManager — 트래블 생존 persistent UI API 추가
```

---

## Task 10: SpyUIManager — ESpyUIType 래퍼

**Files:**
- Modify: `SkillProject/Source/SkillProject/Manager/SpyUIManager.h`
- Modify: `SkillProject/Source/SkillProject/Manager/SpyUIManager.cpp`

**Interfaces:**
- Consumes: `USKUIManager::OpenPersistentUI` / `ClosePersistentUI` (Task 9)
- Produces:
  - `USKUserWidget* USpyUIManager::OpenPersistentSpyUI(ESpyUIType UIType, int32 ZOrder = 100)`
  - `void USpyUIManager::ClosePersistentSpyUI(ESpyUIType UIType)`

- [ ] **Step 1: 헤더에 선언 추가**

`SpyUIManager.h` 의 `OpenSubSpyUI` 선언 아래에 추가한다.

```cpp
	UFUNCTION(BlueprintCallable)
	void OpenSubSpyUI(ESpyUIType UIType, UWidgetComponent* WidgetComponent, EWidgetSpace Space);

public:
	//# 트래블을 넘어 유지되는 UI (로딩 화면 등)
	UFUNCTION(BlueprintCallable)
	USKUserWidget* OpenPersistentSpyUI(ESpyUIType UIType, int32 ZOrder = 100);

	UFUNCTION(BlueprintCallable)
	void ClosePersistentSpyUI(ESpyUIType UIType);
```

헤더 상단에 전방 선언이 없으면 추가한다.

```cpp
class USKUserWidget;
```

- [ ] **Step 2: 구현 추가**

`SpyUIManager.cpp` 의 `OpenSubSpyUI` 구현 아래에 추가한다. 기존 함수들과 동일하게 enum 이름 문자열을 등록명으로 쓴다.

```cpp
USKUserWidget* USpyUIManager::OpenPersistentSpyUI(ESpyUIType UIType, int32 ZOrder)
{
	FString EnumName = StaticEnum<ESpyUIType>()->GetNameStringByValue((int64)UIType);
	return OpenPersistentUI(FName(*EnumName), ZOrder);
}

void USpyUIManager::ClosePersistentSpyUI(ESpyUIType UIType)
{
	FString EnumName = StaticEnum<ESpyUIType>()->GetNameStringByValue((int64)UIType);
	ClosePersistentUI(FName(*EnumName));
}
```

- [ ] **Step 3: 컴파일 검증 (사람 수행)**

Expected: 빌드 성공.

- [ ] **Step 4: Stage + 커밋 메시지(안)**

```bash
git add SkillProject/Source/SkillProject/Manager/SpyUIManager.h \
        SkillProject/Source/SkillProject/Manager/SpyUIManager.cpp
```

```
[Feature] SpyUIManager — persistent UI 래퍼 추가
```

---

## Task 11: SpyLoadingSubsystem — UI 소유·수명 이관

**Files:**
- Modify: `SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.h`
- Modify: `SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.cpp`

**Interfaces:**
- Consumes: `USpyUIManager::OpenPersistentSpyUI` / `ClosePersistentSpyUI` (Task 10), `ESpyUIType::Loading`
- Produces: `StartLoading()` 이 UI 오픈까지 책임진다 (호출부는 변경 없음)

**설계 의도:** UI 오픈을 서브시스템으로 옮기면 세 가지가 동시에 해결된다 — (a) 서브시스템이 GameInstance 수명이라 트래블을 넘어 위젯을 계속 구동할 수 있고, (b) 데디 서버는 `ShouldCreateSubsystem` 이 `false` 라 UI 를 아예 열지 않으며(실측 문제 #3), (c) 동기 로드 경로를 타므로 위젯이 즉시 뜬다(실측 문제 #2).

- [ ] **Step 1: 헤더에 트래블 후처리 선언 추가**

`SpyLoadingSubsystem.h` 의 `protected:` 메서드 블록에 추가한다.

```cpp
	//# 게임플레이 맵으로 전환
	void TransitionToGameplayMap();

	//# 트래블 완료 시점 — 로딩 UI 를 내린다
	void HandlePostLoadMap(UWorld* LoadedWorld);
```

같은 헤더의 멤버 블록에 델리게이트 핸들을 추가한다.

```cpp
	float ElapsedSeconds = 0.f;

	//# PostLoadMapWithWorld 구독 핸들
	FDelegateHandle PostLoadMapHandle;
```

- [ ] **Step 2: cpp include 에 UI 매니저 추가**

`SpyLoadingSubsystem.cpp` 의 include 블록에 두 줄을 추가한다.

```cpp
#include "Manager/SpyAssetManager.h"
#include "Manager/SpyUIManager.h"
#include "Util/DefineEnum.h"
```

- [ ] **Step 3: `StartLoading` 에서 UI 를 직접 연다**

`StartLoading` 안, `bLoading = true;` 바로 **앞**에 UI 오픈과 트래블 델리게이트 구독을 넣는다. 위젯이 먼저 떠야 첫 브로드캐스트를 놓치지 않는다.

```cpp
	//# 콜백이 동기 실행될 수 있으므로 마지막에 켠다
	bLoading = true;
```

위 줄을 아래로 교체한다.

```cpp
	//# 로딩 UI 를 먼저 띄운다 — persistent 경로라 트래블을 넘어 유지되고, 동기 로드라 즉시 표시된다.
	//# 데디케이티드 서버는 이 서브시스템이 생성되지 않으므로 여기 도달하지 않는다.
	if (USpyUIManager* UIManager = USpyUIManager::Get(GetGameInstance()))
	{
		UIManager->OpenPersistentSpyUI(ESpyUIType::Loading);
	}

	//# 트래블 완료를 받아 UI 를 내린다
	if (PostLoadMapHandle.IsValid() == false)
	{
		PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &USpyLoadingSubsystem::HandlePostLoadMap);
	}

	//# 콜백이 동기 실행될 수 있으므로 마지막에 켠다
	bLoading = true;
```

- [ ] **Step 4: `HandlePostLoadMap` 구현**

`TransitionToGameplayMap` 구현 아래에 추가한다. 로딩맵 자체가 로드될 때도 이 델리게이트가 올 수 있으므로 **전환을 시작한 뒤에만** 닫는다.

```cpp
void USpyLoadingSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	//# 아직 전환을 시작하지 않았으면 이 콜백은 로딩맵 자신의 로드다 — 무시한다
	if (bTransitionStarted == false)
	{
		return;
	}

	if (USpyUIManager* UIManager = USpyUIManager::Get(GetGameInstance()))
	{
		UIManager->ClosePersistentSpyUI(ESpyUIType::Loading);
	}

	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}

	UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingSubsystem] 트래블 완료 — 로딩 UI 종료"));
}
```

- [ ] **Step 5: `Deinitialize` 에서 구독 해제**

기존 `Deinitialize` 를 아래로 교체한다.

```cpp
void USpyLoadingSubsystem::Deinitialize()
{
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}

	bLoading = false;
	OnProgressChanged.Clear();
	LoadingConfig = nullptr;

	Super::Deinitialize();
}
```

- [ ] **Step 6: 컴파일 검증 (사람 수행)**

Expected: 빌드 성공.

- [ ] **Step 7: Stage + 커밋 메시지(안)**

```bash
git add SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.h \
        SkillProject/Source/SkillProject/Manager/SpyLoadingSubsystem.cpp
```

```
[Refactor] SpyLoadingSubsystem — 로딩 UI 소유·수명을 서브시스템으로 이관
```

---

## Task 12: SpyLoadingGameMode — UI 오픈 제거

**Files:**
- Modify: `SkillProject/Source/SkillProject/System/SpyLoadingGameMode.cpp`

**Interfaces:**
- Consumes: `USpyLoadingSubsystem::StartLoading` (Task 11 이후 UI 오픈까지 포함)
- Produces: 없음

- [ ] **Step 1: `BeginPlay` 를 킥오프만 남기고 정리**

`SpyLoadingGameMode.cpp` 의 `BeginPlay` 전체를 아래로 교체한다. UI 오픈은 Task 11 에서 서브시스템이 가져갔다 — 여기 남겨두면 데디 서버가 뷰포트 없이 UI 를 여는 실측 문제 #3 이 재발한다.

```cpp
void ASpyLoadingGameMode::BeginPlay()
{
	Super::BeginPlay();

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance == nullptr)
	{
		return;
	}

	//# 데디케이티드 서버에서는 서브시스템이 생성되지 않는다 — null 이면 조용히 무시
	//# (로딩 UI 오픈도 서브시스템이 책임지므로 서버는 위젯을 아예 로드하지 않는다)
	if (USpyLoadingSubsystem* LoadingSubsystem = GameInstance->GetSubsystem<USpyLoadingSubsystem>())
	{
		LoadingSubsystem->StartLoading();
	}
}
```

- [ ] **Step 2: 불필요해진 include 제거**

`SpyLoadingGameMode.cpp` 의 include 블록에서 UI 관련 두 줄을 제거한다.

```cpp
#include "System/SpyLoadingGameMode.h"

#include "Engine/GameInstance.h"
#include "Manager/SpyLoadingSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyLoadingGameMode)
```

- [ ] **Step 3: 컴파일 검증 (사람 수행)**

Expected: 빌드 성공.

- [ ] **Step 4: Stage + 커밋 메시지(안)**

```bash
git add SkillProject/Source/SkillProject/System/SpyLoadingGameMode.cpp
```

```
[Fix] SpyLoadingGameMode — 데디 서버가 로딩 UI 를 여는 문제 제거
```

---

## Task 13: 룰 문서 갱신 + 통합 검증

**Files:**
- Modify: `.claude/rules/plugin-skuicore.md`

- [ ] **Step 1: `plugin-skuicore.md` §2 에 persistent UI 규칙 추가**

§2 의 API 목록 아래에 추가한다.

```markdown
- persistent(트래블 생존): `USKUIManager::OpenPersistentUI(FName, ZOrder)` / `ClosePersistentUI(FName)` / `IsPersistentUIOpen(FName)`
  - 로딩 화면처럼 **맵 전환 중에도 계속 보여야 하는 UI 전용.** 일반 UI 는 `OpenUI` 를 쓴다.
  - GameInstance 를 아우터로 생성하고 `UGameViewportClient::AddViewportWidgetContent` 로 얹으므로 월드가 파괴돼도 유지된다.
  - 위젯 클래스를 **동기 로드**한다 — 즉시 표시가 목적이기 때문. 큰 위젯에는 쓰지 않는다.
  - 뷰포트가 없는 환경(데디케이티드 서버)에서는 아무것도 하지 않고 `nullptr` 을 반환한다.
  - persistent UI 는 `OpenUIList` · 캐시 풀과 분리 관리된다 — `CloseLastUI` 로 닫히지 않는다.
```

- [ ] **Step 2: 기존 Automation 회귀 확인 (사람 수행)**

Session Frontend > Automation 에서 `SkillProject.Manager.Loading` 실행.
Expected: **20건 전부 PASS.** 이 플랜은 순수 함수·`ApplyConfig` 를 건드리지 않았으므로 하나라도 실패하면 의도치 않은 회귀다.

- [ ] **Step 3: PIE 수동 검증 (사람 수행)**

에디터 Play 설정을 **Net Mode: Play Offline, Number of Players 1, Run Dedicated Server 해제** 로 두고 PIE 실행.

Expected:
- 로딩바가 **즉시** 나타난다 (기존 1.23초 지연 소멸 — 실측 문제 #2)
- `LogTemp` 에 `# [SKUIManager] New Persistent UI: Loading` 이 `로딩 시작` 과 **같은 프레임**에 찍힌다
- 100% 도달 → 전환 시 **로딩 화면이 유지된 채** DevMap 이 뜨고, 그 다음 사라진다 (실측 문제 #1)
- `# [SpyLoadingSubsystem] 트래블 완료 — 로딩 UI 종료` 로그가 찍힌다
- 전환 후 로딩 위젯이 화면에 남아 있지 않다

- [ ] **Step 4: 데디케이티드 서버 검증 (사람 수행)**

Play 설정을 **Run Dedicated Server 체크** 로 두고 실행.

Expected:
- 서버 로그에 `No game viewport was found` 가 **더 이상 없다** (실측 문제 #3)
- 서버 로그에 `New Opening UI: Loading` 이 **없다**
- 서버가 `WBP_Loading_C` 를 로드하지 않는다

- [ ] **Step 5: 기존 UI 회귀 확인 (사람 수행)**

DevMap 에서 MainHUD·HpBar·Menu 가 평소대로 뜨는지 확인한다. Task 9 는 기존 `OpenUI` 경로를 건드리지 않았으므로 변화가 없어야 한다.

- [ ] **Step 6: Stage + 커밋 메시지(안)**

```bash
git add .claude/rules/plugin-skuicore.md
```

```
[Docs] plugin-skuicore — persistent UI 규칙 추가
```

---

## 범위 밖 (이 플랜에서 하지 않는 것)

- **MoviePlayer 도입** — T3 = 0.147초로 짧아 불필요. 게임 스레드가 완전히 블로킹되는 구간은 여전히 무보호지만, 측정상 무시할 수준이다.
- **Seamless Travel 전환** — 위와 같은 이유로 보류.
- **킥오프를 GameMode 에서 완전히 제거** — `ASpyLoadingGameMode::BeginPlay` 는 부팅 시 standalone(자기 자신이 서버)에서 정상 동작한다. 네트워크 플레이 중 로딩 화면이 필요해지는 시점(메뉴→서버 접속)에 다시 다룬다. 이 플랜은 UI 소유만 옮긴다.
- **로딩 화면 페이드·연출** — 기획서 §8-2, 별도 승인 필요.
