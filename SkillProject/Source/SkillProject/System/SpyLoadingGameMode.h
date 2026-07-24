// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ModularGameMode.h"

#include "SpyLoadingGameMode.generated.h"

//# 로딩 맵 전용 GameMode — 로딩 UI 오픈과 파이프라인 킥오프만 담당한다
UCLASS(minimalapi)
class ASpyLoadingGameMode : public AModularGameModeBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
};
