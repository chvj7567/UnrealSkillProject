// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkillProjectGameMode.h"
#include "Character/SkillProjectCharacter.h"
#include "SkillProjectPlayerController.h"
#include "UObject/ConstructorHelpers.h"

ASkillProjectGameMode::ASkillProjectGameMode()
{
	static ConstructorHelpers::FClassFinder<APawn> BPPawnClass(TEXT("/Game/ThirdPerson/Blueprints/BP_SkillProjectCharacter"));
	if (BPPawnClass.Class != NULL)
	{
		DefaultPawnClass = BPPawnClass.Class;
	}

	static ConstructorHelpers::FClassFinder<APlayerController> BPPlayerController(TEXT("/Game/ThirdPerson/Input/BP_CharacterController"));
	if (BPPlayerController.Class != NULL)
	{
		PlayerControllerClass = BPPlayerController.Class;
	}
}
