// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"

#include "SpyExecCalculation.generated.h"

UCLASS()
class SKILLPROJECT_API USpyExecCalculation : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
	
public:
	USpyExecCalculation();

public:
	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

protected:
	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> EffectClass;
};
