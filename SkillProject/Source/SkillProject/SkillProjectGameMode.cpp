// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkillProjectGameMode.h"
#include "SkillProjectCharacter.h"
#include "UObject/ConstructorHelpers.h"

ASkillProjectGameMode::ASkillProjectGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
