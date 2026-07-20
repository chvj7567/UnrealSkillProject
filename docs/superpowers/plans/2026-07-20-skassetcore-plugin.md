# SKAssetCore 플러그인 분리 Implementation Plan

> **For agentic workers:** 이 플랜은 Unreal C++ 리팩터링이라 **CLI 컴파일이 없다**. 검증은 사용자가 에디터/VS에서 빌드해 결과를 알려주는 방식으로 이뤄진다. 태스크들은 서로 강결합이라(중간 컴파일 불가) 한 번에 이동 후 마지막에 컴파일한다 — subagent 독립 실행보다 순차 인라인 실행에 적합.

**Goal:** `USKAssetManager`(로딩 로직)와 `USKAssetData`(이름 룩업)를 프로젝트 비의존 런타임 플러그인 `SKAssetCore`로 분리하고, SkillProject엔 얇은 서브클래스(`USpyAssetManager`/`USpyAssetData`)만 남긴다.

**Architecture:** 스펙 `docs/superpowers/specs/2026-07-19-skassetcore-plugin-design.md` 를 그대로 구현. 서브클래스 유지로 ini·call-site 변경을 최소화. base에 `OnLoadProgress` 신규 virtual 훅 추가.

**Tech Stack:** Unreal Engine 5.7, C++, UBT plugin/module, DefaultGame.ini.

**참조:** 스펙 `docs/superpowers/specs/2026-07-19-skassetcore-plugin-design.md` (SoT). 템플릿: `SkillProject/Plugins/ModularGameplayActors/` (uplugin 형식), `SkillProject/Source/SKGAS/SKGAS.Build.cs`.

## Global Constraints

- **커밋:** 각 태스크 완료 시 관련 파일 `git add` + `[Tag] ClassName — 요약` 커밋 메시지. (git-conventions.md; 이 실행은 커밋 허용)
- **동작 보존:** 기존 로딩 동작·성능을 바꾸지 않는다. 순수 이동 + 일반화.
- **주석:** `//#` (cpp-style.md). `!` 부정 대신 명시 비교.
- **컴파일 검증은 사용자:** 에이전트는 코드 작성·정적 확인까지. "컴파일 OK" 를 자체 단정하지 않는다. 최종 빌드는 사용자가 에디터/VS에서 수행하고 에러를 회신.
- **ini 무변경(가정) + fallback:** `AssetDataPath` 는 base로 이동하되 ini 섹션 `[/Script/SkillProject.SpyAssetManager]` 무변경. 런타임에 null resolve되면 Task 8의 fallback 적용.
- **API 매크로:** 플러그인 클래스는 `SKASSETCORE_API`.

---

## File Structure

**신규 (플러그인):**
- `SkillProject/Plugins/SKAssetCore/SKAssetCore.uplugin`
- `SkillProject/Plugins/SKAssetCore/Source/SKAssetCore/SKAssetCore.Build.cs`
- `.../Source/SKAssetCore/Private/SKAssetCoreModule.cpp` (IMPLEMENT_MODULE)
- `.../Source/SKAssetCore/Public/SKAssetData.h`
- `.../Source/SKAssetCore/Private/SKAssetData.cpp`
- `.../Source/SKAssetCore/Public/SKAssetManager.h`
- `.../Source/SKAssetCore/Private/SKAssetManager.cpp`

**수정 (SkillProject):**
- `Source/SkillProject/Manager/SpyAssetManager.h` / `.cpp` → 얇은 서브클래스로 재작성
- `Source/SkillProject/Data/SpyAssetData.h` → include 경로 변경
- `Source/SkillProject/Character/SpyCharacter.cpp` (델리게이트 rename + GetAssetData 타입)
- `Source/SkillProject/Character/AnimInstance/SpyCharacterAnimInstance.cpp` (델리게이트 rename)
- `Source/SkillProject/Manager/SpyUIManager.cpp` (델리게이트 rename ×2 + GetAssetData 타입 ×2 + include 경로)
- `Source/SkillProject/SkillProject.Build.cs` (+SKAssetCore)
- `Source/SpyDataEditorTool/SpyDataEditorTool.Build.cs` (+SKAssetCore)
- `SkillProject.uproject` (Plugins + SkillProject AdditionalDependencies)

**삭제:**
- `Source/SkillProject/Data/SKAssetData.h` / `.cpp` (플러그인으로 이동)

---

## Task 1: 플러그인 스캐폴딩

**Files:**
- Create: `SkillProject/Plugins/SKAssetCore/SKAssetCore.uplugin`
- Create: `SkillProject/Plugins/SKAssetCore/Source/SKAssetCore/SKAssetCore.Build.cs`
- Create: `SkillProject/Plugins/SKAssetCore/Source/SKAssetCore/Private/SKAssetCoreModule.cpp`

