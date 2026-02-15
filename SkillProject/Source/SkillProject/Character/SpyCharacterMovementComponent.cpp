// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/SpyCharacterMovementComponent.h"
#include "Util/DefineEnum.h"
#include "Character/SpyCharacter.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Util/SpyGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyCharacterMovementComponent)

USpyCharacterMovementComponent::USpyCharacterMovementComponent()
{
	SetIsReplicatedByDefault(true);

	bHangUp = false;
}

void USpyCharacterMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(USpyCharacterMovementComponent, SpyInputVector, COND_SkipOwner);
	DOREPLIFETIME(USpyCharacterMovementComponent, ClimbData);
	DOREPLIFETIME(USpyCharacterMovementComponent, ClimbWallData);
}

void USpyCharacterMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	switch (CustomMovementMode)
	{
		case ECustomMovementMode::MOVE_WallClimb:
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
		case ECustomMovementMode::MOVE_WallClimb:
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

void USpyCharacterMovementComponent::PhysWallClimb(float DeltaTime, int32 Iterations)
{
	if (CharacterOwner == nullptr || UpdatedComponent == nullptr)
		return;

	//# 벽타기 스피드 설정
	Velocity = GetWallClimbSpeed();

	//# 벽타기 진행 중 위치 계산
	FHitResult Hit;
	SafeMoveUpdatedComponent(Velocity * DeltaTime, UpdatedComponent->GetComponentRotation(), true, Hit);

	//# 한 번만 호출하도록
	if (CanHangUp() && bHangUp == false)
	{
		bHangUp = true;

		FGameplayEventData EventData;
		EventData.Instigator = GetOwner();

		FVector OwnerLocation = GetOwner()->GetActorLocation();
		FVector OwnerFowardVector = GetOwner()->GetActorForwardVector();

		UWorld* World = GetWorld();
		FVector Start = OwnerLocation + OwnerFowardVector * (ClimbData.DistanceOffset + 50.f) + FVector::UpVector * 200.f;
		FVector End = Start + FVector::DownVector * 500.f;
		FHitResult HitResult;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(GetOwner());

		bool bHit = World->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, Params);
		DrawDebugLine(World, Start, End, bHit ? FColor::Green : FColor::Red, 
false, 1.f, 0, -1.f);

		FGameplayAbilityTargetData_LocationInfo* LocData = new FGameplayAbilityTargetData_LocationInfo();
		LocData->TargetLocation.LiteralTransform = FTransform(HitResult.Location);
		LocData->TargetLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;

		EventData.TargetData.Add(LocData);

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwner(), SpyGameplayTags::Skill_Move_HangUp, EventData);
	}

	CalculateBoneVectorOffset(TEXT("hand_l"), TEXT("Hand_L_IK_Weight"), CurrentOffsetHL, DeltaTime, ClimbData.HandOffset);
	CalculateBoneVectorOffset(TEXT("hand_r"), TEXT("Hand_R_IK_Weight"), CurrentOffsetHR, DeltaTime, ClimbData.HandOffset);
	CalculateBoneVectorOffset(TEXT("foot_l"), TEXT("Foot_L_IK_Weight"), CurrentOffsetFL, DeltaTime, ClimbData.FootOffset);
	CalculateBoneVectorOffset(TEXT("foot_r"), TEXT("Foot_R_IK_Weight"), CurrentOffsetFR, DeltaTime, ClimbData.FootOffset);
}

void USpyCharacterMovementComponent::StartWallClimb(const FClimbData& InClimbData, const FClimbWallData& InClimbWallData)
{
	UE_LOG(LogTemp, Warning, TEXT("StartWallClimb %s"), *GetOwner()->GetName());

	ClimbData = InClimbData;
	ClimbWallData = InClimbWallData;

	bHangUp = false;

	SetMovementMode(MOVE_Custom, (uint8)ECustomMovementMode::MOVE_WallClimb);

	GravityScale = 0.0f;
	bOrientRotationToMovement = false;
	Velocity = FVector::ZeroVector;

	FVector TargetLocation = ClimbWallData.HitVector + (ClimbWallData.NormalVector * ClimbData.DistanceOffset);
	FRotator TargetRotator = (-ClimbWallData.NormalVector).Rotation();
	TargetRotator.Pitch = 0.f;
	TargetRotator.Roll = 0.f;

	UpdatedComponent->SetWorldLocationAndRotation(TargetLocation, TargetRotator, false, nullptr, ETeleportType::TeleportPhysics);
}

