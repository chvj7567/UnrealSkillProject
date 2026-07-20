# SKUICore 플러그인 분리 Implementation Plan

> **For agentic workers:** Unreal C++ 리팩터링 — CLI 컴파일 없음. 검증은 사용자가 **에디터 닫고 VS 풀 빌드** + 런타임 open→close 왕복으로 수행([[skillproject-build-constraints]]). 태스크는 강결합(중간 컴파일 불가) → 순차 인라인 실행, 마지막에 컴파일.

**Goal:** `USpyUIManager`(UI 캐싱/재사용 서브시스템)와 `USKUserWidget`(위젯 베이스)을 프로젝트 비의존 플러그인 `SKUICore`로 분리. SkillProject엔 `USpyUIManager`(ESpyUIType 오버로드)·`USpyUserWidget` 얇은 서브클래스만.

**Architecture:** 스펙 `docs/superpowers/specs/2026-07-20-skuicore-plugin-design.md` 그대로. 캐싱/재사용 로직(OpenUIList/CashingUIList + 3단계 재사용 판정 + LRU 유사 캐싱)을 base `USKUIManager`에 **동작 보존** 이동. GameInstanceSubsystem 인스턴스 분리는 `ShouldCreateSubsystem` + 가드된 `Get()`로 방지.

**Tech Stack:** UE5.7, C++, UBT plugin, UMG/SlateCore, GameInstanceSubsystem.

**참조:** 스펙(위), SKAssetCore 선례(`Plugins/SKAssetCore/` — uplugin/Build.cs 형식), 원본 `SkillProject/Source/SkillProject/Manager/SpyUIManager.{h,cpp}`, `UI/SKUserWidget.{h,cpp}`.

## Global Constraints

- **커밋:** 태스크마다 `git add` + `[Tag] ClassName — 요약` 커밋 (git-conventions; 이 실행 커밋 허용).
- **동작 보존:** UI 오픈/캐싱/재사용/닫기 동작을 **한 줄도 바꾸지 않는다.** 타입 일반화(`USpyUserWidget`→`USKUserWidget`, `USpyAssetManager`→`USKAssetManager`)만.
- **주석 `//#`** (cpp-style). `!` 대신 명시 비교.
- **컴파일 검증은 사용자** (에디터 닫고 VS 풀 빌드). 에이전트는 자체 "컴파일 OK" 단정 금지.
- **API 매크로:** 플러그인 클래스는 `SKUICORE_API`.
- **이넘 불이동:** `ESpyUIType`(DefineEnum.h)는 SkillProject 잔류, 플러그인 경계 안 넘음.

---

## File Structure

**신규 (플러그인):**
- `SkillProject/Plugins/SKUICore/SKUICore.uplugin`
- `.../Source/SKUICore/SKUICore.Build.cs`
- `.../Source/SKUICore/Private/SKUICoreModule.cpp`
- `.../Source/SKUICore/Public/SKUserWidget.h` · `Private/SKUserWidget.cpp` (이동)
- `.../Source/SKUICore/Public/SKUIManager.h` · `Private/SKUIManager.cpp` (이동+일반화)

**수정 (SkillProject):**
- `Source/SkillProject/Manager/SpyUIManager.h` / `.cpp` → 얇은 서브클래스
- `Source/SkillProject/UI/SpyUserWidget.h` (include 스왑)
- `Source/SkillProject/UI/SpyHPBar.h` (include 스왑)
- `Source/SkillProject/SkillProject.Build.cs` (+SKUICore)
- `SkillProject.uproject` (Plugins + AdditionalDependencies)

**삭제:** `Source/SkillProject/UI/SKUserWidget.h` / `.cpp` (플러그인으로 이동)

---

## Task 1: 플러그인 스캐폴딩

**Files:**
- Create: `SkillProject/Plugins/SKUICore/SKUICore.uplugin`
- Create: `SkillProject/Plugins/SKUICore/Source/SKUICore/SKUICore.Build.cs`
- Create: `SkillProject/Plugins/SKUICore/Source/SKUICore/Private/SKUICoreModule.cpp`

