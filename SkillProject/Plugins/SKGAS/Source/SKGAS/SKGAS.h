// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FSKGASModule : public IModuleInterface
{
public:

	//# IModuleInterface 구현
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
