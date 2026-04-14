// Fill out your copyright notice in the Description page of Project Settings.


#include "ManagerComponent/SpySpawnBotManagerComponent.h"
#include "GameFramework/GameStateBase.h"
#include "System/SpyPlayerState.h"
#include "System/SpyAIController.h"
#include "System/SpyGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/TargetPoint.h"
#include "Data/SpyAssetNames.h"

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
	//# ���������� ����
	if (GetOwnerRole() < ROLE_Authority)
		return;

	if (BotControllerClass == nullptr)
		return;

	UWorld* World = GetWorld();

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnInfo.OverrideLevel = GetOwner()->GetLevel();
	SpawnInfo.ObjectFlags |= RF_Transient;

	//# ��Ʈ�ѷ� ���� ����
	if (AAIController* NewController = World->SpawnActor<AAIController>(BotControllerClass, InLocation, InRotator, SpawnInfo))
	{
		if (ASpyGameMode* GameMode = Cast<ASpyGameMode>(World->GetAuthGameMode()))
		{
			//# PlayerState ����
			if (ASpyPlayerState* PS = NewController->GetPlayerState<ASpyPlayerState>())
			{
				PS->SetPlayerName(CreateBotName(PS->GetPlayerId()));
			}

			if (ASpyAIController* SpyAIController = Cast<ASpyAIController>(NewController))
			{
				SpyAIController->SetBehaviorTree(BehaviorTreeAsset);
			}

			//# ���� �⺻���� ��Ʈ�ѷ� �ʱ�ȭ ���μ���
			GameMode->GenericPlayerInitialization(NewController);

			//# Tranform ����
			FTransform SpawnTransform(InRotator, InLocation, FVector::OneVector);

			//# �� ���� �� ����
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
			//# ���� ���� �� �ı�
			if (APawn* ControlledPawn = BotToRemove->GetPawn())
			{
				ControlledPawn->Destroy();
			}

			//# ��Ʈ�ѷ� �ı� (�̶� �α׾ƿ� ó����)
			BotToRemove->Destroy();
		}

		SpawnedBotList.RemoveAtSwap(BotToRemoveIndex);
	}
}

void USpySpawnBotManagerComponent::ServerCreateBots_Implementation()
{
	TArray<AActor*> SpawnPoints;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), SpyActorTags::SpawnEnemy, SpawnPoints);

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