- [ ] **Step 1: `SKUICore.uplugin`**
```json
{
	"FileVersion": 3,
	"Version": 1,
	"VersionName": "1.0",
	"FriendlyName": "SK UI Core",
	"Description": "Project-agnostic UI manager (open/cache/reuse) + widget base. Reusable across projects.",
	"Category": "Gameplay",
	"CreatedBy": "",
	"CreatedByURL": "",
	"EnabledByDefault": true,
	"CanContainContent": false,
	"IsBetaVersion": false,
	"Installed": false,
	"Modules": [
		{
			"Name": "SKUICore",
			"Type": "Runtime",
			"LoadingPhase": "Default"
		}
	],
	"Plugins": [
		{
			"Name": "SKAssetCore",
			"Enabled": true
		}
	]
}
```

- [ ] **Step 2: `SKUICore.Build.cs`**
```csharp
// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SKUICore : ModuleRules
{
	public SKUICore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"UMG",
				"SlateCore",
				"SKAssetCore",
			});
	}
}
```

- [ ] **Step 3: `Private/SKUICoreModule.cpp`**
```cpp
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, SKUICore);
```

- [ ] **Step 4: 스테이징 + 커밋**
```
git add SkillProject/Plugins/SKUICore/SKUICore.uplugin SkillProject/Plugins/SKUICore/Source/SKUICore/SKUICore.Build.cs SkillProject/Plugins/SKUICore/Source/SKUICore/Private/SKUICoreModule.cpp
git commit -m "[Chore] SKUICore — 플러그인 스캐폴딩(uplugin/Build.cs/모듈) 신규"
```

---

## Task 2: `USKUserWidget` 를 플러그인으로 이동

원본 `Source/SkillProject/UI/SKUserWidget.{h,cpp}`. 변경: `SKILLPROJECT_API`→`SKUICORE_API`, cpp의 매니저 역참조를 `USKUIManager`로.

**Files:**
- Create: `SkillProject/Plugins/SKUICore/Source/SKUICore/Public/SKUserWidget.h`
- Create: `SkillProject/Plugins/SKUICore/Source/SKUICore/Private/SKUserWidget.cpp`

- [ ] **Step 1: `Public/SKUserWidget.h`**
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "SKUserWidget.generated.h"

UCLASS()
class SKUICORE_API USKUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	USKUserWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void OnWidgetRebuilt() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnTouchGesture(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

public:
	UFUNCTION(BlueprintCallable)
	void SetConsumePointerInput(bool bInConsumePointerInput);

	UFUNCTION(BlueprintCallable)
	FName GetUIName();

	UFUNCTION(BlueprintCallable)
	void SetUIName(FName InUIName);

	UFUNCTION(BlueprintCallable)
	virtual void Close();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	bool bConsumePointerInput = false;

	UPROPERTY()
	FName UIName;
};
```

- [ ] **Step 2: `Private/SKUserWidget.cpp`** (원본과 동일, include·Close() 변경)
```cpp
#include "SKUserWidget.h"
#include "SKUIManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKUserWidget)

USKUserWidget::USKUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void USKUserWidget::OnWidgetRebuilt()
{
	Super::OnWidgetRebuilt();
}

void USKUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void USKUserWidget::NativeDestruct()
{
	Super::NativeDestruct();
}

FReply USKUserWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return bConsumePointerInput ? FReply::Handled() : Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply USKUserWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return bConsumePointerInput ? FReply::Handled() : Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply USKUserWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return bConsumePointerInput ? FReply::Handled() : Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

FReply USKUserWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return bConsumePointerInput ? FReply::Handled() : Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

FReply USKUserWidget::NativeOnTouchGesture(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	return bConsumePointerInput ? FReply::Handled() : Super::NativeOnTouchGesture(InGeometry, InGestureEvent);
}

FReply USKUserWidget::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	return bConsumePointerInput ? FReply::Handled() : Super::NativeOnTouchStarted(InGeometry, InGestureEvent);
}

FReply USKUserWidget::NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	return bConsumePointerInput ? FReply::Handled() : Super::NativeOnTouchMoved(InGeometry, InGestureEvent);
}

