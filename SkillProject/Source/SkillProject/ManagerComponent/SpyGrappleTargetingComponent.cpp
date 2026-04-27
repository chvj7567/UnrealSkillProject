// Fill out your copyright notice in the Description page of Project Settings.

#include "SpyGrappleTargetingComponent.h"
#include "Data/SpyMovementConfig.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGrappleTargetingComponent)

USpyGrappleTargetingComponent::USpyGrappleTargetingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
}

void USpyGrappleTargetingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(USpyGrappleTargetingComponent, CurrentGrappleTarget);
}

void USpyGrappleTargetingComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn) return;

    APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
    if (!PC || !PC->IsLocalController()) return;

    AActor* NewTarget = FindBestTarget();

    if (LocalCachedTarget != NewTarget)
    {
        LocalCachedTarget = NewTarget;
        OnGrappleTargetChanged.Broadcast(NewTarget);
        Server_SetGrappleTarget(NewTarget);
    }
}

AActor* USpyGrappleTargetingComponent::FindBestTarget() const
{
    if (!MovementConfig) return nullptr;

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
    if (!PC) return nullptr;

    FVector2D ViewportSize;
    GEngine->GameViewport->GetViewportSize(ViewportSize);
    const FVector2D ViewportCenter = ViewportSize * 0.5f;

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));

    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(GetOwner());

    TArray<AActor*> OutActors;
    UKismetSystemLibrary::SphereOverlapActors(
        GetWorld(),
        OwnerPawn->GetActorLocation(),
        MovementConfig->GrapplePromptRange,
        ObjectTypes,
        nullptr,
        ActorsToIgnore,
        OutActors
    );

    AActor* BestTarget = nullptr;
    float BestDistToCenter = MovementConfig->GrappleTargetingScreenRadius;

    for (AActor* Actor : OutActors)
    {
        if (!Actor || !Actor->Tags.Contains(FName("GrappleAnchor"))) continue;

        FVector2D ScreenPos;
        if (!PC->ProjectWorldLocationToScreen(Actor->GetActorLocation(), ScreenPos, true)) continue;

        const float DistToCenter = FVector2D::Distance(ScreenPos, ViewportCenter);
        if (DistToCenter < BestDistToCenter)
        {
            BestDistToCenter = DistToCenter;
            BestTarget = Actor;
        }
    }

    return BestTarget;
}

void USpyGrappleTargetingComponent::Server_SetGrappleTarget_Implementation(AActor* NewTarget)
{
    CurrentGrappleTarget = NewTarget;
}
