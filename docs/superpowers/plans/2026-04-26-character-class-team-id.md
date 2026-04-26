# Character Class TeamId Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** `FCharacterAssetEntry`에 `TeamId` 필드를 추가해 클래스 타입별 팀 번호를 DataAsset 에디터에서 직접 설정할 수 있게 한다.

**Architecture:** `FCharacterAssetEntry` 구조체에 `uint8 TeamId` 필드를 추가하고, `SetCharacterAssetData`에서 하드코딩된 팀 번호 대신 Entry에서 읽어온 값을 사용한다.

**Tech Stack:** Unreal Engine 5.7, C++, GameplayAbilities (FGenericTeamId)

---

## File Map

| 파일 | 변경 |
|------|------|
| `SkillProject/Source/SkillProject/Data/SpyCharacterAssetData.h` | `FCharacterAssetEntry`에 `TeamId` 필드 추가 |
| `SkillProject/Source/SkillProject/System/SpyPlayerState.cpp` | `SetCharacterAssetData` 수정 — 하드코딩 제거, Entry에서 TeamId 읽기 |

---

### Task 1: `FCharacterAssetEntry`에 `TeamId` 필드 추가

**Files:**
- Modify: `SkillProject/Source/SkillProject/Data/SpyCharacterAssetData.h`

- [ ] **Step 1: `FCharacterAssetEntry` 구조체에 `TeamId` 추가**

`SpyCharacterAssetData.h`의 `FCharacterAssetEntry` 구조체 끝에 아래 프로퍼티를 추가한다.

```cpp
UPROPERTY(EditDefaultsOnly, Category = "Team")
uint8 TeamId = 0;
```

추가 위치는 `WeaponSocketName` 프로퍼티 바로 아래:

```cpp
// 변경 후 FCharacterAssetEntry 전체
USTRUCT()
struct FCharacterAssetEntry
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, Meta = (Categories = "Character.Class"))
    FGameplayTag ClassType;

    UPROPERTY(EditDefaultsOnly)
    TArray<TSubclassOf<UActorComponent>> CharacterComponentClasses;

    UPROPERTY(EditDefaultsOnly)
    TArray<TObjectPtr<USpyAbilityData>> ClassSkills;

    UPROPERTY(EditDefaultsOnly)
    TObjectPtr<USpyComboAssetData> ClassCombos;

    UPROPERTY(EditDefaultsOnly)
    TObjectPtr<USpyInputConfig> InputConfig;

    UPROPERTY(EditDefaultsOnly)
    TMap<TObjectPtr<UInputMappingContext>, int32> InputMappingContexts;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    FName WeaponAssetName;

    UPROPERTY(EditDefaultsOnly, Category = "Weapon")
    FName WeaponSocketName = TEXT("weapon_socket");

    UPROPERTY(EditDefaultsOnly, Category = "Team")
    uint8 TeamId = 0;
};
```

- [ ] **Step 2: 컴파일 확인**

Unreal Editor 또는 Visual Studio에서 빌드. 오류 없이 컴파일되어야 한다.
빌드 성공 시 에디터의 `FCharacterAssetEntry` 디테일 패널에 "Team" 카테고리와 `Team Id` 항목이 노출된다.

- [ ] **Step 3: 커밋**

```bash
git add SkillProject/Source/SkillProject/Data/SpyCharacterAssetData.h
git commit -m "[Data] FCharacterAssetEntry에 TeamId 필드 추가"
```

---

### Task 2: `SetCharacterAssetData`에서 Entry의 TeamId 사용

**Files:**
- Modify: `SkillProject/Source/SkillProject/System/SpyPlayerState.cpp`

- [ ] **Step 1: 하드코딩된 팀 세팅 제거 및 Entry 기반으로 수정**

`SpyPlayerState.cpp`의 `SetCharacterAssetData` 함수를 아래와 같이 수정한다.

```cpp
void ASpyPlayerState::SetCharacterAssetData(USpyCharacterAssetData* InCharacterAssetData, bool bInIsPlayer)
{
    //# 서버에서만 세팅
    if (GetLocalRole() != ROLE_Authority)
        return;

    //# 이미 세팅됨
    if (CharacterAssetData)
        return;

    CharacterAssetData = InCharacterAssetData;

    for (const USpyAbilityData* AbilityData : CharacterAssetData->CharacterAssets.CommonSkills)
    {
        if (AbilityData)
        {
            AbilityData->GiveToAbilitySystem(AbilitySystemComponent, nullptr);
        }
    }

    FCharacterAssetEntry* Entry = CharacterAssetData->CharacterAssets.AssetEntries.FindByPredicate([bInIsPlayer](const FCharacterAssetEntry& Data)
        {
            if (bInIsPlayer)
            {
                return Data.ClassType == SpyGameplayTags::Character_Class_Normal;
            }
            else
            {
                return Data.ClassType == SpyGameplayTags::Character_Class_AI;
            }
        });

    if (Entry)
    {
        SetGenericTeamId(Entry->TeamId);

        for (const USpyAbilityData* AbilityData : Entry->ClassSkills)
        {
            if (AbilityData)
            {
                AbilityData->GiveToAbilitySystem(AbilitySystemComponent, nullptr);
            }
        }
    }

    UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, NAME_AbilityReady);
}
```

핵심 변경 사항:
- `SetGenericTeamId(bInIsPlayer ? 100 : 1)` 한 줄 제거
- Entry null 체크(`if (Entry)`) 안으로 ClassSkills 루프 이동
- Entry가 존재할 때 `SetGenericTeamId(Entry->TeamId)` 호출

- [ ] **Step 2: 컴파일 확인**

Unreal Editor 또는 Visual Studio에서 빌드. 오류 없이 컴파일되어야 한다.

- [ ] **Step 3: 에디터에서 BP 에셋 TeamId 값 설정**

에디터에서 `USpyCharacterAssetData` BP 에셋을 열고:
- `Character_Class_Normal` Entry → `Team Id = 100`
- `Character_Class_AI` Entry → `Team Id = 1`

기존 하드코딩 값과 동일하게 유지해 동작 변화 없음을 확인.

- [ ] **Step 4: 런타임 동작 검증**

PIE(Play In Editor)에서 플레이어와 AI가 각각 올바른 팀 ID를 갖는지 확인.
`SpyPlayerState::SetGenericTeamId` 로그에서 `# SpyPlayerState: SetGenericTeamId 100` (플레이어) 및 `1` (AI) 출력 확인.

- [ ] **Step 5: 커밋**

```bash
git add SkillProject/Source/SkillProject/System/SpyPlayerState.cpp
git commit -m "[Feature] 클래스 타입별 팀 번호 DataAsset 기반으로 변경"
```