- [ ] **Step 1: `SKAssetCore.uplugin` 작성**

```json
{
	"FileVersion": 3,
	"Version": 1,
	"VersionName": "1.0",
	"FriendlyName": "SK Asset Core",
	"Description": "Project-agnostic AssetManager + name-to-path asset data lookup. Reusable across projects.",
	"Category": "Gameplay",
	"CreatedBy": "",
	"CreatedByURL": "",
	"EnabledByDefault": true,
	"CanContainContent": false,
	"IsBetaVersion": false,
	"Installed": false,
	"Modules": [
		{
			"Name": "SKAssetCore",
			"Type": "Runtime",
			"LoadingPhase": "Default"
		}
	]
}
```

- [ ] **Step 2: `SKAssetCore.Build.cs` 작성** (SKGAS.Build.cs 패턴)

```csharp
// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SKAssetCore : ModuleRules
{
	public SKAssetCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
			});
	}
}
```

> `Misc/DataValidation.h`(WITH_EDITOR)는 Core/CoreUObject/Engine로 해결됨 (기존 SkillProject가 DataValidation 모듈 없이 컴파일 중 — 스펙 §2). 빌드 시 unresolved면 Task 8에서 `bBuildEditor` 조건부 `"DataValidation"` 추가.

- [ ] **Step 3: 모듈 구현 파일 `Private/SKAssetCoreModule.cpp`**

```cpp
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, SKAssetCore);
```

- [ ] **Step 4: 스테이징 + 커밋**

```
git add SkillProject/Plugins/SKAssetCore/SKAssetCore.uplugin SkillProject/Plugins/SKAssetCore/Source/SKAssetCore/SKAssetCore.Build.cs SkillProject/Plugins/SKAssetCore/Source/SKAssetCore/Private/SKAssetCoreModule.cpp
git commit -m "[Chore] SKAssetCore — 플러그인 스캐폴딩(uplugin/Build.cs/모듈) 신규"
```

---

## Task 2: `USKAssetData` 를 플러그인으로 이동

원본: `Source/SkillProject/Data/SKAssetData.{h,cpp}`. 변경: API 매크로 `SKILLPROJECT_API`→`SKASSETCORE_API`, cpp의 `Get()` 역참조를 `USKAssetManager`로.

**Files:**
- Create: `SkillProject/Plugins/SKAssetCore/Source/SKAssetCore/Public/SKAssetData.h`
- Create: `SkillProject/Plugins/SKAssetCore/Source/SKAssetCore/Private/SKAssetData.cpp`

- [ ] **Step 1: `Public/SKAssetData.h` 작성**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "SKAssetData.generated.h"

USTRUCT()
struct FAssetEntry
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	FName AssetName;

	UPROPERTY(EditDefaultsOnly)
	FSoftObjectPath AssetPath;
};

USTRUCT()
struct FAssetSet
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	TArray<FAssetEntry> AssetEntries;
};

UCLASS(Const, CollapseCategories, meta=(DisplayName="SK Asset Data"))
class SKASSETCORE_API USKAssetData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const USKAssetData& Get();

protected:
#if WITH_EDITOR
	virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

public:
	FSoftObjectPath GetAssetPathByName(const FName& AssetName) const;

private:
	UPROPERTY(EditDefaultsOnly)
	TMap<FName, FAssetSet> AssetGroupNameToSet;

	UPROPERTY()
	TMap<FName, FSoftObjectPath> AssetNameToPath;
};
```

- [ ] **Step 2: `Private/SKAssetData.cpp` 작성** (원본과 동일, include·`Get()` 변경)

```cpp
#include "SKAssetData.h"
#include "SKAssetManager.h"
#include "UObject/ObjectSaveContext.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKAssetData)

const USKAssetData& USKAssetData::Get()
{
	return USKAssetManager::Get().GetAssetData();
}

#if WITH_EDITOR
void USKAssetData::PreSave(FObjectPreSaveContext ObjectSaveContext)
{
	Super::PreSave(ObjectSaveContext);

	AssetNameToPath.Empty();

	AssetGroupNameToSet.KeySort([](const FName& A, const FName& B)
	{
		return (A.Compare(B) < 0);
	});

	for (const auto& Pair : AssetGroupNameToSet)
	{
		const FAssetSet& AssetSet = Pair.Value;
		for (FAssetEntry AssetEntry : AssetSet.AssetEntries)
		{
			AssetNameToPath.Emplace(AssetEntry.AssetName, AssetEntry.AssetPath);
		}
	}
}

