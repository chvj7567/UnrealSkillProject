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
	//# Lyra처럼 한 프레임 대기하여 에셋 매니저와 시스템이 완전히 안착된 후 실행
	Super::InitGame(MapName, Options, ErrorMessage);

	//# Lyra처럼 한 프레임 대기하여 에셋 매니저와 시스템이 완전히 안착된 후 실행
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::HandleGameStartInitialization);
}

void ASpyGameMode::GenericPlayerInitialization(AController* NewPlayer)
{
	//# PlayerController 및 PlayerState 초기화 후 단계
	Super::GenericPlayerInitialization(NewPlayer);

	if (USpyCharacterAssetData* CharacterAssetData = USpyAssetManager::GetAssetByName<USpyCharacterAssetData>(SpyAssetNames::CharacterAssetData))
	{
		//# PS에 데이터 Set
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

	//# 맵 로딩이 끝난 이후 게임 초기화 단계
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
					PawnExtensionComponent->SetCharacterAssetData(CharacterAssetData);
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
	DefaultPawnClass = USpyAssetManager::GetSubclassByName<APawn>(SpyAssetNames::DefaultCharacter);
	PlayerControllerClass = USpyAssetManager::GetSubclassByName<APlayerController>(SpyAssetNames::DefaultPlayerController);
	//# BP_SpyPlayerState 를 쓰려면 형제 항목들처럼 이름 룩업이어야 한다.
	//# C++ 클래스를 직접 지정하면 BP 기본값(MissionComponent 의 MissionConfig 등)이 런타임에 반영되지 않는다
	PlayerStateClass = USpyAssetManager::GetSubclassByName<ASpyPlayerState>(SpyAssetNames::DefaultPlayerState);

	//# 형제 항목과 달리 폴백을 둔다 — PlayerState 가 nullptr 이면 ASC·어트리뷰트까지 전부 사라져
	//# 게임이 통째로 깨지므로, 에셋 등록 누락 시에도 C++ 기본 클래스로 최소 동작을 보장한다
	if (PlayerStateClass == nullptr)
	{
		PlayerStateClass = ASpyPlayerState::StaticClass();

		UE_LOG(LogTemp, Warning, TEXT("# [SpyGameMode] PlayerState 클래스 룩업 실패(%s) — ASpyPlayerState로 폴백합니다. BP 기본값(MissionConfig 등)이 적용되지 않습니다."), *SpyAssetNames::DefaultPlayerState.ToString());
	}

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
