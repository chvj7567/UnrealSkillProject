// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpyGameMode.h"
#include "Character/SpyCharacter.h"
#include "SpyPlayerController.h"
#include "UObject/ConstructorHelpers.h"
#include "System/SpyPlayerState.h"
#include "Manager/SpyAssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGameMode)

ASpyGameMode::ASpyGameMode()
{
	/*static ConstructorHelpers::FClassFinder<APawn> BPPawnClass(TEXT("/Game/Spy/Blueprints/Character/BP_SpyCharacter"));
	if (BPPawnClass.Class)
	{
		DefaultPawnClass = BPPawnClass.Class;
	}

	static ConstructorHelpers::FClassFinder<APlayerController> BPPlayerController(TEXT("/Game/Spy/Blueprints/System/BP_SpyPlayerController"));
	if (BPPlayerController.Class)
	{
		PlayerControllerClass = BPPlayerController.Class;
	}*/
}

void ASpyGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	DefaultPawnClass = USpyAssetManager::GetSubclassByName<APawn>(TEXT("SpyCharacter"));
	PlayerControllerClass = USpyAssetManager::GetSubclassByName<APlayerController>(TEXT("SpyPlayerController"));
	PlayerStateClass = ASpyPlayerState::StaticClass();
}
