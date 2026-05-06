// Fill out your copyright notice in the Description page of Project Settings.

#include "SpyGA_GrappleHook.h"
#include "GrappleCableActor.h"
#include "SpyAbilityTask_GrappleTick.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Data/SpyMovementConfig.h"
#include "Util/SpyGameplayTags.h"
#include "ManagerComponent/SpyGrappleTargetingComponent.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGA_GrappleHook)

bool USpyGA_GrappleHook::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
        return false;

    APawn* Pawn = Cast<APawn>(ActorInfo->AvatarActor.Get());
    if (Pawn && Pawn->IsLocallyControlled())
    {
        USpyGrappleTargetingComponent* TargetComp =
            Pawn->FindComponentByClass<USpyGrappleTargetingComponent>();
        return TargetComp && TargetComp->GetLocalCachedTarget() != nullptr;
    }
    return true;
}

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

    AActor* AvatarActor = GetAvatarActorFromActorInfo();
    ACharacter* Char    = Cast<ACharacter>(AvatarActor);
    if (!Char)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    const bool bAuthority = HasAuthority(&ActivationInfo);
    FVector TargetLocation = FVector::ZeroVector;

    UE_LOG(LogTemp, Warning, TEXT("[GrappleGA][%s] ActivateAbility entry  NetMode=%d"),
        bAuthority ? TEXT("SRV") : TEXT("CLI"), (int32)GetWorld()->GetNetMode());

    if (bAuthority)
    {
        USpyGrappleTargetingComponent* TargetComp =
            AvatarActor->FindComponentByClass<USpyGrappleTargetingComponent>();

        UE_LOG(LogTemp, Warning, TEXT("[GrappleGA][SRV] TargetComp class=%s"),
            TargetComp ? *TargetComp->GetClass()->GetName() : TEXT("NULL"));

        if (!TargetComp)
        {
            UE_LOG(LogTemp, Error, TEXT("[GrappleGA][SRV] TargetComp NOT FOUND → EndAbility"));
            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
            return;
        }

        AActor* Target = TargetComp->GetCurrentGrappleTarget();
        UE_LOG(LogTemp, Warning, TEXT("[GrappleGA][SRV] CurrentGrappleTarget=%s"),
            Target ? *Target->GetName() : TEXT("NULL"));

        if (!Target)
        {
            UE_LOG(LogTemp, Error, TEXT("[GrappleGA][SRV] Target is NULL → EndAbility"));
            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
            return;
        }

        TargetLocation = Target->GetActorLocation();

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = Char;
        CableActor = GetWorld()->SpawnActor<AGrappleCableActor>(SpawnParams);
        if (!CableActor)
        {
            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
            return;
        }
        CableActor->InitCable(Char, TargetLocation, HandBoneName);
        UE_LOG(LogTemp, Warning, TEXT("[GrappleGA][SRV] CableActor spawned  TargetLoc=%s"), *TargetLocation.ToString());
    }
    else
    {
        APawn* Pawn = Cast<APawn>(AvatarActor);
        USpyGrappleTargetingComponent* TargetComp =
            Pawn ? Pawn->FindComponentByClass<USpyGrappleTargetingComponent>() : nullptr;

        UE_LOG(LogTemp, Warning, TEXT("[GrappleGA][CLI] TargetComp class=%s"),
            TargetComp ? *TargetComp->GetClass()->GetName() : TEXT("NULL"));

        AActor* LocalTarget = TargetComp ? TargetComp->GetLocalCachedTarget() : nullptr;

        UE_LOG(LogTemp, Warning, TEXT("[GrappleGA][CLI] LocalCachedTarget=%s"),
            LocalTarget ? *LocalTarget->GetName() : TEXT("NULL"));

        if (!LocalTarget)
        {
            UE_LOG(LogTemp, Error, TEXT("[GrappleGA][CLI] LocalTarget NULL → early return (no task)"));
            return;
        }

        TargetLocation = LocalTarget->GetActorLocation();
    }

    const float Threshold = MovementConfig ? MovementConfig->GrappleArrivalThreshold : 150.f;
    const float PullSpeed = MovementConfig ? MovementConfig->GrapplePullSpeed        : 900.f;

    UE_LOG(LogTemp, Warning, TEXT("[GrappleGA][%s] Creating task  TargetLoc=%s  Speed=%.0f  Threshold=%.0f"),
        bAuthority ? TEXT("SRV") : TEXT("CLI"), *TargetLocation.ToString(), PullSpeed, Threshold);

    USpyAbilityTask_GrappleTick* Task = USpyAbilityTask_GrappleTick::GrappleTick(
        this, CableActor, TargetLocation, PullSpeed, Threshold);
    Task->OnArrived.AddDynamic(this, &USpyGA_GrappleHook::OnGrappleArrived);
    Task->ReadyForActivation();

    if (bAuthority)
    {
        const float Distance = FVector::Dist(Char->GetActorLocation(), TargetLocation);
        const float Timeout  = (PullSpeed > 0.f ? Distance / PullSpeed : 1.f) * 2.f;
        UAbilityTask_WaitDelay* TimeoutTask = UAbilityTask_WaitDelay::WaitDelay(this, Timeout);
        TimeoutTask->OnFinish.AddDynamic(this, &USpyGA_GrappleHook::OnGrappleArrived);
        TimeoutTask->ReadyForActivation();
        UE_LOG(LogTemp, Warning, TEXT("[GrappleGA][SRV] Timeout=%.2fs"), Timeout);
    }

    if (AirLoopMontage)
    {
        if (UAbilityTask_PlayMontageAndWait* MontageTask =
            UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
                this, NAME_None, AirLoopMontage))
        {
            MontageTask->ReadyForActivation();
            UE_LOG(LogTemp, Warning, TEXT("[GrappleGA][%s] AirLoopMontage Task started"),
                bAuthority ? TEXT("SRV") : TEXT("CLI"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[GrappleGA][%s] AirLoopMontage is null — skipping Montage Task"),
            bAuthority ? TEXT("SRV") : TEXT("CLI"));
    }
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

    AActor* AvatarActor = GetAvatarActorFromActorInfo();
    if (AvatarActor && AvatarActor->HasAuthority() && CableActor)
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
