// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkillProjectGameMode.h"
#include "Character/SkillProjectCharacter.h"
#include "SkillProjectPlayerController.h"
#include "UObject/ConstructorHelpers.h"
#include "World/SkillProjectPlayerState.h"

ASkillProjectGameMode::ASkillProjectGameMode()
{
	static ConstructorHelpers::FClassFinder<APawn> BPPawnClass(TEXT("/Game/Spy/Blueprints/Character/BP_SpyCharacter"));
	if (BPPawnClass.Class)
	{
		DefaultPawnClass = BPPawnClass.Class;
	}

	static ConstructorHelpers::FClassFinder<APlayerController> BPPlayerController(TEXT("/Game/Spy/Blueprints/System/BP_SpyCharacterController"));
	if (BPPlayerController.Class)
	{
		PlayerControllerClass = BPPlayerController.Class;
	}

	PlayerStateClass = ASkillProjectPlayerState::StaticClass();
}
