// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SKAssetManager.h"
#include "Data/SpyAssetData.h"

#include "SpyAssetManager.generated.h"

UCLASS(Config = Game)
class SKILLPROJECT_API USpyAssetManager : public USKAssetManager
{
	GENERATED_BODY()

public:
	//# base 의 GetAssetData 를 프로젝트 concrete 타입(USpyAssetData)으로 구현
	//# — PrimaryAssetType("SpyAssetData")·캐시 키가 일치해 1회만 로드됨
	virtual const USKAssetData& GetAssetData() override;

protected:
	//# 프로젝트 확장점: 진행률 → 로딩스크린 UI 연동 지점
	virtual void OnLoadProgress(int32 Loaded, int32 Total) override;

private:
	UPROPERTY(Config)
	TSoftObjectPtr<USpyAssetData> AssetDataPath;
};
