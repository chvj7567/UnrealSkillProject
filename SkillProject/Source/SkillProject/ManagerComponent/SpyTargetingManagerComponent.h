// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ManagerComponent/CommonInterface.Manager.h"
#include "SpyTargetingManagerComponent.generated.h"

class ASpyCharacter;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SKILLPROJECT_API USpyTargetingManagerComponent : public UActorComponent, public ISpyTargetProvider
{
	GENERATED_BODY()

public:
	USpyTargetingManagerComponent();

	//# ISpyTargetProvider
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	virtual void SetCurrentTarget(AActor* NewTarget) override;

	virtual TWeakObjectPtr<AActor> GetTarget() const override
	{
		return CurrentTarget.Get();
	}

	UFUNCTION(BlueprintCallable, Category = "Targeting")
	virtual bool IsTargetValid() const override;

	UFUNCTION(BlueprintCallable, Category = "Targeting")
	virtual bool FindTarget(float Radius) override;

	UFUNCTION(BlueprintCallable, Category = "Targeting")
	void OnTargetDeath(AActor* InOwningActor, AActor* InCauserActor);

protected:
	bool IsPotentialTargetValid(AActor* PotentialTarget) const;

protected:
	UPROPERTY(Replicated)
	TObjectPtr<AActor> CurrentTarget;
};
