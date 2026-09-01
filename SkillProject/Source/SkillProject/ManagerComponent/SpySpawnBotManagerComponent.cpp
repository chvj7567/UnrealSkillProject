// Fill out your copyright notice in the Description page of Project Settings.


#include "ManagerComponent/SpySpawnBotManagerComponent.h"
#include "GameFramework/GameStateBase.h"
#include "System/SpyPlayerState.h"
#include "System/SpyAIController.h"
#include "System/SpyGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/TargetPoint.h"
#include "Data/SpyAssetNames.h"
#include "Character/SpyCharacter.h"
#include "Character/SpyHealthComponent.h"

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
			//# PlayerState 설정
			if (ASpyPlayerState* PS = NewController->GetPlayerState<ASpyPlayerState>())
			{
				PS->SetPlayerName(CreateBotName(PS->GetPlayerId()));
			}

			if (ASpyAIController* SpyAIController = Cast<ASpyAIController>(NewController))
			{
				SpyAIController->SetBehaviorTree(BehaviorTreeAsset);
			}

			//# 폰 생성 및 빙의
			GameMode->GenericPlayerInitialization(NewController);

			//# Tranform 지정
			FTransform SpawnTransform(InRotator, InLocation, FVector::OneVector);

			//# 폰 생성 및 빙의
			GameMode->RestartPlayerAtTransform(NewController, SpawnTransform);
		}

		FSpySpawnedBotInfo BotInfo;
		BotInfo.Controller = NewController;
		BotInfo.SpawnTransform = FTransform(InRotator, InLocation, FVector::OneVector);
		SpawnedBotList.Add(BotInfo);

		//# 서버 전용 — 사망 신호를 구독해 리스폰 타이밍을 스케줄링한다.
		if (APawn* ControlledPawn = NewController->GetPawn())
		{
			if (USpyHealthComponent* HealthComp = USpyHealthComponent::FindHealthComponent(ControlledPawn))
			{
				HealthComp->OnDeath.AddDynamic(this, &ThisClass::HandleBotDeath);
			}
		}

		UE_LOG(LogTemp, Log, TEXT("# [SpawnBotManager]: Spawn %s"), *NewController->GetName());
	}
}

void USpySpawnBotManagerComponent::RemoveOneBot()
{
	if (SpawnedBotList.Num() > 0)
	{
		const int32 BotToRemoveIndex = FMath::RandRange(0, SpawnedBotList.Num() - 1);
		AAIController* BotToRemove = SpawnedBotList[BotToRemoveIndex].Controller;

		if (BotToRemove)
		{
			//# 컨트롤러 파괴 (이때 로그아웃 처리됨)
			if (APawn* ControlledPawn = BotToRemove->GetPawn())
			{
				ControlledPawn->Destroy();
			}

			//# 컨트롤러 파괴 (이때 로그아웃 처리됨)
			BotToRemove->Destroy();
		}

		if (FTimerHandle* PendingRespawnTimer = RespawnTimers.Find(BotToRemove))
		{
			GetWorld()->GetTimerManager().ClearTimer(*PendingRespawnTimer);
			RespawnTimers.Remove(BotToRemove);
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

void USpySpawnBotManagerComponent::HandleBotDeath(AActor* InOwningActor, AActor* InCauserActor)
{
	//# 서버 전용 — OnDeath 는 서버/클라 양쪽에서 발화될 수 있으므로(SKAttributeSet OnRep_Health 포함) 반드시 가드.
	if (GetOwnerRole() < ROLE_Authority)
		return;

	APawn* DeadPawn = Cast<APawn>(InOwningActor);
	if (DeadPawn == nullptr)
		return;

	//# 이 매니저가 스폰한 봇은 항상 ASpyAIController — DissolveDurationSeconds 조회가 이 타입에 있다.
	ASpyAIController* DeadController = Cast<ASpyAIController>(DeadPawn->GetController());
	if (DeadController == nullptr)
		return;

	const FSpySpawnedBotInfo* FoundInfo = SpawnedBotList.FindByPredicate(
		[DeadController](const FSpySpawnedBotInfo& Info) { return Info.Controller == DeadController; });

	if (FoundInfo == nullptr)
		return;

	const float DissolveDurationSeconds = DeadController->GetDissolveDurationSeconds();

	TWeakObjectPtr<AAIController> WeakController(DeadController);
	FTimerHandle& RespawnTimer = RespawnTimers.FindOrAdd(DeadController);
	FTimerDelegate RespawnDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::RespawnBot, WeakController);
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, RespawnDelegate, DissolveDurationSeconds, false);
}

void USpySpawnBotManagerComponent::RespawnBot(TWeakObjectPtr<AAIController> InController)
{
	if (InController.IsValid() == false)
		return;

	AAIController* Controller = InController.Get();
	RespawnTimers.Remove(Controller);

	const FSpySpawnedBotInfo* FoundInfo = SpawnedBotList.FindByPredicate(
		[Controller](const FSpySpawnedBotInfo& Info) { return Info.Controller == Controller; });

	if (FoundInfo == nullptr)
		return;

	if (ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(Controller->GetPawn()))
	{
		SpyCharacter->OnRespawn(FoundInfo->SpawnTransform);
	}
}