EDataValidationResult USKAssetData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	for (const auto& Pair : AssetGroupNameToSet)
	{
		const FAssetSet& AssetSet = Pair.Value;
		for (int32 i = 0; i < AssetSet.AssetEntries.Num(); i++)
		{
			const FAssetEntry& AssetEntry = AssetSet.AssetEntries[i];
			if (AssetEntry.AssetName.IsNone())
			{
				Context.AddError(FText::FromString(FString::Printf(TEXT("Asset Name is None : [Group Name : %s] - [Entry Index : %d]"), *Pair.Key.ToString(), i)));
				Result = EDataValidationResult::Invalid;
			}

			if (AssetEntry.AssetPath.IsValid() == false)
			{
				Context.AddError(FText::FromString(FString::Printf(TEXT("Asset Path is Invalid : [Group Name : %s] - [Entry Index : %d]"), *Pair.Key.ToString(), i)));
				Result = EDataValidationResult::Invalid;
			}
		}
	}

	return Result;
}
#endif

FSoftObjectPath USKAssetData::GetAssetPathByName(const FName& AssetName) const
{
	const FSoftObjectPath* AssetPath = AssetNameToPath.Find(AssetName);
	ensureAlwaysMsgf(AssetPath, TEXT("Can't find Asset Path from Asset Name [%s]."), *AssetName.ToString());
	return *AssetPath;
}
```

- [ ] **Step 3: 스테이징 + 커밋** (아직 컴파일 불가 — Task 3와 한 쌍)

```
git add SkillProject/Plugins/SKAssetCore/Source/SKAssetCore/Public/SKAssetData.h SkillProject/Plugins/SKAssetCore/Source/SKAssetCore/Private/SKAssetData.cpp
git commit -m "[Chore] SKAssetData — 플러그인으로 이동(SKASSETCORE_API, Get()→USKAssetManager)"
```

---

## Task 3: `USKAssetManager` 를 플러그인으로 이동 (로직 전체 + 일반화)

원본: `Source/SkillProject/Manager/SpyAssetManager.{h,cpp}` 의 로직 전부를 base `USKAssetManager` 로. 변경: 클래스명 `USpyAssetManager`→`USKAssetManager`, 델리게이트 `FSpyAssetAndDelegate`→`FSKAssetAndDelegate`, `GetAssetData()` 반환 `const USpyAssetData&`→`const USKAssetData&`, `OnLoadProgress` 신규 virtual + `LogProgress`에서 호출, 미사용 include 제거(`Util/DefineEnum.h`, `NativeGameplayTags.h`, `Blueprint/UserWidget.h`), 로그 prefix `[SpyAssetManager]`→`[SKAssetManager]`, `Get()` fatal 대신 NewObject 유지(동작 보존).

**Files:**
- Create: `SkillProject/Plugins/SKAssetCore/Source/SKAssetCore/Public/SKAssetManager.h`
- Create: `SkillProject/Plugins/SKAssetCore/Source/SKAssetCore/Private/SKAssetManager.cpp`

- [ ] **Step 1: `Public/SKAssetManager.h` 작성**

```cpp
#pragma once

#include "Engine/AssetManager.h"
#include "SKAssetData.h"

#include "SKAssetManager.generated.h"

DECLARE_DELEGATE_OneParam(FSKAssetAndDelegate, UObject*);

UCLASS(Config = Game)
class SKASSETCORE_API USKAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	USKAssetManager();

	virtual void StartInitialLoading() override;

public:
	static USKAssetManager& Get();

	template<typename AssetType>
	static AssetType* GetAssetByPath(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);

	template<typename AssetType>
	static AssetType* GetAssetByName(const FName& AssetName, bool bKeepInMemory = true);

	template<typename AssetType>
	static TSubclassOf<AssetType> GetSubclassByPath(const TSoftClassPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);

	template<typename AssetType>
	static TSubclassOf<AssetType> GetSubclassByName(const FName& AssetName, bool bKeepInMemory = true);

public:
	static UObject* LoadAssetSync(const FSoftObjectPath& AssetPath);
	static void LoadAssetAsync(const FSoftObjectPath& AssetPath, const FSKAssetAndDelegate& OnComplete);
	static void UnloadAsset(const FSoftObjectPath& AssetPath);

protected:
	UPrimaryDataAsset* LoadPrimaryAssetSync(TSubclassOf<UPrimaryDataAsset> DataClass, const TSoftObjectPtr<UPrimaryDataAsset>& DataClassPath, FPrimaryAssetType PrimaryAssetType);
	void LoadAllPrimaryAssetsSync();
	void LoadPrimaryAssetsAsync(const TArray<FPrimaryAssetId>& AssetIds, const FSimpleDelegate& OnComplete);
	void UnloadAllPrimaryAssets();
	void UnloadAllAssets();

protected:
	void LogProgress(int32 Loaded, int32 Total);
	//# 진행률 훅 — 프로젝트 서브클래스가 오버라이드해 로딩스크린 % 등에 연결
	virtual void OnLoadProgress(int32 Loaded, int32 Total) {}
	void AddLoadedAsset(UObject* Asset);
	void RemoveLoadedAsset(UObject* Asset);
	void AddPrimaryAsset(UPrimaryDataAsset* Asset);
	void RemovePrimaryAsset(UPrimaryDataAsset* Asset);

