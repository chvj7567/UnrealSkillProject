# Character Class TeamId Design

## Overview

`FCharacterAssetEntry`에 `TeamId` 필드를 추가해 클래스 타입별 팀 번호를 DataAsset에서 직접 설정할 수 있도록 한다.
현재 `SetCharacterAssetData`에 하드코딩된 `bInIsPlayer ? 100 : 1` 로직을 제거한다.

## Problem

`ASpyPlayerState::SetCharacterAssetData`에서 팀 번호가 하드코딩:

```cpp
SetGenericTeamId(bInIsPlayer ? 100 : 1);
```

새 클래스 타입이 추가될 때마다 코드를 수정해야 하고, 기획자가 팀 번호를 조정할 수 없다.

## Solution

### 데이터 변경: `FCharacterAssetEntry` (`SpyCharacterAssetData.h`)

```cpp
UPROPERTY(EditDefaultsOnly, Category = "Team")
uint8 TeamId = 0;
```

- `FGenericTeamId`가 `uint8`을 사용하므로 타입을 맞춤
- 기본값 `0` = 미설정 상태

### 로직 변경: `SetCharacterAssetData` (`SpyPlayerState.cpp`)

1. 하드코딩된 `SetGenericTeamId(bInIsPlayer ? 100 : 1)` 제거
2. ClassType으로 Entry 탐색 (기존 로직 재사용)
3. Entry가 존재하면 `SetGenericTeamId(Entry->TeamId)` 호출
4. Entry가 없으면 `TeamId = 0` 유지

```cpp
// 변경 전
SetGenericTeamId(bInIsPlayer ? 100 : 1);

// 변경 후 (Entry 탐색 후)
if (Entry)
{
    SetGenericTeamId(Entry->TeamId);
}
```

## Impact

| 파일 | 변경 내용 |
|------|----------|
| `SpyCharacterAssetData.h` | `FCharacterAssetEntry`에 `TeamId` 필드 추가 |
| `SpyPlayerState.cpp` | `SetCharacterAssetData` 내 하드코딩 제거, Entry에서 TeamId 읽도록 수정 |

다른 파일 변경 없음. BP 에셋에서 각 Entry의 TeamId 값을 에디터로 설정.

## Usage

에디터에서 `USpyCharacterAssetData` BP 에셋 열기 → `CharacterAssets.AssetEntries` 배열 → 각 Entry의 `Team Id` 필드에 팀 번호 입력.

- `Character_Class_Normal` Entry: `TeamId = 100`
- `Character_Class_AI` Entry: `TeamId = 1`
