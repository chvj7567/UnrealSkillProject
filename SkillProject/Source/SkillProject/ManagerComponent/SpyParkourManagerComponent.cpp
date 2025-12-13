// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyParkourManagerComponent.h"
#include "Util/DefineEnum.h"
#include "Character/SpyCharacter.h"
#include "Character/SpyCharacterMovementComponent.h"
#include "System/SpyPlayerController.h"
#include "System/SpyPlayerState.h"

USpyParkourManagerComponent::USpyParkourManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

    bIsWallClimbing = false;
    HitNormalVector = FVector::ZeroVector;
}

void USpyParkourManagerComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USpyParkourManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    //# º® Ã¼Å©
    //CheckAbleWallClimbing();
}

void USpyParkourManagerComponent::CheckAbleWallClimbing()
{
    ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetOwner());
    if (SpyCharacter == nullptr)
        return;

    FVector Start = SpyCharacter->GetActorLocation() + SpyCharacter->GetActorForwardVector() * 20.f;
    FVector End = Start + SpyCharacter->GetActorForwardVector() * 40.f;

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(SpyCharacter);

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

        ASpyPlayerController* SpyPlayerController = SpyCharacter->GetController<ASpyPlayerController>();
        ASpyPlayerState* SpyPlayerState = SpyCharacter->GetPlayerState<ASpyPlayerState>();

        if (SpyPlayerController == nullptr || SpyPlayerState == nullptr)
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

