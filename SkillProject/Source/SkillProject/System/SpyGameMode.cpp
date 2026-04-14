// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpyGameMode.h"

#include "UObject/ConstructorHelpers.h"
#include "System/SpyPlayerState.h"
#include "System/SpyGameState.h"
#include "Character/SpyCharacter.h"
#include "Manager/SpyAssetManager.h"
#include "Character/SpyPawnExtensionComponent.h"
#include "Data/SpyCharacterAssetData.h"
#include "Data/SpyAssetNames.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGameMode)

ASpyGameMode::ASpyGameMode()
{
}

void ASpyGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	//# �� �ε��� ���� ���� ���� �ʱ�ȭ �ܰ�
	Super::InitGame(MapName, Options, ErrorMessage);

	//# Lyraó�� �� ������ ����Ͽ� ���� �Ŵ����� �ý����� ������ ������ �� ����
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::HandleGameStartInitialization);
}

void ASpyGameMode::GenericPlayerInitialization(AController* NewPlayer)
{
	//# PlayerController �� PlayerState �ʱ�ȭ �� �ܰ�
	Super::GenericPlayerInitialization(NewPlayer);

	USpyAssetManager& AssetManager = USpyAssetManager::Get();
	if (USpyCharacterAssetData* CharacterAssetData = USpyAssetManager::GetAssetByName<USpyCharacterAssetData>(SpyAssetNames::CharacterAssetData))
	{
		//# PS�� ������ Set
		if (ASpyPlayerState* PS = NewPlayer->GetPlayerState<ASpyPlayerState>())
		{
			PS->SetCharacterAssetData(CharacterAssetData, NewPlayer->IsPlayerController());
		}
	}
}

APawn* ASpyGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient;

	//# ������ ������ ������ ���� �ܰ�� ��� ����
	SpawnInfo.bDeferConstruction = true;

	if (UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer))
	{
		//# ���� �� ���� �ܰ�, Lyra ������ Super ȣ������ �ʰ� ���� ����
		if (APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnInfo))
		{
			if (USpyPawnExtensionComponent* PawnExtensionComponent = USpyPawnExtensionComponent::FindPawnExtensionComponent(SpawnedPawn))
			{
				//# PawnExtensionComponent�� ������ Set
				if (const USpyCharacterAssetData* CharacterAssetData = GetCharacterDataForController(NewPlayer))
				{
					PawnExtensionComponent->SetCharacterAssetData(CharacterAssetData);
				}
			}

			//# ������ ���� �ܰ� ����
			SpawnedPawn->FinishSpawning(SpawnTransform);

			return SpawnedPawn;
		}
	}

	return nullptr;
}

void ASpyGameMode::HandleGameStartInitialization()
{
	DefaultPawnClass = USpyAssetManager::GetSubclassByName<APawn>(SpyAssetNames::DefaultCharacter);
	PlayerControllerClass = USpyAssetManager::GetSubclassByName<APlayerController>(SpyAssetNames::DefaultPlayerController);
	PlayerStateClass = ASpyPlayerState::StaticClass();
	GameStateClass = USpyAssetManager::GetSubclassByName<ASpyGameState>(SpyAssetNames::DefaultGameState);
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
