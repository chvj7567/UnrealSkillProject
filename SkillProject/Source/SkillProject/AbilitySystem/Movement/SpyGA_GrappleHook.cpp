// Fill out your copyright notice in the Description page of Project Settings.

#include "SpyGA_GrappleHook.h"
#include "GrappleCableActor.h"
#include "SpyAbilityTask_GrappleTick.h"
#include "Data/SpyMovementConfig.h"
#include "Util/SpyGameplayTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGA_GrappleHook)

USpyGA_GrappleHook::USpyGA_GrappleHook()
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    ActivationBlockedTags.AddTag(SpyGameplayTags::Character_State_Grapple);
}

void USpyGA_GrappleHook::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    CurrentSpecHandle     = Handle;
    CurrentActorInfo      = ActorInfo;
    CurrentActivationInfo = ActivationInfo;

    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        ASC->AddLooseGameplayTag(SpyGameplayTags::Lock_Input_Move);
        ASC->AddLooseGameplayTag(SpyGameplayTags::Character_State_Grapple);
    }

    if (!HasAuthority(&ActivationInfo))
        return;

    FVector ImpactPoint;
    if (!TryLineTrace(ImpactPoint))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!Char)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    LaunchToTarget(Char, ImpactPoint);

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Char;
    CableActor = GetWorld()->SpawnActor<AGrappleCableActor>(SpawnParams);
    if (CableActor)
    {
        CableActor->InitCable(Char, ImpactPoint, HandBoneName);
    }

    const float Threshold = MovementConfig ? MovementConfig->GrappleArrivalThreshold : 150.f;
    USpyAbilityTask_GrappleTick* Task = USpyAbilityTask_GrappleTick::GrappleTick(this, CableActor, Threshold);
    Task->OnArrived.AddDynamic(this, &USpyGA_GrappleHook::OnGrappleArrived);
    Task->ReadyForActivation();
}

void USpyGA_GrappleHook::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        ASC->RemoveLooseGameplayTag(SpyGameplayTags::Lock_Input_Move);
        ASC->RemoveLooseGameplayTag(SpyGameplayTags::Character_State_Grapple);
    }

    if (HasAuthority(&ActivationInfo) && CableActor)
    {
        CableActor->Destroy();
        CableActor = nullptr;
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USpyGA_GrappleHook::OnGrappleArrived()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

bool USpyGA_GrappleHook::TryLineTrace(FVector& OutImpactPoint) const
{
    APawn* OwnerPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
    APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
    if (!PC || !MovementConfig) return false;

    FVector  CamLoc;
    FRotator CamRot;
    PC->GetPlayerViewPoint(CamLoc, CamRot);

    const FVector TraceEnd = CamLoc + CamRot.Vector() * MovementConfig->GrappleMaxRange;

    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetAvatarActorFromActorInfo());

    if (!GetWorld()->LineTraceSingleByChannel(HitResult, CamLoc, TraceEnd, ECC_WorldStatic, Params))
        return false;

    OutImpactPoint = HitResult.ImpactPoint;
    return true;
}

void USpyGA_GrappleHook::LaunchToTarget(ACharacter* Character, const FVector& TargetLocation) const
{
    if (!MovementConfig) return;

    const FVector CharLoc   = Character->GetActorLocation();
    const FVector Direction  = (TargetLocation - CharLoc).GetSafeNormal();
    const float   HorzDist   = FVector::Dist2D(CharLoc, TargetLocation);
    const float   Speed      = HorzDist / FMath::Max(MovementConfig->GrappleFlightTime, KINDA_SMALL_NUMBER);

    FVector LaunchVelocity   = Direction * Speed;
    LaunchVelocity.Z        += HorzDist * MovementConfig->GrappleLaunchArcZScale;

    Character->LaunchCharacter(LaunchVelocity, true, true);
}
