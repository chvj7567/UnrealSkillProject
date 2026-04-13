// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/SpyCharacterMovementComponent.h"
#include "Util/DefineEnum.h"
#include "Character/SpyCharacter.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Util/SpyGameplayTags.h"
#include "ManagerComponent/SpyTargetingManagerComponent.h"
#include "SKGameplayTags.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "Data/SpyMovementConfig.h"

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
			ASpyCharacter* OwnerCharacter = Cast<ASpyCharacter>(GetOwner());
			if (OwnerCharacter == nullptr)
				break;

			USpyTargetingManagerComponent* TargetingComp = OwnerCharacter->FindComponentByClass<USpyTargetingManagerComponent>();
			if (TargetingComp == nullptr)
				break;

			if (USpyAbilitySystemComponent* ASC = OwnerCharacter->GetSpyAbilitySystemComponent())
			{
				if (ASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Death))
					break;
			}

			if (TargetingComp->GetTarget().IsValid())
			{
				FVector LookDir = TargetingComp->GetTarget()->GetActorLocation() - OwnerCharacter->GetActorLocation();
				LookDir.Z = 0.f;
				FRotator TargetRot = LookDir.Rotation();

				OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
				OwnerCharacter->SetActorRotation(TargetRot);
				break;
			}
			else
			{
				OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = true;
			}

			Super::PhysicsRotation(DeltaTime);
		}
		break;
	}
}

void USpyCharacterMovementComponent::PhysWallClimb(float DeltaTime, int32 Iterations)
{
	if (CharacterOwner == nullptr || UpdatedComponent == nullptr)
		return;

	//# ��Ÿ�� ���ǵ� ����
	Velocity = GetWallClimbSpeed();

	//# ��Ÿ�� ���� �� ��ġ ���
	FHitResult Hit;
	SafeMoveUpdatedComponent(Velocity * DeltaTime, UpdatedComponent->GetComponentRotation(), true, Hit);

	//# �� ���� ȣ���ϵ���
	if (CanHangUp() && bHangUp == false)
	{
		bHangUp = true;

		FGameplayEventData EventData;
		EventData.Instigator = GetOwner();

		FVector OwnerLocation = GetOwner()->GetActorLocation();
		FVector OwnerFowardVector = GetOwner()->GetActorForwardVector();

		UWorld* World = GetWorld();
		const float HangUpFwdOffset = MovementConfig ? MovementConfig->ClimbHangUpRayForwardOffset : 50.f;
		const float HangUpUpOffset  = MovementConfig ? MovementConfig->ClimbHangUpRayUpOffset     : 200.f;
		const float HangUpDownDist  = MovementConfig ? MovementConfig->ClimbHangUpRayDownDistance : 500.f;
		FVector Start = OwnerLocation + OwnerFowardVector * (ClimbData.DistanceOffset + HangUpFwdOffset) + FVector::UpVector * HangUpUpOffset;
		FVector End = Start + FVector::DownVector * HangUpDownDist;
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

	const FName BoneHL  = MovementConfig ? MovementConfig->IKBoneHandLeft    : FName("hand_l");
	const FName BoneHR  = MovementConfig ? MovementConfig->IKBoneHandRight   : FName("hand_r");
	const FName BoneFL  = MovementConfig ? MovementConfig->IKBoneFootLeft    : FName("foot_l");
	const FName BoneFR  = MovementConfig ? MovementConfig->IKBoneFootRight   : FName("foot_r");
	const FName CurveHL = MovementConfig ? MovementConfig->IKCurveHandLeft   : FName("Hand_L_IK_Weight");
	const FName CurveHR = MovementConfig ? MovementConfig->IKCurveHandRight  : FName("Hand_R_IK_Weight");
	const FName CurveFL = MovementConfig ? MovementConfig->IKCurveFootLeft   : FName("Foot_L_IK_Weight");
	const FName CurveFR = MovementConfig ? MovementConfig->IKCurveFootRight  : FName("Foot_R_IK_Weight");
	CalculateBoneVectorOffset(BoneHL, CurveHL, CurrentOffsetHL, DeltaTime, ClimbData.HandOffset);
	CalculateBoneVectorOffset(BoneHR, CurveHR, CurrentOffsetHR, DeltaTime, ClimbData.HandOffset);
	CalculateBoneVectorOffset(BoneFL, CurveFL, CurrentOffsetFL, DeltaTime, ClimbData.FootOffset);
	CalculateBoneVectorOffset(BoneFR, CurveFR, CurrentOffsetFR, DeltaTime, ClimbData.FootOffset);
}

void USpyCharacterMovementComponent::StartWallClimb(const FClimbData& InClimbData, const FClimbWallData& InClimbWallData)
{
	UE_LOG(LogTemp, Warning, TEXT("StartWallClimb %s"), *GetOwner()->GetName());

	ClimbData = InClimbData;
	ClimbWallData = InClimbWallData;

	bHangUp = false;

	SetMovementMode(MOVE_Custom, (uint8)ECustomMovementMode::MOVE_WallClimb);

	CachedGravityScale = GravityScale;
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

	GravityScale = CachedGravityScale;
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
	const float CheckFwdOffset = MovementConfig ? MovementConfig->HangUpCheckRayForwardOffset : 100.f;
	FVector End = Start + OwnerFowardVector * (ClimbData.DistanceOffset + CheckFwdOffset);
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

	//# �� �������� �Ÿ� ���
	float DistanceFromPlane = FVector::DotProduct(AnimBoneLocation - WallPoint, WallNormal);

	//# ��ǥ ������ (�� ���� �������� �̵�)
	FVector TargetWorldOffset = (-WallNormal * (DistanceFromPlane - Offset)) * IKWeight;

	//# ���� -> �޽� �������� ��ǥ ��ȯ
	FVector TargetMeshOffset = Mesh->GetComponentTransform().InverseTransformVectorNoScale(TargetWorldOffset);

	CurrentOffsetVar = FMath::VInterpTo(CurrentOffsetVar, TargetMeshOffset, DeltaTime, InterpSpeed);

	return CurrentOffsetVar;
}

FVector USpyCharacterMovementComponent::GetWallClimbSpeed()
{
	const FVector WallNormal = ClimbWallData.NormalVector;
	const FVector UpVector = FVector::UpVector;

	//# ������ ���� ���� ���� ������ ���� Get
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

