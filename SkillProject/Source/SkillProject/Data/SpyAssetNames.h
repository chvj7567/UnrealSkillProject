#pragma once

#include "HAL/Platform.h"

namespace SpyAssetNames
{
	// DataAsset 등록명
	inline const FName CharacterAssetData = TEXT("SpyCharacterAssetData");
	inline const FName AnimAssetData      = TEXT("SpyAnimAssetData");
	inline const FName MovementConfig     = TEXT("SpyMovementConfig");
	inline const FName GrapplePromptWidget = TEXT("WBP_GrapplePrompt");

	// 클래스 등록명 (GameMode 초기화용)
	inline const FName DefaultCharacter        = TEXT("SpyCharacter");
	inline const FName DefaultPlayerController = TEXT("SpyPlayerController");
	inline const FName DefaultGameState        = TEXT("SpyGameState");
}

namespace SpyActorTags
{
	// 스폰 포인트 액터 태그
	inline const FName SpawnEnemy = TEXT("SpawnEnemy");
}

namespace SpyAINames
{
	// AI 청각 이벤트 태그
	inline const FName AttackNoise = TEXT("AttackNoise");
}
