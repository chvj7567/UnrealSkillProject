// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "SpyCharacterMovementComponent.generated.h"

UCLASS()
class SKILLPROJECT_API USpyCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
public:
	USpyCharacterMovementComponent();

	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;

public:
	FORCEINLINE float GetInputDirection()
	{
		float Angle = FMath::Atan2(SpyInputVector.Y, SpyInputVector.X);
		return FMath::RadiansToDegrees(Angle);
	}

	FORCEINLINE void SetInputVector(FVector2D InInputVector) { SpyInputVector = InInputVector; }

public:
	void PhysWallClimb(float DeltaTime, int32 Iterations);
	void StartWallClimb(const FVector& WallNormal);
	void StopWallClimb();

protected:
	FVector2D SpyInputVector;
	FVector CachedWallNormal;
};
