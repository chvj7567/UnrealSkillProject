// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AnimInstance/CharacterAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "World/SkillProjectPlayerState.h"

void UCharacterAnimInstance::NativeBeginPlay()
{
    Super::NativeBeginPlay();

    APawn* PawnOwner = TryGetPawnOwner();
    if (PawnOwner == nullptr)
        return;

    Player = Cast<ASkillProjectCharacter>(PawnOwner);
    if (Player == nullptr)
        return;

    PlayerMovementComponent = Player->GetCharacterMovement();
    if (PlayerMovementComponent == nullptr)
        return;
}

void UCharacterAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

    if (Player == nullptr || PlayerMovementComponent == nullptr)
        return;

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

    //# Set Velocity And GroundSpeed And ShouldMove
    {
        Velocity = Player->GetVelocity();
        GroundSpeed = FMath::Sqrt(FMath::Pow(Velocity.X, 2) + FMath::Pow(Velocity.Y, 2));

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

    //# Set IsDeath
    {
        if (ASkillProjectPlayerState* SpyPlayerState = Cast<ASkillProjectPlayerState>(Player->GetPlayerState()))
        {
            IsDeath = SpyPlayerState->HasState(ESpyPlayerStateFlags::IsAlive) == false;
        }
    }
}

void UCharacterAnimInstance::AnimNotify_AttackHit(UAnimNotify* Notify)
{
    Player->TestHit();
}
