// Fill out your copyright notice in the Description page of Project Settings.

#include "SpyGA_GrappleHook.h"
#include "GrappleCableActor.h"
#include "SpyAbilityTask_GrappleTick.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Data/SpyMovementConfig.h"
#include "Util/SpyGameplayTags.h"
#include "Character/CommonInterface.Character.h"
#include "System/SpyMissionComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGA_GrappleHook)

bool USpyGA_GrappleHook::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    if (Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags) == false)
        return false;

    APawn* Pawn = Cast<APawn>(ActorInfo->AvatarActor.Get());
    if (Pawn && Pawn->IsLocallyControlled())
    {
		ISpyCharacterRoot* RootPtr = Cast<ISpyCharacterRoot>(Pawn);
		TScriptInterface<ISpyGrappleHost> GrappleHandle = RootPtr ? RootPtr->GetGrappleHost() : TScriptInterface<ISpyGrappleHost>();
		ISpyGrappleHost* Host = IsValid(GrappleHandle.GetObject()) ? GrappleHandle.GetInterface() : nullptr;
		return Host != nullptr && Host->GetLocalCachedTarget() != nullptr;
	}
	return true;
}

USpyGA_GrappleHook::USpyGA_GrappleHook()
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    ActivationBlockedTags.AddTag(SpyGameplayTags::Character_State_Grapple);

    CableActorClass = AGrappleCableActor::StaticClass();
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
    if (Char == nullptr)
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
		ISpyCharacterRoot* RootPtr = Cast<ISpyCharacterRoot>(AvatarActor);
		TScriptInterface<ISpyGrappleHost> GrappleHandle = RootPtr ? RootPtr->GetGrappleHost() : TScriptInterface<ISpyGrappleHost>();
		ISpyGrappleHost* Host = IsValid(GrappleHandle.GetObject()) ? GrappleHandle.GetInterface() : nullptr;

		UE_LOG(LogTemp, Warning, TEXT("[GrappleGA][SRV] TargetComp class=%s"),
			   Host ? *Host->_getUObject()->GetClass()->GetName() : TEXT("NULL"));

		if (Host == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("[GrappleGA][SRV] TargetComp NOT FOUND → EndAbility"));
            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
            return;
		}

		AActor* Target = Host->GetCurrentGrappleTarget();
		UE_LOG(LogTemp, Warning, TEXT("[GrappleGA][SRV] CurrentGrappleTarget=%s"),
			   Target ? *Target->GetName() : TEXT("NULL"));

		if (Target == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("[GrappleGA][SRV] Target is NULL → EndAbility"));
            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
            return;
        }

        TargetLocation = Target->GetActorLocation();

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = Char;
        TSubclassOf<AGrappleCableActor> SpawnClass = CableActorClass;
        if (SpawnClass == nullptr)
        {
            SpawnClass = AGrappleCableActor::StaticClass();
        }
        CableActor = GetWorld()->SpawnActor<AGrappleCableActor>(SpawnClass, SpawnParams);
        if (CableActor == nullptr)
        {
            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
            return;
        }
        CableActor->InitCable(Char, TargetLocation, HandBoneName);
        UE_LOG(LogTemp, Warning, TEXT("[GrappleGA][SRV] CableActor spawned  TargetLoc=%s"), *TargetLocation.ToString());

		//# 미션 진행 — 서버가 타겟을 확정하고 케이블이 실제로 걸린 시점.
		//# 이 블록은 bAuthority 분기 안이고 위에서 Target == nullptr 을 이미 걸러냈다
		if (USpyMissionComponent* MissionComp = USpyMissionComponent::FindMissionComponent(Char->GetPlayerState()))
		{
			MissionComp->AddProgress(SpyGameplayTags::Skill_Move_GrappleHook, 1);
		}
	}
    else
    {
        APawn* Pawn = Cast<APawn>(AvatarActor);
		ISpyCharacterRoot* RootPtr = Cast<ISpyCharacterRoot>(Pawn);
		TScriptInterface<ISpyGrappleHost> GrappleHandle = RootPtr ? RootPtr->GetGrappleHost() : TScriptInterface<ISpyGrappleHost>();
		ISpyGrappleHost* Host = IsValid(GrappleHandle.GetObject()) ? GrappleHandle.GetInterface() : nullptr;

		UE_LOG(LogTemp, Warning, TEXT("[GrappleGA][CLI] TargetComp class=%s"),
			   Host ? *Host->_getUObject()->GetClass()->GetName() : TEXT("NULL"));

		AActor* LocalTarget = Host ? Host->GetLocalCachedTarget() : nullptr;

		UE_LOG(LogTemp, Warning, TEXT("[GrappleGA][CLI] LocalCachedTarget=%s"),
            LocalTarget ? *LocalTarget->GetName() : TEXT("NULL"));

        if (LocalTarget == nullptr)
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

void USpyGA_GrappleHook::InputPressed(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo)
{
    Super::InputPressed(Handle, ActorInfo, ActivationInfo);

    UE_LOG(LogTemp, Warning, TEXT("[GrappleGA] InputPressed during active → cancel grapple"));

    if (USpyAbilitySystemComponent* SpyASC = Cast<USpyAbilitySystemComponent>(ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr))
    {
        SpyASC->ConsumeInputForHandle(Handle);
    }

    CancelAbility(Handle, ActorInfo, ActivationInfo, true);
}
