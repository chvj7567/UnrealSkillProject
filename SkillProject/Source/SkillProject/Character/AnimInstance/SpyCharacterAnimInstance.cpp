// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AnimInstance/SpyCharacterAnimInstance.h"
#include "System/SpyPlayerState.h"
#include "Character/SpyCharacter.h"
#include "Character/SpyCharacterMovementComponent.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"
#include "Util/SpyGameplayTags.h"

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

    //# Set IsDeath And IsClimbing, Velocity, GroundSpeed, WallClimbSpeed
    {
        if (ASpyPlayerState* SpyPlayerState = Player->GetPlayerState<ASpyPlayerState>())
        {
            IsDeath = false/*SpyPlayerState->HasState(SpyGameplayTags::Character_State_Survival_Alive) == false*/;
            IsClimbing = PlayerMovementComponent->GetMovementName() == TEXT("Custom");
            if (IsClimbing)
            {
                Velocity = PlayerMovementComponent->GetWallClimbSpeed();
                GroundSpeed = 0.0f;
                Speed = FMath::Sqrt(FMath::Pow(Velocity.X, 2) + FMath::Pow(Velocity.Y, 2) + FMath::Pow(Velocity.Z, 2));

                ZOffset_HL = PlayerMovementComponent->ZOffsetHL;
                ZOffset_HR = PlayerMovementComponent->ZOffsetHR;
                ZOffset_FL = PlayerMovementComponent->ZOffsetFL;
                ZOffset_FR = PlayerMovementComponent->ZOffsetFR;
            }
            else
            {
                Velocity = Player->GetVelocity();
                GroundSpeed = FMath::Sqrt(FMath::Pow(Velocity.X, 2) + FMath::Pow(Velocity.Y, 2));
                Speed = FMath::Sqrt(FMath::Pow(Velocity.X, 2) + FMath::Pow(Velocity.Y, 2) + FMath::Pow(Velocity.Z, 2));

                ZOffset_HL = 0.f;
                ZOffset_HR = 0.f;
                ZOffset_FL = 0.f;
                ZOffset_FR = 0.f;
            }
        }
    }

    //# Set ShouldMove
    {
        FVector CurrentAcceleration = PlayerMovementComponent->GetCurrentAcceleration();
        if (CurrentAcceleration.IsZero() == false && GroundSpeed > 3.f)
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
}

void USpyCharacterAnimInstance::AnimNotify_AttackHit(UAnimNotify* Notify)
{
    //Player->TestHit();
}