// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/SpyCharacterMovementComponent.h"
#include "Util/DefineEnum.h"
#include "Character/SpyCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyCharacterMovementComponent)

USpyCharacterMovementComponent::USpyCharacterMovementComponent()
{
}

void USpyCharacterMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	switch (CustomMovementMode)
	{
	case (uint8)ECustomMovementMode::MOVE_WallClimb:
	{
		PhysWallClimb(DeltaTime, Iterations);
	}
	break;
	default:
	{
		Super::PhysCustom(DeltaTime, Iterations);
	}
	break;
	}
}

void USpyCharacterMovementComponent::PhysicsRotation(float DeltaTime)
{
	switch (CustomMovementMode)
	{
	case (uint8)ECustomMovementMode::MOVE_WallClimb:
	{
		return;
	}
	break;
	default:
	{
		Super::PhysicsRotation(DeltaTime);
	}
	break;
	}
}

void USpyCharacterMovementComponent::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	const UEnum* ModeEnum = StaticEnum<EMovementMode>();

	FString PrevModeName = ModeEnum->GetValueAsString(PrevMovementMode);
	FString CurrentModeName = ModeEnum->GetValueAsString(MovementMode);

	//UE_LOG(LogTemp, Warning, TEXT("Movement Mode Changed!"));
	//UE_LOG(LogTemp, Log, TEXT("Before: %s (Custom: %d) %s"), *PrevModeName, PreviousCustomMode, *GetOwner()->GetName());
	//UE_LOG(LogTemp, Log, TEXT("After:  %s (Custom: %d) %s"), *CurrentModeName, CustomMovementMode, *GetOwner()->GetName());
}

void USpyCharacterMovementComponent::PhysWallClimb(float DeltaTime, int32 Iterations)
{
	if (CharacterOwner == nullptr || UpdatedComponent == nullptr)
		return;

	const FVector WallNormal = ClimbData.WallData.NormalVector;
	const FVector UpVector = FVector::UpVector;

	FVector WallRight = FVector::CrossProduct(UpVector, WallNormal).GetSafeNormal();
	FVector WallUp = FVector::CrossProduct(WallNormal, WallRight).GetSafeNormal();
;
	FVector DesiredVelocity =
		WallUp * SpyInputVector.Y * ClimbData.Speed +
		WallRight * -SpyInputVector.X * ClimbData.Speed;
	
	Velocity = DesiredVelocity;

	FVector Delta = Velocity * DeltaTime;
	FHitResult Hit;
	SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentRotation(), true, Hit);

	CalculateBoneOffset(TEXT("hand_l"), ZOffsetHL, DeltaTime);
	CalculateBoneOffset(TEXT("hand_r"), ZOffsetHR, DeltaTime);
	CalculateBoneOffset(TEXT("foot_l"), ZOffsetFL, DeltaTime);
	CalculateBoneOffset(TEXT("foot_r"), ZOffsetFR, DeltaTime);
}

void USpyCharacterMovementComponent::StartWallClimb(const FClimbData& InClimbData)
{
	UE_LOG(LogTemp, Warning, TEXT("StartWallClimb %s"), *GetOwner()->GetName());

	ClimbData = InClimbData;

	SetMovementMode(MOVE_Custom, (uint8)ECustomMovementMode::MOVE_WallClimb);

	CharacterOwner->bUseControllerRotationYaw = false;

	GravityScale = 0.0f;
	bUseControllerDesiredRotation = false;
	bOrientRotationToMovement = false;
	Velocity = FVector::ZeroVector;

	FVector TargetLocation = ClimbData.WallData.HitVector + (ClimbData.WallData.NormalVector * ClimbData.DistanceOffset);
	FRotator TargetRotator = (-ClimbData.WallData.NormalVector).Rotation();
	TargetRotator.Pitch = 0.f;
	TargetRotator.Roll = 0.f;

	UpdatedComponent->SetWorldLocationAndRotation(TargetLocation, TargetRotator, false, nullptr, ETeleportType::TeleportPhysics);

	bForceNextFloorCheck = false;

	if (GetOwner()->HasAuthority())
	{
		CurrentFloor.Clear();
	}
}

void USpyCharacterMovementComponent::EndWallClimb()
{
	UE_LOG(LogTemp, Warning, TEXT("EndWallClimb %s"), *GetOwner()->GetName());

	GravityScale = 1.0f;
	bOrientRotationToMovement = true;
	SetMovementMode(MOVE_Walking);
}

float USpyCharacterMovementComponent::GetInputAngleByForward()
{
	float Angle = FMath::RadiansToDegrees(FMath::Atan2(SpyInputVector.X, SpyInputVector.Y));
	return Angle;
}

float USpyCharacterMovementComponent::GetClosestLadderHeight(float CurrentHeight)
{
	//# Temp
	TArray<float> Ladder = { 100, 150, 200, 250, 300, 350, 400, 450, 500 };

	if (Ladder.Num() == 0) return 0.0f;

	float ClosestValue = Ladder[0];
	float MinDiff = FMath::Abs(CurrentHeight - Ladder[0]);

	for (int32 i = 1; i < Ladder.Num(); i++)
	{
		float CurrentDiff = FMath::Abs(CurrentHeight - Ladder[i]);

		if (CurrentDiff < MinDiff)
		{
			MinDiff = CurrentDiff;
			ClosestValue = Ladder[i];
		}
	}

	return ClosestValue;
}

float USpyCharacterMovementComponent::CalculateBoneOffset(FName BoneName, float& CurrentOffsetVar, float DeltaTime)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter == nullptr)
		return 0.f;

	FVector AnimBoneLocation = OwnerCharacter->GetMesh()->GetSocketLocation(BoneName);
	float TargetZ = GetClosestLadderHeight(AnimBoneLocation.Z);
	float RawOffset = TargetZ - AnimBoneLocation.Z;

	CurrentOffsetVar = FMath::FInterpTo(CurrentOffsetVar, RawOffset, DeltaTime, InterpSpeed);

	return CurrentOffsetVar;
}