// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/SpyCharacterMovementComponent.h"
#include "Util/DefineEnum.h"
#include "Character/SpyCharacter.h"

USpyCharacterMovementComponent::USpyCharacterMovementComponent()
{
}

void USpyCharacterMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	Super::PhysCustom(DeltaTime, Iterations);

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

	SpyInputVector = FVector2D::ZeroVector;
}

void USpyCharacterMovementComponent::PhysWallClimb(float DeltaTime, int32 Iterations)
{
	if (!CharacterOwner || !UpdatedComponent)
	{
		return;
	}

	// 벽 표면 기준 방향 벡터 계산
	const FVector WallNormal = CachedWallNormal;
	const FVector UpVector = FVector::UpVector;

	FVector WallRight = FVector::CrossProduct(UpVector, WallNormal).GetSafeNormal();

	FVector WallUp = FVector::CrossProduct(WallNormal, WallRight).GetSafeNormal();

	const float ClimbSpeed = 300.f;
	FVector DesiredVelocity =
		WallUp * SpyInputVector.Y * ClimbSpeed +
		WallRight * -SpyInputVector.X * ClimbSpeed;

	Velocity = DesiredVelocity;

	UE_LOG(LogTemp, Warning, TEXT("ConsumeInputVector: X=%.2f, Y=%.2f / Velocity X=%.2f, Y=%.2f, Z=%.2f"), SpyInputVector.X, SpyInputVector.Y, Velocity.X, Velocity.Y, Velocity.Z);

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
	Velocity = FVector::ZeroVector;
}

void USpyCharacterMovementComponent::StopWallClimb()
{
	UE_LOG(LogTemp, Warning, TEXT("StopWallClimb"));

	GravityScale = 1.0f;
	SetMovementMode(MOVE_Walking);
}
