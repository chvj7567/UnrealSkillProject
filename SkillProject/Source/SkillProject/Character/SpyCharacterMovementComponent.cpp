// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/SpyCharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Util/DefineEnum.h"
#include "Character/SpyCharacter.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Util/SpyGameplayTags.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "Data/SpyMovementConfig.h"
#include "DrawDebugHelpers.h"
#include "SKDebug.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyCharacterMovementComponent)

USpyCharacterMovementComponent::USpyCharacterMovementComponent()
{
	SetIsReplicatedByDefault(true);
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

void USpyCharacterMovementComponent::InjectTargetProvider(TScriptInterface<ISpyTargetProvider> InProvider)
{
	//# 핸들 유효성과 무관하게 해결됨으로 표시한다 — 널도 "컴포넌트 없음" 이라는 확정 정보다.
	TargetProvider = InProvider;
	bTargetProviderResolved = true;
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

			//# 아래 타겟팅 분기는 플레이어 전용 로직이다.
			//# AI 는 ASpyCharacter::PossessedBy 에서 bUseControllerRotationYaw = true /
			//# bOrientRotationToMovement = false 로 설정되고, 회전은 컨트롤러의 FaceRotation 이 담당한다.
			//# 타겟이 없는 AI 가 이 분기에 들어오면 매 프레임 bOrientRotationToMovement 를 true 로 되돌려
			//# 그 설정을 무효화한다 → 공격 중(Lock.Input.Move) BT 가 회전을 놓는 순간 엔진이 경로 이동 방향
			//# (CircleStrafe EQS = 플레이어 반대편)으로 회전시켜 180° 스핀이 발생한다.
			//# 판정은 설정을 넣은 PossessedBy 와 동일한 기준(IsPlayerController)으로 맞춘다 —
			//# 두 곳이 다른 기준을 쓰면 어긋날 수 있다. 컨트롤러가 아직 없는 스폰 직후 프레임도
			//# 이 경로로 빠지지만, 플래그를 건드리지 않는 쪽이라 오판해도 안전하다.
			AController* OwnerController = OwnerCharacter->GetController();
			if (OwnerController == nullptr || OwnerController->IsPlayerController() == false)
			{
				Super::PhysicsRotation(DeltaTime);
				break;
			}

			//# 주입 전이면 "타깃 없음" 경로로 흘린다 — 컴포넌트 등록 이후의 기존 동작과 동치.
			if (bTargetProviderResolved == false)
			{
				OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = true;
				Super::PhysicsRotation(DeltaTime);
				break;
			}

			//# 주입됐는데 널이면 타깃팅 컴포넌트가 없는 캐릭터다 — 기존 nullptr 경로와 동일하게 빠진다.
			ISpyTargetProvider* Provider = TargetProvider.GetInterface();
			if (Provider == nullptr)
				break;

			if (USpyAbilitySystemComponent* ASC = OwnerCharacter->GetSpyAbilitySystemComponent())
			{
				if (ASC->HasMatchingGameplayTag(SKGameplayTags::Character_State_Death))
					break;
			}

			if (Provider->GetTarget().IsValid())
			{
				FVector LookDir = Provider->GetTarget()->GetActorLocation() - OwnerCharacter->GetActorLocation();
				LookDir.Z = 0.f;
				FRotator TargetRot = LookDir.Rotation();

				//# 플레이어 전용 토글 — 유지한다. EndWallClimb 이 bOrientRotationToMovement 를 무조건 true 로
				//# 되돌리므로, 타겟 락 중 등반이 끝난 프레임을 여기서 다시 false 로 복구해 준다.
				OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
				OwnerCharacter->SetActorRotation(TargetRot);
				break;
			}
			else
			{
				//# 엔진 PhysicsRotation 은 bOrientRotationToMovement / bUseControllerDesiredRotation 이
				//# 모두 false 면 즉시 반환한다 — 타겟 해제 후 이동 방향 회전을 되살리려면 이 대입이 필요하다.
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
		const float HangUpFwdOffset = MovementConfig ? MovementConfig->ClimbHangUpRayForwardOffset : 50.f;
		const float HangUpUpOffset  = MovementConfig ? MovementConfig->ClimbHangUpRayUpOffset     : 200.f;
		const float HangUpDownDist  = MovementConfig ? MovementConfig->ClimbHangUpRayDownDistance : 500.f;
		FVector Start = OwnerLocation + OwnerFowardVector * (ClimbData.DistanceOffset + HangUpFwdOffset) + FVector::UpVector * HangUpUpOffset;
		FVector End = Start + FVector::DownVector * HangUpDownDist;
		FHitResult HitResult;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(GetOwner());

		bool bHit = World->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, Params);
		if (SKDebugDrawEnabled())
			DrawDebugLine(World, Start, End, bHit ? FColor::Green : FColor::Red, false, 1.f, 0, -1.f);

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

	//# 등반 중에 다시 들어오면 이미 0 인 GravityScale 을 캐시에 덮어써 복구값이 0 이 된다 — 최초 1회만 캐시한다
	if (bWallClimbing == false)
	{
		bWallClimbing = true;
		CachedGravityScale = GravityScale;
	}

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
	//# 등반을 시작한 적이 없으면 되돌릴 상태도 없다.
	//# (벽이 없을 때 USpyGA_WallClimb::ActivateAbility 가 곧바로 EndAbility → EndWallClimb 을 부르는 경로가 있어
	//#  이 가드가 없으면 중력이 캐시 초기값으로 덮어써지고 이동 모드까지 강제로 Walking 이 된다)
	if (bWallClimbing == false)
		return;

	UE_LOG(LogTemp, Warning, TEXT("EndWallClimb %s"), *GetOwner()->GetName());

	bWallClimbing = false;

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
	if (SKDebugDrawEnabled())
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

	//# 벽 평면까지의 거리 계산
	float DistanceFromPlane = FVector::DotProduct(AnimBoneLocation - WallPoint, WallNormal);

	//# 목표 오프셋 (벽 법선 기준으로 이동)
	FVector TargetWorldOffset = (-WallNormal * (DistanceFromPlane - Offset)) * IKWeight;

	//# 외적을 통해 벽의 위쪽 오른쪽 벡터 Get
	FVector TargetMeshOffset = Mesh->GetComponentTransform().InverseTransformVectorNoScale(TargetWorldOffset);

	CurrentOffsetVar = FMath::VInterpTo(CurrentOffsetVar, TargetMeshOffset, DeltaTime, InterpSpeed);

	return CurrentOffsetVar;
}

bool USpyCharacterMovementComponent::IsClimbingActive() const
{
	const FName ModeName = MovementConfig ? MovementConfig->ClimbingMovementModeName : FName(TEXT("Custom"));
	return GetMovementName() == ModeName;
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