FReply USKUserWidget::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	return bConsumePointerInput ? FReply::Handled() : Super::NativeOnTouchEnded(InGeometry, InGestureEvent);
}

void USKUserWidget::SetConsumePointerInput(bool bInConsumePointerInput)
{
	bConsumePointerInput = bInConsumePointerInput;
}

FName USKUserWidget::GetUIName()
{
	return UIName;
}

void USKUserWidget::SetUIName(FName InUIName)
{
	UIName = InUIName;
}

void USKUserWidget::Close()
{
	//# 위젯 → 매니저 역호출 (플러그인 base)
	if (USKUIManager* UIMgr = USKUIManager::Get(this))
	{
		UIMgr->CloseUI(UIName);
	}
}
```
> 원본은 `USpyUIManager::Get(this)->CloseUI(UIName)` (null 체크 없음). 플러그인화하며 `USKUIManager::Get` + null 가드 추가(동작 동일, 안전).

- [ ] **Step 3: 스테이징 + 커밋** (Task 3와 한 쌍 — 아직 컴파일 불가)
```
git add SkillProject/Plugins/SKUICore/Source/SKUICore/Public/SKUserWidget.h SkillProject/Plugins/SKUICore/Source/SKUICore/Private/SKUserWidget.cpp
git commit -m "[Chore] SKUserWidget — 플러그인으로 이동(SKUICORE_API, Close()→USKUIManager)"
```

---

## Task 3: `USKUIManager` 이동+일반화 (캐싱/재사용 엔진 — 핵심)

원본 `SpyUIManager`의 FName 로직 전체를 base로. `USpyUserWidget`→`USKUserWidget`, `USpyAssetManager`→`USKAssetManager`. `ShouldCreateSubsystem`/가드된 `Get()` 추가. **캐싱/재사용 동작 불변.**

**Files:**
- Create: `SkillProject/Plugins/SKUICore/Source/SKUICore/Public/SKUIManager.h`
- Create: `SkillProject/Plugins/SKUICore/Source/SKUICore/Private/SKUIManager.cpp`

- [ ] **Step 1: `Public/SKUIManager.h`**
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Components/WidgetComponent.h"

#include "SKUIManager.generated.h"

class UWidgetComponent;
class USKUserWidget;

UCLASS()
class SKUICORE_API USKUIManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//# 파생 서브클래스(예: USpyUIManager)가 있으면 base 는 생성하지 않음 → leaf 인스턴스 1개만
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	//# base·leaf 어디서든 동일한 leaf 인스턴스를 반환
	static USKUIManager* Get(const UObject* WorldContextObject);

public:
	UFUNCTION(BlueprintCallable)
	void OpenUI(FName InUIName);

	UFUNCTION(BlueprintCallable)
	void CloseUI(FName InUIName);

	UFUNCTION(BlueprintCallable)
	void CloseLastUI();

	UFUNCTION(BlueprintCallable)
	void OpenSubUI(FName InUIName, UWidgetComponent* WidgetComponent, EWidgetSpace Space);

	UFUNCTION(BlueprintCallable)
	void AddCashingUI(USKUserWidget* UserWidget);

protected:
	const int MaxCashingUICount = 5;

protected:
	UPROPERTY()
	TArray<TObjectPtr<USKUserWidget>> OpenUIList;

	UPROPERTY()
	TArray<TObjectPtr<USKUserWidget>> CashingUIList;
};
```

