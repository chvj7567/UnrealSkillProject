// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DefineEnum.generated.h"

UENUM(BlueprintType)
enum class ESpyUIType : uint8
{
    None,
    MainHUD         UMETA(DisplayName = "MainHUD"),
};