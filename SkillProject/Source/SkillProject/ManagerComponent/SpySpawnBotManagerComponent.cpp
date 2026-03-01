// Fill out your copyright notice in the Description page of Project Settings.


#include "ManagerComponent/SpySpawnBotManagerComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "System/SpyAIController.h"
#include "System/SpyGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/TargetPoint.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpySpawnBotManagerComponent)

USpySpawnBotManagerComponent::USpySpawnBotManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USpySpawnBotManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwnerRole() == ROLE_Authority)
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &USpySpawnBotManagerComponent::ServerCreateBots);
	}
}

void USpySpawnBotManagerComponent::SpawnOneBot(FVector InLocation, FRotator InRotator)
{
	//# 서버에서만 실행
	if (GetOwnerRole() < ROLE_Authority)
		return;

	if (BotControllerClass == nullptr)
		return;

	UWorld* World = GetWorld();

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnInfo.OverrideLevel = GetOwner()->GetLevel();
	SpawnInfo.ObjectFlags |= RF_Transient;

	//# 컨트롤러 먼저 생성
	if (AAIController* NewController = World->SpawnActor<AAIController>(BotControllerClass, InLocation, InRotator, SpawnInfo))
	{
		if (ASpyGameMode* GameMode = Cast<ASpyGameMode>(World->GetAuthGameMode()))
		{
			//# PlayerState 이름 설정
			if (APlayerState* PS = NewController->GetPlayerState<APlayerState>())
			{
				PS->SetPlayerName(CreateBotName(PS->GetPlayerId()));
			}

			if (ASpyAIController* SpyAIController = Cast<ASpyAIController>(NewController))
			{
				SpyAIController->SetBehaviorTree(BehaviorTreeAsset);
			}

			//# 봇도 기본적인 컨트롤러 초기화 프로세스
			GameMode->GenericPlayerInitialization(NewController);

			//# Tranform 지정
			FTransform SpawnTransform(InRotator, InLocation, FVector::OneVector);

			//# 폰 생성 및 빙의
			GameMode->RestartPlayerAtTransform(NewController, SpawnTransform);
		}

		SpawnedBotList.Add(NewController);
		UE_LOG(LogTemp, Log, TEXT("# [SpawnBotManager]: Spawn %s"), *NewController->GetName());
	}
}

void USpySpawnBotManagerComponent::RemoveOneBot()
{
	if (SpawnedBotList.Num() > 0)
	{
		const int32 BotToRemoveIndex = FMath::RandRange(0, SpawnedBotList.Num() - 1);
		AAIController* BotToRemove = SpawnedBotList[BotToRemoveIndex];

		if (BotToRemove)
		{
			//# 조종 중인 폰 파괴
			if (APawn* ControlledPawn = BotToRemove->GetPawn())
			{
				ControlledPawn->Destroy();
			}

			//# 컨트롤러 파괴 (이때 로그아웃 처리됨)
			BotToRemove->Destroy();
		}

		SpawnedBotList.RemoveAtSwap(BotToRemoveIndex);
	}
}

void USpySpawnBotManagerComponent::ServerCreateBots_Implementation()
{
	TArray<AActor*> SpawnPoints;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("SpawnEnemy"), SpawnPoints);

	for (AActor* SpawnPoint : SpawnPoints)
	{
		if (SpawnPoint)
		{
			FVector Location = SpawnPoint->GetActorLocation();
			FRotator Rotation = SpawnPoint->GetActorRotation();

			SpawnOneBot(Location, Rotation);
		}
	}
}

FString USpySpawnBotManagerComponent::CreateBotName(int32 ID)
{
	return FString::Printf(TEXT("%s_%d"), *BotNamePrefix, ID);
}