- [ ] **Step 2: `Private/SKUIManager.cpp`** (원본 로직 전부, 타입 일반화 + 서브시스템 가드)
```cpp
#include "SKUIManager.h"
#include "SKUserWidget.h"
#include "SKAssetManager.h"
#include "SKAssetData.h"
#include "Blueprint/UserWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKUIManager)

bool USKUIManager::ShouldCreateSubsystem(UObject* Outer) const
{
	//# 파생 클래스가 있으면 base 는 생성 안 함 (leaf 만 생성 → 인스턴스 분리 방지)
	TArray<UClass*> DerivedClasses;
	GetDerivedClasses(GetClass(), DerivedClasses, false);
	return DerivedClasses.Num() == 0;
}

void USKUIManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void USKUIManager::Deinitialize()
{
	Super::Deinitialize();
}

USKUIManager* USKUIManager::Get(const UObject* WorldContextObject)
{
	if (WorldContextObject == nullptr)
		return nullptr;

	if (const UWorld* World = WorldContextObject->GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			//# 파생 포함 인스턴스 조회 — leaf(USpyUIManager) 하나가 잡힌다
			const TArray<USKUIManager*>& Subsystems = GI->GetSubsystemArray<USKUIManager>();
			checkf(Subsystems.Num() <= 1, TEXT("USKUIManager 인스턴스가 2개 이상 — ShouldCreateSubsystem 확인"));
			return Subsystems.Num() > 0 ? Subsystems[0] : nullptr;
		}
	}

	return nullptr;
}

void USKUIManager::OpenUI(FName InUIName)
{
	//# 패키지 빌드에서 BP 오브젝트(BP_X.BP_X)는 cook 시 stripped되므로 generated class(BP_X.BP_X_C) 경로로 로드
	const USKAssetData& AssetData = USKAssetManager::Get().GetAssetData();
	const FSoftObjectPath& AssetPath = AssetData.GetAssetPathByName(InUIName);
	FString ClassPathString = AssetPath.GetAssetPathString();
	if (ClassPathString.EndsWith(TEXT("_C")) == false)
	{
		ClassPathString.Append(TEXT("_C"));
	}
	FSoftObjectPath ClassPath(ClassPathString);

	FSKAssetAndDelegate LoadDelegate;
	LoadDelegate.BindLambda([this, InUIName](UObject* LoadedAsset)
		{
			if (LoadedAsset == nullptr)
				return;

			if (TSubclassOf<USKUserWidget> UI = USKAssetManager::GetSubclassByName<USKUserWidget>(InUIName))
			{
				//# 이미 열려있는 UI 확인
				const TObjectPtr<USKUserWidget>* FindOpenningUI = OpenUIList.FindByPredicate(
					[InUIName](TObjectPtr<USKUserWidget>& UserWidget)
					{
						return UserWidget->GetUIName() == InUIName;
					});

				if (FindOpenningUI)
				{
					//# 동일한 UI는 중복해서 띄우지 않음
					UE_LOG(LogTemp, Warning, TEXT("Already Opening UI: %s"), *InUIName.ToString());
					return;
				}

				//# 캐싱 중인 UI 확인
				const TObjectPtr<USKUserWidget>* FindCashingUI = CashingUIList.FindByPredicate(
					[InUIName](TObjectPtr<USKUserWidget>& UserWidget)
					{
						return UserWidget->GetUIName() == InUIName;
					});

				if (FindCashingUI)
				{
					//# 캐싱 중인 UI이면 Open
					OpenUIList.Add(FindCashingUI->Get());
					FindCashingUI->Get()->AddToViewport();

					UE_LOG(LogTemp, Warning, TEXT("Cashing Opening UI: %s"), *InUIName.ToString());
					return;
				}

				//# UI 생성
				if (USKUserWidget* UserWidget = CreateWidget<USKUserWidget>(GetWorld(), UI))
				{
					UserWidget->SetUIName(InUIName);
					OpenUIList.Add(UserWidget);

					UserWidget->AddToViewport();

					UE_LOG(LogTemp, Warning, TEXT("New Opening UI: %s"), *InUIName.ToString());
				}
			}
		});

	USKAssetManager::LoadAssetAsync(ClassPath, LoadDelegate);
}

void USKUIManager::CloseUI(FName InUIName)
{
	UE_LOG(LogTemp, Warning, TEXT("CloseUI: %s"), *InUIName.ToString());

	if (OpenUIList.IsEmpty())
		return;

	//# 이미 열려있는 UI 확인
	const TObjectPtr<USKUserWidget>* FindOpenningUI = OpenUIList.FindByPredicate(
		[InUIName](TObjectPtr<USKUserWidget>& UserWidget)
		{
			return UserWidget->GetUIName() == InUIName;
		});

	if (FindOpenningUI)
	{
		OpenUIList.Remove(FindOpenningUI->Get());
		AddCashingUI(FindOpenningUI->Get());
		FindOpenningUI->Get()->RemoveFromParent();
	}
}

void USKUIManager::CloseLastUI()
{
	if (OpenUIList.IsEmpty())
		return;

	if (USKUserWidget* UserWidget = OpenUIList.Last())
	{
		OpenUIList.Pop();
		CloseUI(UserWidget->GetUIName());
	}
}

void USKUIManager::OpenSubUI(FName InUIName, UWidgetComponent* WidgetComponent, EWidgetSpace Space)
{
	const USKAssetData& AssetData = USKAssetManager::Get().GetAssetData();
	const FSoftObjectPath& AssetPath = AssetData.GetAssetPathByName(InUIName);
	FString ClassPathString = AssetPath.GetAssetPathString();
	if (ClassPathString.EndsWith(TEXT("_C")) == false)
	{
		ClassPathString.Append(TEXT("_C"));
	}
	FSoftObjectPath ClassPath(ClassPathString);

	if (WidgetComponent == nullptr)
		return;

	FSKAssetAndDelegate LoadDelegate;
	TWeakObjectPtr<UWidgetComponent> WeakWidget = WidgetComponent;
	LoadDelegate.BindLambda([InUIName, WeakWidget, Space](UObject* LoadedAsset)
		{
			if (LoadedAsset == nullptr || WeakWidget.IsValid() == false)
				return;

			if (TSubclassOf<USKUserWidget> UI = USKAssetManager::GetSubclassByName<USKUserWidget>(InUIName))
			{
				WeakWidget->SetWidgetClass(UI);
				WeakWidget->SetWidgetSpace(Space);
				WeakWidget->InitWidget();
				WeakWidget->SetVisibility(true);
			}
		});

	USKAssetManager::LoadAssetAsync(ClassPath, LoadDelegate);
}

void USKUIManager::AddCashingUI(USKUserWidget* UserWidget)
{
	//# 캐싱 수가 Max라면 오래된 UI 제거 후 추가
	if (MaxCashingUICount > 0)
	{
		for (USKUserWidget* CashingUserWIdget : CashingUIList)
		{
			if (CashingUserWIdget->GetUIName() == UserWidget->GetUIName())
				return;
		}

		CashingUIList.Add(UserWidget);

		if (CashingUIList.Num() >= MaxCashingUICount)
		{
			CashingUIList.RemoveAt(0);
		}
	}
}
```

