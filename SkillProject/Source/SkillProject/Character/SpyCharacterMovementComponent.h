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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;
	virtual void PhysicsRotation(float DeltaTime) override;

public:
	void PhysWallClimb(float DeltaTime, int32 Iterations);
	void StartWallClimb(const FClimbData& InClimbData, const FClimbWallData& InClimbWallData);
	void EndWallClimb();

	float GetInputAngleByForward();
	float GetClosestLadderHeight(float CurrentHeight);
	float CalculateBoneOffset(FName BoneName, float& CurrentOffsetVar, float DeltaTime);

	FVector GetWallClimbSpeed();
	void SetWallClimbInput(FVector2D InputVector);

	UFUNCTION(Server, Unreliable)
    void Server_SetWallClimbInput(FVector2D InputVector);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float InterpSpeed;

	UPROPERTY(Replicated)
	FVector2D SpyInputVector;

	UPROPERTY(Replicated)
	FClimbData ClimbData;

	UPROPERTY(Replicated)
	FClimbWallData ClimbWallData;

public:
	//# 이전 프레임의 IK 도달 지점을 저장 (떨림 방지용 보간 타겟)
	float ZOffsetHL;
	float ZOffsetHR;
	float ZOffsetFL;
	float ZOffsetFR;
};
