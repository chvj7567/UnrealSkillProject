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

void USpyCharacterMovementComponent::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
}

void USpyCharacterMovementComponent::PhysWallClimb(float DeltaTime, int32 Iterations)
{
	if (!CharacterOwner || !UpdatedComponent)
		return;

	const FVector WallNormal = ClimbWallData.NormalVector;
	const FVector UpVector = FVector::UpVector;

	FVector WallRight = FVector::CrossProduct(UpVector, WallNormal).GetSafeNormal();
	FVector WallUp = FVector::CrossProduct(WallNormal, WallRight).GetSafeNormal();

	const float ClimbSpeed = 30.f;
	FVector DesiredVelocity =
		WallUp * SpyInputVector.Y * ClimbSpeed +
		WallRight * -SpyInputVector.X * ClimbSpeed;
	
	Velocity = DesiredVelocity;

	FVector Delta = Velocity * DeltaTime;

	FHitResult Hit;
	SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentRotation(), true, Hit);
}

void USpyCharacterMovementComponent::StartWallClimb(const FClimbData& InClimbData, const FClimbWallData& InClimbWallData)
{
	UE_LOG(LogTemp, Warning, TEXT("StartWallClimb"));

	ClimbData = InClimbData;
	ClimbWallData = InClimbWallData;

	SetMovementMode(MOVE_Custom, (uint8)ECustomMovementMode::MOVE_WallClimb);

	GravityScale = 0.0f;
	bOrientRotationToMovement = false;
	Velocity = FVector::ZeroVector;

	FVector TargetLocation = ClimbWallData.HitVector + (ClimbWallData.NormalVector * ClimbData.DistanceOffset);
	FRotator TargetRotator = (-ClimbWallData.NormalVector).Rotation();
	TargetRotator.Pitch = 0.f;
	TargetRotator.Roll = 0.f;

	CharacterOwner->SetActorLocation(TargetLocation);
	CharacterOwner->SetActorRotation(TargetRotator);
}

void USpyCharacterMovementComponent::EndWallClimb()
{
	UE_LOG(LogTemp, Warning, TEXT("EndWallClimb"));

	GravityScale = 1.0f;
	bOrientRotationToMovement = true;
	SetMovementMode(MOVE_Walking);
}

float USpyCharacterMovementComponent::GetInputAngleByForward()
{
	float Angle = FMath::Atan2(SpyInputVector.X, SpyInputVector.Y);
	return FMath::RadiansToDegrees(Angle);
}
