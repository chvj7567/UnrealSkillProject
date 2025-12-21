// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"

#include "SpyCueNotify_Static.generated.h"

UCLASS()
class SKILLPROJECT_API USpyCueNotify_Static : public UGameplayCueNotify_Static
{
	GENERATED_BODY()
	
public:
	USpyCueNotify_Static();

public:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

private:
	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> AnimMontage;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> Particle;
};