- [ ] **Step 2b: 캐싱/재사용 로직 보존 확인 (육안)**
원본 `SpyUIManager.cpp`(OpenUI 재사용 3단계, CloseUI→AddCashingUI, AddCashingUI LRU)와 위 코드가 **타입명만 다르고 제어 흐름 동일**한지 대조. 로그 문자열("Already Opening UI"/"Cashing Opening UI"/"New Opening UI") 그대로.

- [ ] **Step 3: 스테이징 + 커밋**
```
git add SkillProject/Plugins/SKUICore/Source/SKUICore/Public/SKUIManager.h SkillProject/Plugins/SKUICore/Source/SKUICore/Private/SKUIManager.cpp
git commit -m "[Chore] SKUIManager — UI 캐싱/재사용 엔진 플러그인 이동+일반화 + 서브시스템 가드"
```

---

## Task 4: SkillProject `USpyUIManager` 얇은 서브클래스로 재작성

**Files:**
- Modify: `Source/SkillProject/Manager/SpyUIManager.h` (전체 교체)
- Modify: `Source/SkillProject/Manager/SpyUIManager.cpp` (전체 교체)

- [ ] **Step 1: `SpyUIManager.h` 전체 교체**
```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SKUIManager.h"
#include "Util/DefineEnum.h"

#include "SpyUIManager.generated.h"

class UWidgetComponent;

UCLASS()
class SKILLPROJECT_API USpyUIManager : public USKUIManager
{
	GENERATED_BODY()

public:
	//# leaf 서브시스템 인스턴스 접근 (호출부가 쓰는 형태)
	static USpyUIManager* Get(const UObject* WorldContextObject);

public:
	UFUNCTION(BlueprintCallable)
	void OpenSpyUI(ESpyUIType UIType);

	UFUNCTION(BlueprintCallable)
	void CloseSpyUI(ESpyUIType UIType);

	UFUNCTION(BlueprintCallable)
	void OpenSubSpyUI(ESpyUIType UIType, UWidgetComponent* WidgetComponent, EWidgetSpace Space);
};
```

