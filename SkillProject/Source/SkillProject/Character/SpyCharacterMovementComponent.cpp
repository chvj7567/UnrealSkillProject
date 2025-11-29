// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/SpyCharacterMovementComponent.h"
#include "Util/DefineEnum.h"
#include "Character/SpyCharacter.h"

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

	const FVector WallNormal = CachedWallNormal;
	const FVector UpVector = FVector::UpVector;

	FVector WallRight = FVector::CrossProduct(UpVector, WallNormal).GetSafeNormal();
	FVector WallUp = FVector::CrossProduct(WallNormal, WallRight).GetSafeNormal();

	const float ClimbSpeed = 100.f;
	FVector DesiredVelocity =
		WallUp * SpyInputVector.Y * ClimbSpeed +
		WallRight * -SpyInputVector.X * ClimbSpeed;
	
	Velocity = DesiredVelocity;

	FVector Delta = Velocity * DeltaTime;

	FHitResult Hit;
	SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentRotation(), true, Hit);
}

void USpyCharacterMovementComponent::StartWallClimb(const FVector& WallNormal)
{
	UE_LOG(LogTemp, Warning, TEXT("StartWallClimb"));

	CachedWallNormal = WallNormal;

	SetMovementMode(MOVE_Custom, (uint8)ECustomMovementMode::MOVE_WallClimb);

	GravityScale = 0.0f;
	bOrientRotationToMovement = false;
	Velocity = FVector::ZeroVector;

	FVector ForwardDir = -WallNormal;
	FRotator TargetRot = ForwardDir.Rotation();
	TargetRot.Pitch = 0.f;
	TargetRot.Roll = 0.f;

	CharacterOwner->SetActorRotation(TargetRot);
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