protected:
	template <typename GameDataClass>
	const GameDataClass& GetOrLoadTypedGameData(const TSoftObjectPtr<GameDataClass>& DataPath)
	{
		if (TObjectPtr<UPrimaryDataAsset> const* pResult = GameDataMap.Find(GameDataClass::StaticClass()))
		{
			return *CastChecked<GameDataClass>(*pResult);
		}

		return *CastChecked<const GameDataClass>(LoadPrimaryAssetSync(GameDataClass::StaticClass(), DataPath, GameDataClass::StaticClass()->GetFName()));
	}

public:
	const USKAssetData& GetAssetData();

private:
	UPROPERTY()
	TSet<TObjectPtr<const UObject>> LoadedAssets;

	UPROPERTY(Transient)
	TMap<TObjectPtr<UClass>, TObjectPtr<UPrimaryDataAsset>> GameDataMap;

	FCriticalSection LoadedAssetsCritical;

private:
	UPROPERTY(Config)
	TSoftObjectPtr<USKAssetData> AssetDataPath;
};

template<typename AssetType>
AssetType* USKAssetManager::GetAssetByPath(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory)
{
	AssetType* LoadedAsset = nullptr;

	const FSoftObjectPath& AssetPath = AssetPointer.ToSoftObjectPath();

	if (AssetPath.IsValid())
	{
		LoadedAsset = AssetPointer.Get();
		if (LoadedAsset == nullptr)
		{
			LoadedAsset = Cast<AssetType>(LoadAssetSync(AssetPath));
		}

		if (LoadedAsset && bKeepInMemory)
		{
			Get().AddLoadedAsset(Cast<UObject>(LoadedAsset));
		}
	}

	return LoadedAsset;
}

template <typename AssetType>
AssetType* USKAssetManager::GetAssetByName(const FName& AssetName, bool bKeepInMemory)
{
	const USKAssetData& AssetData = Get().GetAssetData();
	const FSoftObjectPath& AssetPath = AssetData.GetAssetPathByName(AssetName);
	TSoftObjectPtr<AssetType> AssetPtr(AssetPath);
	return GetAssetByPath<AssetType>(AssetPtr, bKeepInMemory);
}

template<typename AssetType>
TSubclassOf<AssetType> USKAssetManager::GetSubclassByPath(const TSoftClassPtr<AssetType>& AssetPointer, bool bKeepInMemory)
{
	TSubclassOf<AssetType> LoadedSubclass;

	const FSoftObjectPath& AssetPath = AssetPointer.ToSoftObjectPath();

	if (AssetPath.IsValid())
	{
		LoadedSubclass = AssetPointer.Get();
		if (LoadedSubclass == nullptr)
		{
			LoadedSubclass = Cast<UClass>(LoadAssetSync(AssetPath));
		}

		if (LoadedSubclass && bKeepInMemory)
		{
			Get().AddLoadedAsset(Cast<UObject>(LoadedSubclass));
		}
	}

	return LoadedSubclass;
}

template <typename AssetType>
TSubclassOf<AssetType> USKAssetManager::GetSubclassByName(const FName& AssetName, bool bKeepInMemory)
{
	const USKAssetData& AssetData = Get().GetAssetData();
	const FSoftObjectPath& AssetPath = AssetData.GetAssetPathByName(AssetName);

	FString AssetPathString = AssetPath.GetAssetPathString();
	if (AssetPathString.EndsWith("_C") == false)
	{
		AssetPathString.Append(TEXT("_C"));
	}

	FSoftClassPath ClassPath(AssetPathString);
	TSoftClassPtr<AssetType> ClassPtr(ClassPath);
	return GetSubclassByPath<AssetType>(ClassPtr, bKeepInMemory);
}
```

- [ ] **Step 2: `Private/SKAssetManager.cpp` 작성** (원본 로직 전부, 클래스/델리게이트/로그/반환타입 변경, `OnLoadProgress` 호출 추가, 미사용 include 제거)

```cpp
#include "SKAssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKAssetManager)

USKAssetManager::USKAssetManager()
{
}

void USKAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	LoadAllPrimaryAssetsSync();
}

USKAssetManager& USKAssetManager::Get()
{
	check(GEngine);

	if (USKAssetManager* Singleton = Cast<USKAssetManager>(GEngine->AssetManager))
	{
		return *Singleton;
	}

	return *NewObject<USKAssetManager>();
}

