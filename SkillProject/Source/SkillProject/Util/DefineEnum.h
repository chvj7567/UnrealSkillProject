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
	Dialogue UMETA(DisplayName = "Dialogue"),
	MissionOffer UMETA(DisplayName = "MissionOffer"),
	InteractPrompt UMETA(DisplayName = "InteractPrompt"),
	QuitConfirm UMETA(DisplayName = "QuitConfirm"),
};

UENUM(BlueprintType)
enum ECustomMovementMode : uint8
{
    MOVE_Default       UMETA(DisplayName = "Default"),
    MOVE_WallClimb     UMETA(DisplayName = "Wall Climb"),
};

//# 미션 1개의 수락 방식을 가른다. Gameplay는 NPC Offer 카드로 수동 수락,
//# Dialogue/Interact는 배열 진입과 동시에 자동 수락된다(카드를 보여줄 주체가 없다)
UENUM(BlueprintType)
enum class ESpyMissionType : uint8
{
	Gameplay,
	Dialogue,
	Interact,
};

//# NPC 상호작용 대사 상태 4종. Locked/Completed 구분을 두지 않는다 —
//# 둘 다 "현재 미션이 이 NPC와 무관함"으로 통합된다 (spec §6)
UENUM(BlueprintType)
enum class ESpyNPCDialogueState : uint8
{
	Default,
	Offer,
	InProgress,
	Report,
};