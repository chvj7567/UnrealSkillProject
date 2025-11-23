// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "SpyParkourManagerComponent.generated.h"

class UCharacterMovementComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SKILLPROJECT_API USpyParkourManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USpyParkourManagerComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	FORCEINLINE bool IsWallClimbing() const { return bIsWallClimbing; }
	FORCEINLINE FVector GetHitNormalVector() const { return HitNormalVector; }

public:
	void CheckAbleWallClimbing();

protected:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

private:
	bool bIsWallClimbing;
	FVector HitNormalVector;
};
