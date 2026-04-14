// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpyTargetingManagerComponent.generated.h"

class ASpyCharacter;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SKILLPROJECT_API USpyTargetingManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USpyTargetingManagerComponent();

	UFUNCTION(BlueprintCallable, Category = "Targeting")
	void SetCurrentTarget(AActor* NewTarget);

	TWeakObjectPtr<AActor> GetTarget() const { return CurrentTarget.Get(); }

	UFUNCTION(BlueprintCallable, Category = "Targeting")
	bool IsTargetValid() const;

	UFUNCTION(BlueprintCallable, Category = "Targeting")
	bool FindTarget(float Radius);

	UFUNCTION(BlueprintCallable, Category = "Targeting")
	void OnTargetDeath(AActor* InOwningActor, AActor* InCauserActor);

protected:
	bool IsPotentialTargetValid(AActor* PotentialTarget) const;

protected:
	UPROPERTY(Replicated)
	TObjectPtr<AActor> CurrentTarget;
};
