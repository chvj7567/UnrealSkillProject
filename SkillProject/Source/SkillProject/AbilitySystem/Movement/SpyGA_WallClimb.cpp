// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyGA_WallClimb.h"
#include "GameFramework/Character.h"
#include "Util/DefineEnum.h"
#include "Character/SpyCharacterMovementComponent.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"
#include "System/SpyPlayerController.h"
#include "Input/SpyInputComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGA_WallClimb)

USpyGA_WallClimb::USpyGA_WallClimb()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void USpyGA_WallClimb::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USpyGA_WallClimb::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USpyGA_WallClimb::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    CurrentSpecHandle = Handle;
    CurrentActorInfo = ActorInfo;
    CurrentActivationInfo = ActivationInfo;

    if (TryToggleClimbAction() == false)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    }

}

void USpyGA_WallClimb::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    EndWallClimb();

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USpyGA_WallClimb::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
    //# 토글 기능
    if (IsActive() == false)
        return;

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

bool USpyGA_WallClimb::TryToggleClimbAction()
{
    if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
    {
        if (USpyParkourManagerComponent* ParkourComponent = OwnerCharacter->FindComponentByClass<USpyParkourManagerComponent>())
        {
            ParkourComponent->OnClimbData.AddUniqueDynamic(this, &USpyGA_WallClimb::StartWallClimb);
            return ParkourComponent->TryToggleClimbAction();
        }
    }

    return false;
}

void USpyGA_WallClimb::StartWallClimb(const FClimbData& InClimbData, const FClimbWallData& InClimbWallData)
{
    if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
    {
        if (USpyCharacterMovementComponent* MoveComp = Cast<USpyCharacterMovementComponent>(OwnerCharacter->GetCharacterMovement()))
        {
            MoveComp->StartWallClimb(InClimbData, InClimbWallData);
        }

        if (IsPredictingClient())
        {
            if (USpyInputComponent* InputComp = USpyInputComponent::FindInputComponent(OwnerCharacter))
            {
                InputComp->AddMappingContext(InputMappingContext, 99);
            }
        }
    }
}

void USpyGA_WallClimb::EndWallClimb()
{
    if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
    {
        if (USpyCharacterMovementComponent* MoveComp = Cast<USpyCharacterMovementComponent>(OwnerCharacter->GetCharacterMovement()))
        {
            MoveComp->EndWallClimb();
        }

        if (IsPredictingClient())
        {
            if (USpyInputComponent* InputComp = USpyInputComponent::FindInputComponent(OwnerCharacter))
            {
                InputComp->RemoveMappingContext(InputMappingContext);
            }
        }

        if (USpyParkourManagerComponent* ParkourComponent = OwnerCharacter->FindComponentByClass<USpyParkourManagerComponent>())
        {
            ParkourComponent->OnClimbData.RemoveDynamic(this, &USpyGA_WallClimb::StartWallClimb);
        }
    }
}