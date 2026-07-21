// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"

#include "SpyGE_ExperienceGain.generated.h"

//# 경험치 가산 Instant 이펙트 — 매그니튜드는 SetByCaller(Data.Experience.Gain)로 전달한다
UCLASS()
class SKILLPROJECT_API USpyGE_ExperienceGain : public UGameplayEffect
{
	GENERATED_BODY()

public:
	USpyGE_ExperienceGain();
};
