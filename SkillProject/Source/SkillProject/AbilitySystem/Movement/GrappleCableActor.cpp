// Fill out your copyright notice in the Description page of Project Settings.

#include "GrappleCableActor.h"
#include "CableComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GrappleCableActor)

AGrappleCableActor::AGrappleCableActor()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    CableComponent = CreateDefaultSubobject<UCableComponent>(TEXT("CableComponent"));
    SetRootComponent(CableComponent);

    CableComponent->bAttachStart = true;
    CableComponent->bAttachEnd   = false;
    CableComponent->CableLength  = 0.f;
    CableComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CableComponent->SetAbsolute(false, true, false);

    SetReplicatingMovement(false);
}

void AGrappleCableActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AGrappleCableActor, TargetLocation);
    DOREPLIFETIME(AGrappleCableActor, HandBoneName);
    DOREPLIFETIME(AGrappleCableActor, OwnerCharacter);
}

void AGrappleCableActor::BeginPlay()
{
    Super::BeginPlay();

    if (CableComponent)
    {
        CableComponent->CableWidth  = CableWidth;
        CableComponent->NumSegments = NumSegments;
        if (CableMaterial)
        {
            CableComponent->SetMaterial(0, CableMaterial);
        }
    }

    UpdateCableTransform();
}

void AGrappleCableActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateCableTransform();
}

void AGrappleCableActor::InitCable(ACharacter* InOwnerCharacter, const FVector& InTargetLocation, const FName& InHandBoneName)
{
    OwnerCharacter = InOwnerCharacter;
    TargetLocation = InTargetLocation;
    HandBoneName   = InHandBoneName;
    UpdateCableTransform();
}

void AGrappleCableActor::UpdateCableTransform()
{
    if (OwnerCharacter == nullptr || CableComponent == nullptr) return;

    USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
    if (Mesh == nullptr) return;

    FVector HandBoneWorld        = Mesh->GetBoneLocation(HandBoneName);
    SetActorLocation(HandBoneWorld);
    CableComponent->EndLocation  = TargetLocation - HandBoneWorld;
}
