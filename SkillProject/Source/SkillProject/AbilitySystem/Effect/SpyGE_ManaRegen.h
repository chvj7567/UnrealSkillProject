// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"

#include "SpyGE_ManaRegen.generated.h"

//# 마나 시간 재생 — 무한 지속 + 주기(2초)마다 Mana +3 (= 1.5/초). 상한 클램프는 USKAttributeSet 이 담당
UCLASS()
class SKILLPROJECT_API USpyGE_ManaRegen : public UGameplayEffect
{
	GENERATED_BODY()

public:
	USpyGE_ManaRegen();
};