- [ ] **Step 2: `SpyUIManager.cpp` 전체 교체**
```cpp
// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/SpyUIManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyUIManager)

USpyUIManager* USpyUIManager::Get(const UObject* WorldContextObject)
{
	if (WorldContextObject == nullptr)
		return nullptr;

	if (const UWorld* World = WorldContextObject->GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			return GI->GetSubsystem<USpyUIManager>();
		}
	}

	return nullptr;
}

void USpyUIManager::OpenSpyUI(ESpyUIType UIType)
{
	FString EnumName = StaticEnum<ESpyUIType>()->GetNameStringByValue((int64)UIType);
	OpenUI(FName(*EnumName));
}

void USpyUIManager::CloseSpyUI(ESpyUIType UIType)
{
	FString EnumName = StaticEnum<ESpyUIType>()->GetNameStringByValue((int64)UIType);
	CloseUI(FName(*EnumName));
}

void USpyUIManager::OpenSubSpyUI(ESpyUIType UIType, UWidgetComponent* WidgetComponent, EWidgetSpace Space)
{
	FString EnumName = StaticEnum<ESpyUIType>()->GetNameStringByValue((int64)UIType);
	OpenSubUI(FName(*EnumName), WidgetComponent, Space);
}
```

- [ ] **Step 3: 스테이징 + 커밋**
```
git add SkillProject/Source/SkillProject/Manager/SpyUIManager.h SkillProject/Source/SkillProject/Manager/SpyUIManager.cpp
git commit -m "[Refactor] SpyUIManager — USKUIManager 얇은 서브클래스로 전환(ESpyUIType 오버로드만)"
```

---

## Task 5: include 경로 스왑 (프로젝트 위젯 2곳)

`USKUserWidget`이 플러그인으로 이동했으므로, 이를 상속하는 프로젝트 위젯의 include 경로만 스왑. 코드 변경 없음.

- [ ] **Step 1: `UI/SpyUserWidget.h`** — `#include "UI/SKUserWidget.h"` → `#include "SKUserWidget.h"`
- [ ] **Step 2: `UI/SpyHPBar.h`** — `#include "UI/SKUserWidget.h"` → `#include "SKUserWidget.h"`
- [ ] **Step 3: 잔존 참조 확인**
Run: `rg -n "UI/SKUserWidget.h" SkillProject/Source`
Expected: 매치 없음 (모두 플러그인 `SKUserWidget.h` 로 이전).
- [ ] **Step 4: 스테이징 + 커밋**
```
git add SkillProject/Source/SkillProject/UI/SpyUserWidget.h SkillProject/Source/SkillProject/UI/SpyHPBar.h
git commit -m "[Refactor] Spy UI 위젯 — SKUserWidget include 플러그인 경로로 스왑"
```

---

## Task 6: 빌드 배선 (Build.cs, uproject)

- [ ] **Step 1: `SkillProject.Build.cs`** — `PublicDependencyModuleNames`에 `"SKUICore"` 추가 (예: `"SKAssetCore",` 다음)
- [ ] **Step 2: `SkillProject.uproject`**
  - `Plugins`에 추가:
    ```json
    {
        "Name": "SKUICore",
        "Enabled": true
    }
    ```
  - `Modules[0]`(SkillProject) `AdditionalDependencies`에 `"SKUICore"` 추가
- [ ] **Step 3: uproject JSON 유효성 확인** — `python -c "import json;json.load(open('SkillProject/SkillProject.uproject'))"` 또는 node
- [ ] **Step 4: 스테이징 + 커밋**
```
git add SkillProject/Source/SkillProject/SkillProject.Build.cs SkillProject/SkillProject.uproject
git commit -m "[Chore] Build — SKUICore 의존성 추가(SkillProject/uproject)"
```

