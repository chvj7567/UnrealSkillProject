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

    //# Set IsDeath
    {
        if (ASpyPlayerState* SpyPlayerState = Cast<ASpyPlayerState>(Player->GetPlayerState()))
        {
            IsDeath = SpyPlayerState->HasState(ESpyPlayerStateFlags::IsAlive) == false;
        }
    }

    //# Set Climbing
    {
        if (USpyParkourManagerComponent* SpyParkourComponent = Player->GetSpyParkourManagerComponent())
        {
            IsWallClimbing = SpyParkourComponent->IsWallClimbing();
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
