// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AnimInstance/SpyCharacterAnimInstance.h"
#include "System/SpyPlayerState.h"
#include "Character/SpyCharacter.h"
#include "Character/SpyCharacterMovementComponent.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyCharacterAnimInstance)

void USpyCharacterAnimInstance::NativeBeginPlay()
{
    Super::NativeBeginPlay();

    APawn* PawnOwner = TryGetPawnOwner();
    if (PawnOwner == nullptr)
        return;

    Player = Cast<ASpyCharacter>(PawnOwner);
    if (Player == nullptr)
        return;

    PlayerMovementComponent = Player->GetSpyCharacterMovementComponent();
    if (PlayerMovementComponent == nullptr)
        return;
}

void USpyCharacterAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

    if (Player == nullptr || PlayerMovementComponent == nullptr)
        return;

    //# Set InputAngle
    {
        InputAngle = PlayerMovementComponent->GetInputAngleByForward();
    }

    //# Set AimPitch
    {
        FRotator Rot;
        if (Player->IsLocallyControlled())
        {
            //# 로컬 플레이어 -> 컨트롤러 기준
            Rot = Player->GetControlRotation();
        }
        else
        {
            //# 서버/원격 -> 캐릭터 기준
            Rot = Player->GetBaseAimRotation();
        }

        AimPitch = FRotator::NormalizeAxis(Rot.Pitch);
    }

    //# Set Velocity, GroundSpeed, WallClimbSpeed, ShouldMove
    {
        Velocity = Player->GetVelocity();
        GroundSpeed = FMath::Sqrt(FMath::Pow(Velocity.X, 2) + FMath::Pow(Velocity.Y, 2));
        WallClimbSpeed = FMath::Sqrt(FMath::Pow(Velocity.X, 2) + FMath::Pow(Velocity.Z, 2));

        FVector CurrentAcceleration = PlayerMovementComponent->GetCurrentAcceleration();
        if (CurrentAcceleration.IsZero() == false && GroundSpeed > 3.f && IsLanding == false)
        {
            ShouldMove = true;
        }
        else
        {
            ShouldMove = false;
        }
    }

    //# Set IsFalling And IsCrouching
    {
        IsFalling = PlayerMovementComponent->IsFalling();
        IsCrouching = PlayerMovementComponent->IsCrouching();
    }

    //# Set IsDeath And IsClimbing
    {
        if (ASpyPlayerState* SpyPlayerState = Cast<ASpyPlayerState>(Player->GetPlayerState()))
        {
            IsDeath = SpyPlayerState->HasState(ESpyPlayerStateFlags::IsAlive) == false;
            IsClimbing = SpyPlayerState->HasState(ESpyPlayerStateFlags::IsClimb);
            if (IsClimbing)
            {
                CalculateBoneOffset(TEXT("hand_l"), ZOffset_HL, DeltaSeconds);
                CalculateBoneOffset(TEXT("hand_r"), ZOffset_HR, DeltaSeconds);
                CalculateBoneOffset(TEXT("foot_l"), ZOffset_FL, DeltaSeconds);
                CalculateBoneOffset(TEXT("foot_r"), ZOffset_FR, DeltaSeconds);
            }
        }
    }
}

void USpyCharacterAnimInstance::AnimNotify_AttackHit(UAnimNotify* Notify)
{
    //Player->TestHit();
}

void USpyCharacterAnimInstance::AnimNotify_DisableMove(UAnimNotify* Notify)
{
    if (ASpyPlayerState* SpyPlayerState = Cast<ASpyPlayerState>(Player->GetPlayerState()))
    {
        if (SpyPlayerState->HasState(ESpyPlayerStateFlags::IsClimb) == false)
        {
            IsLanding = true;
            PlayerMovementComponent->DisableMovement();
        }
    }
}

void USpyCharacterAnimInstance::AnimNotify_AbleMove(UAnimNotify* Notify)
{
    IsLanding = false;
    PlayerMovementComponent->SetMovementMode(EMovementMode::MOVE_Walking);
}

float USpyCharacterAnimInstance::GetClosestLadderHeight(float CurrentHeight)
{
    TArray<float> Ladder = { 100, 150, 200, 270, 340, 400, 450, 500, 560 };

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

float USpyCharacterAnimInstance::CalculateBoneOffset(FName BoneName, float& CurrentOffsetVar, float DeltaTime)
{
    FVector AnimBoneLoc = Player->GetMesh()->GetSocketLocation(BoneName);
    float TargetZ = GetClosestLadderHeight(AnimBoneLoc.Z);
    float RawOffset = TargetZ - AnimBoneLoc.Z;

    CurrentOffsetVar = FMath::FInterpTo(CurrentOffsetVar, RawOffset, DeltaTime, 0.f);
    return CurrentOffsetVar;
}
