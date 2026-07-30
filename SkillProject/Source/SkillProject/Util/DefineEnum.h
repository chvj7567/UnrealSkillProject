// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DefineEnum.generated.h"

UENUM(BlueprintType)
enum ESpyUIType : uint8
{
	None UMETA(DisplayName = "None"),
	MainHUD UMETA(DisplayName = "MainHUD"),
	HpBar UMETA(DisplayName = "HpBar"),
	Loading UMETA(DisplayName = "Loading"),
	SessionBrowser UMETA(DisplayName = "SessionBrowser"),
};

UENUM(BlueprintType)
enum ECustomMovementMode : uint8
{
    MOVE_Default       UMETA(DisplayName = "Default"),
    MOVE_WallClimb     UMETA(DisplayName = "Wall Climb"),
};