UObject* USKAssetManager::LoadAssetSync(const FSoftObjectPath& AssetPath)
{
	UObject* Asset = nullptr;

	if (AssetPath.IsValid())
	{
		TUniquePtr<FScopeLogTime> LogTimePtr;

		if (UAssetManager::IsInitialized())
		{
			Asset = UAssetManager::GetStreamableManager().LoadSynchronous(AssetPath, false);
		}
		else
		{
			Asset = AssetPath.TryLoad();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Secondary Asset Sync Load Complete %s"), *AssetPath.ToString());

	return Asset;
}

void USKAssetManager::LoadAssetAsync(const FSoftObjectPath& AssetPath, const FSKAssetAndDelegate& OnComplete)
{
	//# 경로가 유효한지 확인
	if (AssetPath.IsValid() == false)
	{
		OnComplete.ExecuteIfBound(nullptr);
		return;
	}

	//# 이미 로드되어 있는지 확인
	if (UObject * Asset = AssetPath.ResolveObject())
	{
		OnComplete.ExecuteIfBound(Asset);
		return;
	}

	//# 비동기 로드
	Get().GetStreamableManager().RequestAsyncLoad(AssetPath, FStreamableDelegate::CreateLambda([AssetPath, OnComplete]()
		{
			//# 로드된 에셋이 유효하면 저장
			if (UObject* Asset = AssetPath.ResolveObject())
			{
				Get().AddLoadedAsset(Asset);
				OnComplete.ExecuteIfBound(Asset);
				UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Secondary Asset Async Load Complete: %s"), *AssetPath.ToString());
			}
			else
			{
				OnComplete.ExecuteIfBound(nullptr);
			}
		}));
}

void USKAssetManager::UnloadAsset(const FSoftObjectPath& AssetPath)
{
	UObject* Asset = AssetPath.ResolveObject();

	Get().RemoveLoadedAsset(Asset);

	UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Primary Asset Unloaded: %s"), *AssetPath.ToString());
}

void USKAssetManager::LoadAllPrimaryAssetsSync()
{
	//# 에디터 설정 창에서 정한 타입 정보 가져옴
	TArray<FPrimaryAssetTypeInfo> TypeInfos;
	GetPrimaryAssetTypeInfoList(TypeInfos);

	TArray<FPrimaryAssetId> AllAssetIds;
	for (const FPrimaryAssetTypeInfo& TypeInfo : TypeInfos)
	{
		//# 에셋 스캔 전이므로 강제로 스캔
		ScanPathsForPrimaryAssets(TypeInfo.PrimaryAssetType, TypeInfo.AssetScanPaths, TypeInfo.AssetBaseClassLoaded, false);

		//# 에셋 아이디 가져옴
		TArray<FPrimaryAssetId> AssetIds;
		GetPrimaryAssetIdList(TypeInfo.PrimaryAssetType, AssetIds);
		AllAssetIds.Append(AssetIds);

		for (const FPrimaryAssetId& AssetID : AssetIds)
		{
			UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Scan AssetType: %s, AssetID: %s"), *TypeInfo.PrimaryAssetType.ToString(), *AssetID.ToString());
		}
	}

	const int32 TotalCount = AllAssetIds.Num();
	int32 LoadedCount = 0;

	if (TotalCount == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("# [SKAssetManager] No Primary Assets"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Primary Asset Sync Load Start (%d)"), TotalCount);

	for (const FPrimaryAssetId& AssetId : AllAssetIds)
	{
		//# 동기 로드
		TSharedPtr<FStreamableHandle> LoadHandle = LoadPrimaryAsset(AssetId, TArray<FName>());
		if (LoadHandle.IsValid())
		{
			LoadHandle->WaitUntilComplete(0.0f, false);

			if (UPrimaryDataAsset* Asset = Cast<UPrimaryDataAsset>(LoadHandle->GetLoadedAsset()))
			{
				AddPrimaryAsset(Asset);

				FString ClassName = Asset->GetClass()->GetName();
				FString AssetName = Asset->GetName();
				UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Primary Asset Sync Load: Class[%s] Asset[%s]"), *ClassName, *AssetName);
			}
		}

		LoadedCount++;
		LogProgress(LoadedCount, TotalCount);
	}

	UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] All Primary Asset Sync Load Complete"));
}

UPrimaryDataAsset* USKAssetManager::LoadPrimaryAssetSync(TSubclassOf<UPrimaryDataAsset> DataClass, const TSoftObjectPtr<UPrimaryDataAsset>& DataClassPath, FPrimaryAssetType PrimaryAssetType)
{
	//# LoadAllPrimaryAssetsSync에서 일괄 로드 함
	//# 로드 못한 경우 개별 다운로드 진행
	UPrimaryDataAsset* Asset = nullptr;

	if (DataClassPath.IsNull() == false)
	{
		if (GIsEditor)
		{
			Asset = DataClassPath.LoadSynchronous();
			LoadPrimaryAssetsWithType(PrimaryAssetType);
		}
		else
		{
			TSharedPtr<FStreamableHandle> Handle = LoadPrimaryAssetsWithType(PrimaryAssetType);
			if (Handle.IsValid())
			{
				Handle->WaitUntilComplete(0.0f, false);

				Asset = Cast<UPrimaryDataAsset>(Handle->GetLoadedAsset());
			}
		}
	}

	if (Asset)
	{
		AddPrimaryAsset(Asset);

		FString ClassName = Asset->GetClass()->GetName();
		FString AssetName = Asset->GetName();
		UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Primary Asset Sync Load: Class[%s] Asset[%s]"), *ClassName, *AssetName);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Primary Asset Sync Load Failed: %s"), *DataClass->GetClass()->GetName());
	}

	return Asset;
}

void USKAssetManager::LoadPrimaryAssetsAsync(const TArray<FPrimaryAssetId>& AssetIds, const FSimpleDelegate& OnComplete)
{
	if (AssetIds.Num() == 0)
	{
		OnComplete.ExecuteIfBound();
		return;
	}

	//# 비동기 로드
	LoadPrimaryAssets(AssetIds, TArray<FName>(), FStreamableDelegate::CreateLambda([this, AssetIds, OnComplete]()
		{
			for (const FPrimaryAssetId& AssetID : AssetIds)
			{
				if (UObject* Asset = GetPrimaryAssetObject(AssetID))
				{
					AddPrimaryAsset(Cast<UPrimaryDataAsset>(Asset));
				}
			}

			UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Primary Asset Async Load Complete"));
			OnComplete.ExecuteIfBound();
		}));
}

void USKAssetManager::UnloadAllPrimaryAssets()
{
	//# 캐싱된 모든 PrimaryData 언로드
	for (auto& Data : GameDataMap)
	{
		UPrimaryDataAsset* Asset = Data.Value;
		UnloadPrimaryAsset(Asset->GetPrimaryAssetId());

		FString ClassName = Asset->GetClass()->GetName();
		FString AssetName = Asset->GetName();
		UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Primary Assets Unloaded AssetType: %s, AssetID: %s"), *ClassName, *AssetName);
	}

	GameDataMap.Empty();

	UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Primary Assets Unloaded Complete"));
}

void USKAssetManager::UnloadAllAssets()
{
	//# 일반 에셋은 참조만 끊어주면 다음 GC에 의해 제거됨
	LoadedAssets.Empty();

	UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Secondary Assets Unloaded Complete"));
}

void USKAssetManager::LogProgress(int32 Loaded, int32 Total)
{
	const int32 Percent = FMath::Clamp(FMath::RoundToInt((float)Loaded / (float)Total * 100.f), 1, 100);
	UE_LOG(LogTemp, Log, TEXT("# [SKAssetManager] Loading %d%% (%d / %d)"), Percent, Loaded, Total);

	//# 진행률 훅 — 서브클래스가 오버라이드
	OnLoadProgress(Loaded, Total);
}

void USKAssetManager::AddLoadedAsset(UObject* Asset)
{
	//# 로드된 에셋이 유효하면 저장(GC에 의해 삭제되지 않도록 참조 유지)
	if (ensureAlways(Asset))
	{
		FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);
		LoadedAssets.Add(Asset);
	}
}

