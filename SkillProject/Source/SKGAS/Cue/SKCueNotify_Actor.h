// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"

#include "SKCueNotify_Actor.generated.h"

UCLASS()
class SKGAS_API ASKCueNotify_Actor : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()
	
public:
	ASKCueNotify_Actor();

protected:
	virtual void BeginPlay() override;

public:
	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual void GameplayCueFinishedCallback() override;
	virtual bool Recycle() override;

public:
	UFUNCTION()
	void OnParticleSystemFinished(UParticleSystemComponent* FinishedComponent);

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UParticleSystemComponent* ParticleSystemComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector ParticleOffset;
};
