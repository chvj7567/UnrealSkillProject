// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/SpyAssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyAssetManager)

const USKAssetData& USpyAssetManager::GetAssetData()
{
	return GetOrLoadTypedGameData<USpyAssetData>(AssetDataPath);
}

void USpyAssetManager::OnLoadProgress(int32 Loaded, int32 Total)
{
	//# 현재는 base LogProgress 로그로 충분. 추후 로딩스크린 위젯에 % 전달 지점.
}
