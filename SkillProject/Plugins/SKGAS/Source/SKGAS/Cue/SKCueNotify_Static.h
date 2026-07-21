// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"

#include "SKCueNotify_Static.generated.h"

UCLASS()
class SKGAS_API USKCueNotify_Static : public UGameplayCueNotify_Static
{
	GENERATED_BODY()
	
public:
	USKCueNotify_Static();

public:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

private:
	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> NormalParticle;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> CriticalParticle;
};