void USKAssetManager::RemoveLoadedAsset(UObject* Asset)
{
	if (ensureAlways(Asset))
	{
		FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);
		LoadedAssets.Remove(Asset);
	}
}

void USKAssetManager::AddPrimaryAsset(UPrimaryDataAsset* Asset)
{
	//# 로드된 에셋이 유효하면 저장(GC에 의해 삭제되지 않도록 참조 유지)
	if (ensureAlways(Asset))
	{
		FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);
		GameDataMap.Add(Asset->GetClass(), Asset);
	}
}

void USKAssetManager::RemovePrimaryAsset(UPrimaryDataAsset* Asset)
{
	if (ensureAlways(Asset))
	{
		FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);
		GameDataMap.Remove(Asset->GetClass());
	}
}

const USKAssetData& USKAssetManager::GetAssetData()
{
	return GetOrLoadTypedGameData<USKAssetData>(AssetDataPath);
}
```

- [ ] **Step 3: 스테이징 + 커밋**

```
git add SkillProject/Plugins/SKAssetCore/Source/SKAssetCore/Public/SKAssetManager.h SkillProject/Plugins/SKAssetCore/Source/SKAssetCore/Private/SKAssetManager.cpp
git commit -m "[Chore] SKAssetManager — 로직 전체 플러그인 이동 + OnLoadProgress 훅 신규"
```

---

## Task 4: SkillProject 얇은 서브클래스로 재작성

**Files:**
- Modify: `Source/SkillProject/Manager/SpyAssetManager.h` (전체 교체)
- Modify: `Source/SkillProject/Manager/SpyAssetManager.cpp` (전체 교체)
- Modify: `Source/SkillProject/Data/SpyAssetData.h` (include 경로)

- [ ] **Step 1: `SpyAssetManager.h` 전체 교체**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SKAssetManager.h"

#include "SpyAssetManager.generated.h"

UCLASS(Config = Game)
class SKILLPROJECT_API USpyAssetManager : public USKAssetManager
{
	GENERATED_BODY()

protected:
	//# 프로젝트 확장점: 진행률 → 로딩스크린 UI 연동 지점
	virtual void OnLoadProgress(int32 Loaded, int32 Total) override;
};
```

