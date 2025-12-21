// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DefineEnum.generated.h"

UENUM(BlueprintType)
enum class ESpyUIType : uint8
{
    None            UMETA(DisplayName = "None"),
    MainHUD         UMETA(DisplayName = "MainHUD"),
    HpBar           UMETA(DisplayName = "HpBar"),
    Menu            UMETA(DisplayName = "Menu"),
};

UENUM(BlueprintType, meta = (Bitflags))
enum class ESpyPlayerStateFlags : uint8
{
    None = 0,
    IsAlive = 1 << 0,
    IsClimb = 1 << 1,
};
ENUM_CLASS_FLAGS(ESpyPlayerStateFlags);

UENUM(BlueprintType)
enum class ECustomMovementMode : uint8
{
    MOVE_Default       UMETA(DisplayName = "Default"),
    MOVE_WallClimb     UMETA(DisplayName = "Wall Climb"),
};

UENUM(BlueprintType)
enum class ESpyCharacterType : uint8
{
    Normal         UMETA(DisplayName = "Normal"),
};

UENUM(BlueprintType)
enum class ESpySkillType : uint8
{
    SkillA         UMETA(DisplayName = "SkillA"),
    SkillB         UMETA(DisplayName = "SkillB"),
    SkillC         UMETA(DisplayName = "SkillC"),
    SkillD         UMETA(DisplayName = "SkillD"),
    SkillE         UMETA(DisplayName = "SkillE"),
};