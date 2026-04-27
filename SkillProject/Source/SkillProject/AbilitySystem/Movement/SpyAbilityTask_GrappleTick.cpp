// Fill out your copyright notice in the Description page of Project Settings.

#include "SpyAbilityTask_GrappleTick.h"
#include "GrappleCableActor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyAbilityTask_GrappleTick)

USpyAbilityTask_GrappleTick::USpyAbilityTask_GrappleTick()
{
    bTickingTask = true;
}

USpyAbilityTask_GrappleTick* USpyAbilityTask_GrappleTick::GrappleTick(
    UGameplayAbility* OwningAbility,
    AGrappleCableActor* InCableActor,
    float InArrivalThreshold)
{
    USpyAbilityTask_GrappleTick* Task = NewAbilityTask<USpyAbilityTask_GrappleTick>(OwningAbility);
    Task->CableActor       = InCableActor;
    Task->ArrivalThreshold = InArrivalThreshold;
    return Task;
}

void USpyAbilityTask_GrappleTick::Activate()
{
    // Tick handles everything
}

void USpyAbilityTask_GrappleTick::TickTask(float DeltaTime)
{
    Super::TickTask(DeltaTime);

    AActor* Avatar = GetAvatarActor();
    if (!Avatar || !Avatar->HasAuthority()) return;

    if (!CableActor.IsValid())
    {
        EndTask();
        return;
    }

    const float Distance = FVector::Dist(Avatar->GetActorLocation(), CableActor->GetTargetLocation());
    if (Distance <= ArrivalThreshold)
    {
        OnArrived.Broadcast();
        EndTask();
    }
}
