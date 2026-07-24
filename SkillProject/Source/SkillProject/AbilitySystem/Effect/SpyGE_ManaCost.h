// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"

#include "SpyGE_ManaCost.generated.h"

//# 마나 감산 Instant 이펙트 — 감산량은 SetByCaller(Data.Cost.Mana)로 전달한다(음수 부호는 호출부 책임)
UCLASS()
class SKILLPROJECT_API USpyGE_ManaCost : public UGameplayEffect
{
	GENERATED_BODY()

public:
	USpyGE_ManaCost();
};
