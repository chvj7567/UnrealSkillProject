// Copyright Epic Games, Inc. All Rights Reserved.

#include "System/SpyLoadingGameMode.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Manager/SpyLoadingSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyLoadingGameMode)

void ASpyLoadingGameMode::BeginPlay()
{
	Super::BeginPlay();

	//# 데디 서버는 로딩 화면이 없다 — 게임플레이 맵으로 바로 서버 트래블한다.
	//# GameMode 는 데디에서도 살아 있다(로딩 서브시스템은 생성이 막힌다) — 그래서 판정이 여기 있어야 한다.
	if (GetNetMode() == NM_DedicatedServer)
	{
		const FName GameplayMapPackageName = USpyLoadingSubsystem::ResolveGameplayMapPackageName();
		if (GameplayMapPackageName.IsNone())
			return;

		UWorld* World = GetWorld();
		if (World == nullptr)
			return;

		const FString TravelURL = GameplayMapPackageName.ToString();
		UE_LOG(LogTemp, Log, TEXT("# [SpyLoadingGameMode] 데디서버 — 로딩 생략, 게임플레이 맵 서버 트래블: %s"), *TravelURL);

		//# BeginPlay(월드 초기화) 도중 트래블하지 않고 다음 틱으로 지연한다 — 프로젝트 표준 패턴
		TWeakObjectPtr<UWorld> WeakWorld = World;
		GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [WeakWorld, TravelURL]()
		{
			if (WeakWorld.IsValid())
			{
				WeakWorld->ServerTravel(TravelURL);
			}
		}));
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance == nullptr)
		return;

	//# 클라·스탠드얼론만 로딩 파이프라인을 돈다(로딩 UI 오픈도 서브시스템이 책임진다)
	if (USpyLoadingSubsystem* LoadingSubsystem = GameInstance->GetSubsystem<USpyLoadingSubsystem>())
	{
		LoadingSubsystem->StartLoading();
	}
}
