// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyParkourManagerComponent.h"
#include "Util/DefineEnum.h"
#include "Character/SpyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "System/SpyPlayerController.h"
#include "System/SpyPlayerState.h"

USpyParkourManagerComponent::USpyParkourManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USpyParkourManagerComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USpyParkourManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    CheckAbleWallClimbing();
}

void USpyParkourManagerComponent::CheckAbleWallClimbing()
{
    ASpyCharacter* OwnerChar = Cast<ASpyCharacter>(GetOwner());
    if (!OwnerChar)
        return;

    FVector Start = OwnerChar->GetActorLocation() + OwnerChar->GetActorForwardVector() * 20.f;
    FVector End = Start + OwnerChar->GetActorForwardVector() * 40.f;

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwnerChar);

    bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

    FColor LineColor = bHit ? FColor::Green : FColor::Red;
    float Lifetime = 1.0f;
    float Thickness = 2.0f;

    DrawDebugLine(
        GetWorld(),
        Start,
        End,
        LineColor,
        false,
        Lifetime,
        0,
        Thickness
    );

    if (bIsWallClimbing != bHit)
    {
        bIsWallClimbing = bHit;
        HitNormalVector = Hit.ImpactNormal;

        ASpyPlayerController* SpyPlayerController = OwnerChar->GetController<ASpyPlayerController>();
        ASpyPlayerState* SpyPlayerState = OwnerChar->GetPlayerState<ASpyPlayerState>();

        if (!SpyPlayerController || !SpyPlayerState)
            return;

        if (bIsWallClimbing)
        {
            SpyPlayerState->AddState(ESpyPlayerStateFlags::IsClimb);
        }
        else
        {
            SpyPlayerState->RemoveState(ESpyPlayerStateFlags::IsClimb);
        }

        SpyPlayerController->SetMappingContext();
    }
}