---

## Task 7: 구 파일 삭제

- [ ] **Step 1: 이동된 원본 삭제**
```
git rm SkillProject/Source/SkillProject/UI/SKUserWidget.h SkillProject/Source/SkillProject/UI/SKUserWidget.cpp
```
- [ ] **Step 2: 잔존 참조 없음 확인**
Run: `rg -n "UI/SKUserWidget.h|Manager/SpyUIManager.h" SkillProject/Source` — `Manager/SpyUIManager.h`는 호출부에 남아있어야 정상(USpyUIManager 유지), `UI/SKUserWidget.h`는 0.
- [ ] **Step 3: 커밋**
```
git commit -m "[Chore] SKUserWidget — 구 SkillProject 위치 파일 삭제(플러그인 이동 완료)"
```

---

## Task 8: 빌드 검증 (사용자) + 수정

> **사용자가 에디터 닫고 VS 풀 빌드**로 수행. 에이전트는 에러 회신 받아 수정. Live Coding 금지(구조 변경 + 메모리 — [[skillproject-build-constraints]]).

- [ ] **Step 1: 프로젝트 파일 재생성** — `SkillProject/Launch.bat` 또는 uproject 우클릭 → Generate VS project files (신규 플러그인 모듈 인식).
- [ ] **Step 2: 컴파일** — 에디터 닫고 VS 풀 빌드. 에러 시 파일:줄 회신 → 수정 후 재빌드. 예상 리스크:
  - include 경로(플러그인 Public 자동 노출) → `SKUICore.Build.cs` 확인.
  - UMG/SlateCore 심볼 unresolved(`FReply`/`CreateWidget`/`WidgetComponent`) → Build.cs 의존 확인.
  - `USKUserWidget` 미정의(프로젝트 위젯) → include 스왑 누락 확인.
- [ ] **Step 3: 서브시스템 단일 인스턴스** — 실행 시 `Get()`의 `checkf(Num<=1)` 미발동. (2개면 `ShouldCreateSubsystem` 점검.)
- [ ] **Step 4: 캐싱/재사용 왕복 검증 (핵심)** — 게임에서:
  - UI 열기 → 닫기 → **다시 열기** → 두 번째 오픈 로그가 **"Cashing Opening UI"**(재생성 아님)인지 확인.
  - 같은 UI 두 번 연속 열기 → **"Already Opening UI"**(중복 무시) 확인.
  - `USKUserWidget::Close()`(위젯 자체 닫기) 경로로 닫아도 정상 닫힘 — 인스턴스 분리 없음 직접 검증.
  - OpenSpyUI/OpenSubSpyUI(MainHUD/Menu/HpBar) 기존과 동일 동작.
- [ ] **Step 5: 통과 후 마무리** — 인프라 문서(`.claude/rules/unreal-infra.md` §3 DataAsset·모듈, `project.md` infrastructure.modules)에 SKUICore 추가 반영 여부를 사용자에게 제안.

---

## Self-Review (작성자 점검)

**Spec 커버리지:** §2 플러그인 구조→T1; §3.1 USKUserWidget→T2; §3.2 USKUIManager+캐싱/재사용+서브시스템 가드→T3; §4 얇은 서브클래스→T4; §5.1 include 스왑→T5(+T2 back-ref); §5.3 Build→T6; §5.4 순환 제거→T2/T3; §6 인스턴스 분리 방지→T3 ShouldCreateSubsystem/Get; §7 검증→T8. ✅

**Placeholder:** 없음. 신규/이동 파일 전체 내용 포함, include·호출부는 파일:줄 명시.

**타입/이름 일관성:** `USKUIManager`·`USKUserWidget`·`SKUICORE_API`·`ShouldCreateSubsystem`·`GetSubsystemArray` 전 태스크 일관. 캐싱 리스트 타입 `TArray<USKUserWidget>` 일관. `ESpyUIType`는 서브클래스에만.

**강결합:** T2~T7 중간 컴파일 불가 — 전부 적용 후 T8 첫 빌드. 순차 인라인 실행.
