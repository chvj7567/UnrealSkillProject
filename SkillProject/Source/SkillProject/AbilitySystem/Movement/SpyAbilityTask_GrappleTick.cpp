// Fill out your copyright notice in the Description page of Project Settings.

#include "SpyAbilityTask_GrappleTick.h"
#include "GrappleCableActor.h"
#include "GameFramework/Character.h"
#include "SKDebug.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyAbilityTask_GrappleTick)

USpyAbilityTask_GrappleTick::USpyAbilityTask_GrappleTick()
{
    bTickingTask = true;
}

USpyAbilityTask_GrappleTick* USpyAbilityTask_GrappleTick::GrappleTick(
    UGameplayAbility* OwningAbility,
    AGrappleCableActor* InCableActor,
    FVector InTargetLocation,
    float InPullSpeed,
    float InArrivalThreshold)
{
    USpyAbilityTask_GrappleTick* Task = NewAbilityTask<USpyAbilityTask_GrappleTick>(OwningAbility);
    Task->CableActor       = InCableActor;
    Task->TargetLocation   = InTargetLocation;
    Task->PullSpeed        = InPullSpeed;
    Task->ArrivalThreshold = InArrivalThreshold;
    return Task;
}

void USpyAbilityTask_GrappleTick::Activate()
{
}

void USpyAbilityTask_GrappleTick::TickTask(float DeltaTime)
{
    Super::TickTask(DeltaTime);

    ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
    if (!Character) return;

    const FVector CharLoc  = Character->GetActorLocation();
    const FVector ToTarget = (TargetLocation - CharLoc).GetSafeNormal();

    if (Character->HasAuthority())
    {
        if (!CableActor.IsValid())
        {
            if (SpyDebugDrawEnabled())
                GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("[GrappleTick] CableActor invalid — EndTask"));
            EndTask();
            return;
        }

        const float Distance = FVector::Dist(CharLoc, TargetLocation);
        if (SpyDebugDrawEnabled())
        {
            GEngine->AddOnScreenDebugMessage(10, 0.f, FColor::Cyan,
                FString::Printf(TEXT("[GrappleTick] Dist=%.0f  Speed=%.0f"), Distance, PullSpeed));
        }

        if (Distance <= ArrivalThreshold)
        {
            OnArrived.Broadcast();
            EndTask();
            return;
        }
    }

    // 클라이언트·서버 모두: CMC LocalPredicted 예측 이동 + 회전
    const FVector HorizDir = FVector(ToTarget.X, ToTarget.Y, 0.f).GetSafeNormal();
    if (!HorizDir.IsNearlyZero())
    {
        const FRotator Current    = Character->GetActorRotation();
        const FRotator DesiredYaw = FRotator(Current.Pitch, HorizDir.Rotation().Yaw, Current.Roll);
        Character->SetActorRotation(
            FMath::RInterpTo(Current, DesiredYaw, DeltaTime, 15.f),
            ETeleportType::TeleportPhysics);
    }

    Character->LaunchCharacter(ToTarget * PullSpeed, true, true);
}

void USpyAbilityTask_GrappleTick::OnDestroy(bool bInOwnerFinished)
{
    Super::OnDestroy(bInOwnerFinished);
}
