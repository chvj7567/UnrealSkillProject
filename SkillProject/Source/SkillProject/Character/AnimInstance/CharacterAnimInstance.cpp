// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AnimInstance/CharacterAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"

void UCharacterAnimInstance::NativeBeginPlay()
{
    Super::NativeBeginPlay();

    APawn* PawnOwner = TryGetPawnOwner();
    if (PawnOwner == nullptr)
        return;

    MyCharacter = Cast<ASkillProjectCharacter>(PawnOwner);
    if (MyCharacter == nullptr)
        return;

    MyMovementComponent = MyCharacter->GetCharacterMovement();
    if (MyMovementComponent == nullptr)
        return;
}

void UCharacterAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

    if (MyCharacter == nullptr || MyMovementComponent == nullptr)
        return;

    //# Set AimPitch
    {
        FRotator Rot;
        if (MyCharacter->IsLocallyControlled())
        {
            //# 로컬 플레이어 -> 컨트롤러 기준
            Rot = MyCharacter->GetControlRotation();
        }
        else
        {
            //# 서버/원격 -> 캐릭터 기준
            Rot = MyCharacter->GetBaseAimRotation();
        }

        AimPitch = FRotator::NormalizeAxis(Rot.Pitch);
    }

    //# Set Velocity And GroundSpeed And ShouldMove
    {
        Velocity = MyCharacter->GetVelocity();
        GroundSpeed = FMath::Sqrt(FMath::Pow(Velocity.X, 2) + FMath::Pow(Velocity.Y, 2));

        FVector CurrentAcceleration = MyMovementComponent->GetCurrentAcceleration();
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
        IsFalling = MyMovementComponent->IsFalling();
        IsCrouching = MyMovementComponent->IsCrouching();
    }
}