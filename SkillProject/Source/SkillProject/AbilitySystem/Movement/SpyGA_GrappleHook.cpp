// Fill out your copyright notice in the Description page of Project Settings.

#include "SpyGA_GrappleHook.h"
#include "GrappleCableActor.h"
#include "SpyAbilityTask_GrappleTick.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Data/SpyMovementConfig.h"
#include "Util/SpyGameplayTags.h"
#include "ManagerComponent/SpyGrappleTargetingComponent.h"
#include "GameFramework/Character.h"
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

    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, TEXT("[GrappleGA] ActivateAbility called"));

    if (!HasAuthority(&ActivationInfo))
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("[GrappleGA] No authority — client only"));
        return;
    }

    AActor* AvatarActor = GetAvatarActorFromActorInfo();
    USpyGrappleTargetingComponent* TargetComp =
        AvatarActor ? AvatarActor->FindComponentByClass<USpyGrappleTargetingComponent>() : nullptr;

    if (!TargetComp)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("[GrappleGA] TargetComp NOT FOUND"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    AActor* Target = TargetComp->GetCurrentGrappleTarget();
    if (!Target)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("[GrappleGA] CurrentGrappleTarget is NULL"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
        FString::Printf(TEXT("[GrappleGA] Target=%s"), *Target->GetName()));

    ACharacter* Char = Cast<ACharacter>(AvatarActor);
    if (!Char)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    const FVector ImpactPoint = Target->GetActorLocation();
    LaunchToTarget(Char, ImpactPoint);

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Char;
    CableActor = GetWorld()->SpawnActor<AGrappleCableActor>(SpawnParams);
    if (!CableActor)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    CableActor->InitCable(Char, ImpactPoint, HandBoneName);

    const float Threshold = MovementConfig ? MovementConfig->GrappleArrivalThreshold : 150.f;
    USpyAbilityTask_GrappleTick* Task = USpyAbilityTask_GrappleTick::GrappleTick(this, CableActor, Threshold);
    Task->OnArrived.AddDynamic(this, &USpyGA_GrappleHook::OnGrappleArrived);
    Task->ReadyForActivation();

    const float Timeout = MovementConfig ? MovementConfig->GrappleFlightTime * 2.f : 2.f;
    UAbilityTask_WaitDelay* TimeoutTask = UAbilityTask_WaitDelay::WaitDelay(this, Timeout);
    TimeoutTask->OnFinish.AddDynamic(this, &USpyGA_GrappleHook::OnGrappleArrived);
    TimeoutTask->ReadyForActivation();
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

void USpyGA_GrappleHook::LaunchToTarget(ACharacter* Character, const FVector& TargetLocation) const
{
    if (!MovementConfig) return;

    const FVector CharLoc  = Character->GetActorLocation();
    const FVector Direction = (TargetLocation - CharLoc).GetSafeNormal();
    const float   HorzDist  = FVector::Dist2D(CharLoc, TargetLocation);
    const float   Speed     = HorzDist / FMath::Max(MovementConfig->GrappleFlightTime, KINDA_SMALL_NUMBER);

    FVector LaunchVelocity  = Direction * Speed;
    LaunchVelocity.Z       += HorzDist * MovementConfig->GrappleLaunchArcZScale;

    Character->LaunchCharacter(LaunchVelocity, true, true);
}
