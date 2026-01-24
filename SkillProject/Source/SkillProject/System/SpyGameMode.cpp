// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpyGameMode.h"

#include "UObject/ConstructorHelpers.h"
#include "System/SpyPlayerState.h"
#include "Character/SpyCharacter.h"
#include "Manager/SpyAssetManager.h"
#include "Character/SpyPawnExtensionComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGameMode)

ASpyGameMode::ASpyGameMode()
{
}

void ASpyGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	//# 맵 로딩이 끝난 이후 게임 초기화 단계
	Super::InitGame(MapName, Options, ErrorMessage);

	//# Lyra처럼 한 프레임 대기하여 에셋 매니저와 시스템이 완전히 안착된 후 실행
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::HandleGameStartInitialization);
}

void ASpyGameMode::GenericPlayerInitialization(AController* NewPlayer)
{
	//# PlayerController 및 PlayerState 초기화 후 단계
	Super::GenericPlayerInitialization(NewPlayer);

	USpyAssetManager& AssetManager = USpyAssetManager::Get();
	const USpyCharacterAssetData& CharacterAssetData = AssetManager.GetCharacterAssetData();

	//# PS에 데이터 Set
	if (ASpyPlayerState* PS = NewPlayer->GetPlayerState<ASpyPlayerState>())
	{
		PS->SetCharacterAssetData(CharacterAssetData);
	}
}

APawn* ASpyGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient;

	//# 스폰은 하지만 생성자 이후 단계는 잠시 정지
	SpawnInfo.bDeferConstruction = true;

	if (UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer))
	{
		//# 실제 폰 생성 단계, Lyra 식으로 Super 호출하지 않고 직접 생성
		if (APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnInfo))
		{
			if (USpyPawnExtensionComponent* PawnExtensionComponent = USpyPawnExtensionComponent::FindPawnExtensionComponent(SpawnedPawn))
			{
				//# PawnExtensionComponent에 데이터 Set
				if (const USpyCharacterAssetData* CharacterAssetData = GetCharacterDataForController(NewPlayer))
				{
					PawnExtensionComponent->SetCharacterAssetData(*CharacterAssetData);
				}
			}

			//# 생성자 이후 단계 실행
			SpawnedPawn->FinishSpawning(SpawnTransform);

			return SpawnedPawn;
		}
	}

	return nullptr;
}

void ASpyGameMode::HandleGameStartInitialization()
{
	DefaultPawnClass = USpyAssetManager::GetSubclassByName<APawn>(TEXT("SpyCharacter"));
	PlayerControllerClass = USpyAssetManager::GetSubclassByName<APlayerController>(TEXT("SpyPlayerController"));
	PlayerStateClass = ASpyPlayerState::StaticClass();
}

const USpyCharacterAssetData* ASpyGameMode::GetCharacterDataForController(const AController* InController) const
{
	if (InController == nullptr)
		return nullptr;

	if (const ASpyPlayerState* SpyPS = InController->GetPlayerState<ASpyPlayerState>())
	{
		if (const USpyCharacterAssetData* CharacterAssetData = SpyPS->GetCharacterAssetData())
		{
			return CharacterAssetData;
		}
	}

	return nullptr;
}
