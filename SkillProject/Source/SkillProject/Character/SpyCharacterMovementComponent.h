// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"

#include "SpyCharacterMovementComponent.generated.h"

struct FClimbWallData;

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
	void StartWallClimb(const FClimbData& InClimbData, const FClimbWallData& ClimbWallData);
	void EndWallClimb();

	float GetInputAngleByForward();
	float GetClosestLadderHeight(float CurrentHeight);
	float CalculateBoneOffset(FName BoneName, float& CurrentOffsetVar, float DeltaTime);

protected:
	FVector2D SpyInputVector;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float InterpSpeed;

	FClimbData ClimbData;
	FClimbWallData ClimbWallData;

public:
	//# 이전 프레임의 IK 도달 지점을 저장 (떨림 방지용 보간 타겟)
	float ZOffsetHL;
	float ZOffsetHR;
	float ZOffsetFL;
	float ZOffsetFR;
};
