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