- [ ] **Step 2: `SpyAssetManager.cpp` 전체 교체**

```cpp
// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/SpyAssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyAssetManager)

void USpyAssetManager::OnLoadProgress(int32 Loaded, int32 Total)
{
	//# 현재는 base LogProgress 로그로 충분. 추후 로딩스크린 위젯에 % 전달 지점.
}
```

- [ ] **Step 3: `SpyAssetData.h` include 경로 변경**

기존 `#include "Data/SKAssetData.h"` 한 줄을 `#include "SKAssetData.h"` 로 변경 (플러그인 Public 헤더). 나머지(`USpyAssetData : public USKAssetData`)는 그대로.

- [ ] **Step 4: 스테이징 + 커밋**

```
git add Source/SkillProject/Manager/SpyAssetManager.h Source/SkillProject/Manager/SpyAssetManager.cpp Source/SkillProject/Data/SpyAssetData.h
git commit -m "[Refactor] SpyAssetManager — USKAssetManager 얇은 서브클래스로 전환"
```

> 경로는 리포 루트 기준 `SkillProject/Source/...`. (커밋 명령은 `SkillProject/` 작업 디렉터리 기준으로 조정.)

---

## Task 5: call-site 수정 (델리게이트 rename + GetAssetData 타입 + include)

- [ ] **Step 1: 델리게이트 rename (4곳)** — `FSpyAssetAndDelegate` → `FSKAssetAndDelegate`
  - `Source/SkillProject/Character/SpyCharacter.cpp:309`
  - `Source/SkillProject/Character/AnimInstance/SpyCharacterAnimInstance.cpp:28`
  - `Source/SkillProject/Manager/SpyUIManager.cpp:52`
  - `Source/SkillProject/Manager/SpyUIManager.cpp:155`
  각 줄 `FSpyAssetAndDelegate LoadDelegate;` → `FSKAssetAndDelegate LoadDelegate;`

- [ ] **Step 2: `GetAssetData()` 반환 타입 (3곳)** — `const USpyAssetData&` → `const USKAssetData&`
  - `SpyCharacter.cpp:300` : `const USpyAssetData& SKAssetData` → `const USKAssetData& SKAssetData`
  - `SpyUIManager.cpp:43` : `const USpyAssetData& AssetData` → `const USKAssetData& AssetData`
  - `SpyUIManager.cpp:143` : 동일

- [ ] **Step 3: include 경로 (1곳)** — `SpyUIManager.cpp:9` : `#include "Data/SKAssetData.h"` → `#include "SKAssetData.h"`
  (SpyCharacter.cpp / SpyCharacterAnimInstance.cpp 는 `Manager/SpyAssetManager.h`→`SKAssetManager.h`→`SKAssetData.h` 로 USKAssetData 를 전이적으로 획득하므로 별도 include 불필요. 컴파일 에러 시 명시 include 추가.)

- [ ] **Step 4: 스테이징 + 커밋**

```
git add Source/SkillProject/Character/SpyCharacter.cpp Source/SkillProject/Character/AnimInstance/SpyCharacterAnimInstance.cpp Source/SkillProject/Manager/SpyUIManager.cpp
git commit -m "[Refactor] AssetManager 호출부 — 델리게이트 rename·GetAssetData 타입·include 정리"
```

---

## Task 6: 빌드 배선 (Build.cs ×2, uproject)

- [ ] **Step 1: `SkillProject.Build.cs`** — `PublicDependencyModuleNames` 에 `"SKAssetCore"` 추가 (예: `"SKGAS",` 다음 줄)

- [ ] **Step 2: `SpyDataEditorTool.Build.cs`** — `PublicDependencyModuleNames` 의 문자열 배열에 `"SKAssetCore"` 추가 (SSpyAssetsTab 이 USKAssetData 참조)

- [ ] **Step 3: `SkillProject.uproject`**
  - `Plugins` 배열에 추가:
    ```json
    {
        "Name": "SKAssetCore",
        "Enabled": true
    }
    ```
  - `Modules[0]`(SkillProject) 의 `AdditionalDependencies` 에 `"SKAssetCore"` 추가

- [ ] **Step 4: 스테이징 + 커밋**

