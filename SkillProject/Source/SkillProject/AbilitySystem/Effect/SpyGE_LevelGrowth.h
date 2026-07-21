// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"

#include "SpyGE_LevelGrowth.generated.h"

//# 레벨업 성장 Instant 이펙트 — MaxHealth/MaxMana 증가량을 SetByCaller로 전달한다
UCLASS()
class SKILLPROJECT_API USpyGE_LevelGrowth : public UGameplayEffect
{
	GENERATED_BODY()

public:
	USpyGE_LevelGrowth();
};
