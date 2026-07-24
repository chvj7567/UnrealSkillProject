// Copyright Epic Games, Inc. All Rights Reserved.

#include "System/SpyLoadingGameMode.h"

#include "Engine/GameInstance.h"
#include "Manager/SpyLoadingSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyLoadingGameMode)

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