void USpyCharacterMovementComponent::EndWallClimb()
{
	UE_LOG(LogTemp, Warning, TEXT("EndWallClimb %s"), *GetOwner()->GetName());

	GravityScale = 1.0f;
	bOrientRotationToMovement = true;

	SetMovementMode(MOVE_Walking);
}

bool USpyCharacterMovementComponent::CanHangUp()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter == nullptr)
		return false;

	FVector OwnerLocation = OwnerCharacter->GetActorLocation();
	FVector OwnerFowardVector = OwnerCharacter->GetActorForwardVector();

	UWorld* World = GetWorld();
	FVector Start = OwnerLocation + (FVector::UpVector * ClimbData.CheckHangUpHeight);
	FVector End = Start + OwnerFowardVector * (ClimbData.DistanceOffset + 100.f);
	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerCharacter);

	bool bHit = World->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, Params);
	DrawDebugLine(World, Start, End, bHit ? FColor::Green : FColor::Red, false, 1.f, 0, -1.f);

	return bHit == false;
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

FVector USpyCharacterMovementComponent::CalculateBoneVectorOffset(FName BoneName, FName CurveName, FVector& CurrentOffsetVar, float DeltaTime, float Offset)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter == nullptr)
		return FVector::ZeroVector;

	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	if (Mesh == nullptr)
		return FVector::ZeroVector;

	UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
	if (AnimInstance == nullptr)
		return FVector::ZeroVector;

	float IKWeight = AnimInstance->GetCurveValue(CurveName);
	FVector AnimBoneLocation = Mesh->GetSocketLocation(BoneName);

	FVector WallPoint = ClimbWallData.HitVector;
	FVector WallNormal = ClimbWallData.NormalVector.GetSafeNormal();

	//# 벽 평면까지의 거리 계산
	float DistanceFromPlane = FVector::DotProduct(AnimBoneLocation - WallPoint, WallNormal);

	//# 목표 오프셋 (벽 법선 기준으로 이동)
	FVector TargetWorldOffset = (-WallNormal * (DistanceFromPlane - Offset)) * IKWeight;

	//# 월드 -> 메쉬 공간으로 좌표 변환
	FVector TargetMeshOffset = Mesh->GetComponentTransform().InverseTransformVectorNoScale(TargetWorldOffset);

	CurrentOffsetVar = FMath::VInterpTo(CurrentOffsetVar, TargetMeshOffset, DeltaTime, InterpSpeed);

	return CurrentOffsetVar;
}

FVector USpyCharacterMovementComponent::GetWallClimbSpeed()
{
	const FVector WallNormal = ClimbWallData.NormalVector;
	const FVector UpVector = FVector::UpVector;

	//# 외적을 통해 벽의 위쪽 오른쪽 벡터 Get
	FVector WallRight = FVector::CrossProduct(UpVector, WallNormal).GetSafeNormal();
	FVector WallUp = FVector::CrossProduct(WallNormal, WallRight).GetSafeNormal();

	FVector Speed = WallUp * SpyInputVector.Y * ClimbData.Speed +
		WallRight * -SpyInputVector.X * ClimbData.Speed;

	return Speed;
}

void USpyCharacterMovementComponent::SetWallClimbInput(FVector2D InputVector)
{
	SpyInputVector = InputVector;

	if (PawnOwner && PawnOwner->IsLocallyControlled())
	{
		Server_SetWallClimbInput(InputVector);
	}
}

void USpyCharacterMovementComponent::Server_SetWallClimbInput_Implementation(FVector2D InputVector)
{
	SpyInputVector = InputVector;
}

