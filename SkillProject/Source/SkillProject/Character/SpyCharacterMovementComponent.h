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
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;

public:
	FORCEINLINE void SetInputVector(FVector2D InInputVector) { SpyInputVector = InInputVector; }

public:
	void PhysWallClimb(float DeltaTime, int32 Iterations);
	void StartWallClimb(const FVector& WallNormal);
	void EndWallClimb();

	float GetInputAngleByForward();

protected:
	FVector2D SpyInputVector;
	FVector CachedWallNormal;
};