```
git add SkillProject/Source/SkillProject/SkillProject.Build.cs SkillProject/Source/SpyDataEditorTool/SpyDataEditorTool.Build.cs SkillProject/SkillProject.uproject
git commit -m "[Chore] Build — SKAssetCore 의존성 추가(SkillProject/SpyDataEditorTool/uproject)"
```

---

## Task 7: 구 파일 삭제

- [ ] **Step 1: 이동된 원본 삭제**
  - `Source/SkillProject/Data/SKAssetData.h`
  - `Source/SkillProject/Data/SKAssetData.cpp`

- [ ] **Step 2: 잔존 참조 없음 확인**

Run: `rg -n "Data/SKAssetData.h" SkillProject/Source`
Expected: 매치 없음 (모두 플러그인 `SKAssetData.h` 로 이전됨).

- [ ] **Step 3: 스테이징 + 커밋**

```
git rm SkillProject/Source/SkillProject/Data/SKAssetData.h SkillProject/Source/SkillProject/Data/SKAssetData.cpp
git commit -m "[Chore] SKAssetData — 구 SkillProject 위치 파일 삭제(플러그인 이동 완료)"
```

---

## Task 8: 빌드 검증 (사용자) + 수정

> 이 태스크는 **사용자가 에디터/VS에서 수행**. 에이전트는 에러 회신을 받아 수정한다.

- [ ] **Step 1: 프로젝트 파일 재생성** — `SkillProject/Launch.bat` 실행 또는 uproject 우클릭 → Generate Visual Studio project files (신규 플러그인 모듈 인식).

- [ ] **Step 2: 컴파일** — Unreal Editor 또는 Visual Studio 에서 빌드. 에러가 있으면 파일:줄 회신 → 에이전트가 수정 후 재빌드 요청. 예상 리스크:
  - `SKAssetData.h`/`SKAssetManager.h` include 경로(플러그인 Public 자동 노출) — 안 잡히면 `SKAssetCore.Build.cs` 에 `PublicIncludePaths` 확인.
  - `Misc/DataValidation.h` unresolved → `SKAssetCore.Build.cs` 에 `bBuildEditor` 조건부 `"DataValidation"` 추가.
  - USKAssetData 미정의 에러(call-site) → 해당 .cpp 에 `#include "SKAssetData.h"` 명시 추가.

- [ ] **Step 3: 런타임 부트 로그 확인 (ini 무변경 검증 — #1 리스크)** — 에디터 PIE 또는 실행 후 Output Log 에서:
  - `# [SKAssetManager] Scan AssetType: SpyAssetData ...` → `# [SKAssetManager] All Primary Asset Sync Load Complete` 출력 확인.
  - **AssetDataPath 가 null resolve 되면**(SpyAssetData 로드 실패 로그) → **fallback**: `SkillProject/Config/DefaultGame.ini` 에 아래 섹션 추가(값 복제):
    ```ini
    [/Script/SKAssetCore.SKAssetManager]
    AssetDataPath=/Game/Spy/Data/SpyAssetData.SpyAssetData
    ```
    재실행해 로드 확인. (UE 상속 config는 leaf 섹션에서 읽히므로 대개 불필요하나 안전장치.)

- [ ] **Step 4: 기능 스모크** — 캐릭터/UI 로드가 기존과 동일 동작하는지(에셋 접근 경로 정상). 진행률 훅(`OnLoadProgress`) 호출은 로그로 확인 가능.

- [ ] **Step 5: 검증 통과 후 마무리** — 인프라 문서(`.claude/rules/unreal-infra.md`, `.claude/project.md`)의 SKAssetCore "분리 예정(미구현)" 표기를 "실존"으로 갱신할지 사용자에게 제안.

---

## Self-Review (작성자 점검)

**Spec 커버리지:** §2 플러그인 구조→T1; §3.1 USKAssetData→T2; §3.2 USKAssetManager+OnLoadProgress→T3; §4 얇은 서브클래스→T4; §5.1 GetAssetData 호출부 3곳→T5; §5.2 Build.cs→T6; §5.3 uproject→T6; §5.4 ini 무변경+검증→T8; §5.5 순환 의존 제거(SKAssetData.cpp Get())→T2; §7 검증→T8. 델리게이트 rename(스펙 §3.2)은 T5에서 호출부 4곳 반영. ✅

**Placeholder:** 없음. 모든 신규 파일 전체 내용 포함, 호출부는 파일:줄 + before/after 명시.

**타입/이름 일관성:** `USKAssetManager`·`USKAssetData`·`FSKAssetAndDelegate`·`SKASSETCORE_API`·`OnLoadProgress`·`AssetDataPath` 전 태스크 일관. `GetAssetData()` 반환은 base `const USKAssetData&` 단일 정의, 서브클래스 override 없음(호출부 3곳만 타입 변경).

**강결합 주의:** T2~T7 은 중간 컴파일 불가 — 전부 적용 후 T8에서 첫 빌드. 실행 시 순차 진행.
