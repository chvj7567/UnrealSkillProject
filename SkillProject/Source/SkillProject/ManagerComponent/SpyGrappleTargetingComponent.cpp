// Fill out your copyright notice in the Description page of Project Settings.

#include "SpyGrappleTargetingComponent.h"
#include "Data/SpyMovementConfig.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"
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

    if (NewTarget != LocalCachedTarget.Get())
    {
        LocalCachedTarget = NewTarget;
        OnGrappleTargetChanged.Broadcast(NewTarget);

        // null은 서버에 보내지 않음 — 마지막 유효 타겟을 유지
        if (NewTarget != nullptr)
        {
            Server_SetGrappleTarget(NewTarget);
        }
    }
}

AActor* USpyGrappleTargetingComponent::FindBestTarget() const
{
    if (!MovementConfig)
    {
        GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, TEXT("[GrappleTgt] MovementConfig is NULL"));
        return nullptr;
    }

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
    if (!PC) return nullptr;

    FVector2D ViewportSize;
    GEngine->GameViewport->GetViewportSize(ViewportSize);
    const FVector2D ViewportCenter = ViewportSize * 0.5f;

    // 탐색 반경 구체
    DrawDebugSphere(GetWorld(), OwnerPawn->GetActorLocation(),
        MovementConfig->GrapplePromptRange, 16, FColor::Cyan, false, 0.f, 0, 1.f);

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

    GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::White,
        FString::Printf(TEXT("[GrappleTgt] Overlap=%d  Center=%s  Radius=%.0f"),
            OutActors.Num(), *ViewportCenter.ToString(), MovementConfig->GrappleTargetingScreenRadius));

    AActor* BestTarget = nullptr;
    float BestDistToCenter = MovementConfig->GrappleTargetingScreenRadius;

    for (AActor* Actor : OutActors)
    {
        if (!Actor) continue;

        const bool bHasTag = Actor->Tags.Contains(FName("GrappleAnchor"));

        FVector2D ScreenPos;
        const bool bOnScreen = PC->ProjectWorldLocationToScreen(Actor->GetActorLocation(), ScreenPos, true);
        const float DistToCenter = bOnScreen ? FVector2D::Distance(ScreenPos, ViewportCenter) : -1.f;

        // 월드 디버그 텍스트
        DrawDebugString(GetWorld(), Actor->GetActorLocation() + FVector(0,0,50),
            FString::Printf(TEXT("%s\nTag:%d Screen:%d Dist:%.0f"),
                *Actor->GetName(), bHasTag, bOnScreen, DistToCenter),
            nullptr, bHasTag ? FColor::Yellow : FColor::Silver, 0.f);

        if (!bHasTag || !bOnScreen) continue;

        if (DistToCenter < BestDistToCenter)
        {
            BestDistToCenter = DistToCenter;
            BestTarget = Actor;
        }
    }

    if (BestTarget)
    {
        DrawDebugSphere(GetWorld(), BestTarget->GetActorLocation(), 60.f, 8, FColor::Green, false, 0.f, 0, 3.f);
        GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Green,
            FString::Printf(TEXT("[GrappleTgt] BEST=%s (%.0fpx)"), *BestTarget->GetName(), BestDistToCenter));
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Orange, TEXT("[GrappleTgt] No target"));
    }

    return BestTarget;
}

void USpyGrappleTargetingComponent::Server_SetGrappleTarget_Implementation(AActor* NewTarget)
{
    UE_LOG(LogTemp, Warning, TEXT("[GrappleTgt][Server] Set=%s  NetMode=%d  Time=%.3f  Frame=%llu"),
        NewTarget ? *NewTarget->GetName() : TEXT("NULL"),
        (int32)GetWorld()->GetNetMode(),
        GetWorld()->GetTimeSeconds(),
        GFrameCounter);
    CurrentGrappleTarget = NewTarget;
}
