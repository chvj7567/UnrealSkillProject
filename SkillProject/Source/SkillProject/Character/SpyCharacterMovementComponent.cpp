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
	float Angle = FMath::RadiansToDegrees(FMath::Atan2(SpyInputVector.X, SpyInputVector.Y));
	return Angle;
}

float USpyCharacterMovementComponent::GetClosestLadderHeight(float CurrentHeight)
{
	TArray<float> Ladder = { 100, 150, 200, 250, 300, 350, 400, 450, 500 };

	// 배열이 비어있는지 확인
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

	FVector AnimBoneLoc = OwnerCharacter->GetMesh()->GetSocketLocation(BoneName);
	float TargetZ = GetClosestLadderHeight(AnimBoneLoc.Z);
	float RawOffset = TargetZ - AnimBoneLoc.Z;
	CurrentOffsetVar = FMath::FInterpTo(CurrentOffsetVar, RawOffset, DeltaTime, InterpSpeed);

	return CurrentOffsetVar;
}