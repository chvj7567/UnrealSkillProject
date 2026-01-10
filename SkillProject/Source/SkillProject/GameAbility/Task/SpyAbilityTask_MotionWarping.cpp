// Fill out your copyright notice in the Description page of Project Settings.


#include "GameAbility/Task/SpyAbilityTask_MotionWarping.h"
#include "Character/SpyCharacter.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"

USpyAbilityTask_MotionWarping* USpyAbilityTask_MotionWarping::CreateMotionWarpingSyncTask(UGameplayAbility* OwningAbility, FVector SLoc, FRotator SRot, FVector ELoc, FRotator ERot)
{
    USpyAbilityTask_MotionWarping* MotionWarpingTask = NewAbilityTask<USpyAbilityTask_MotionWarping>(OwningAbility);
    MotionWarpingTask->SavedStartLoc = SLoc;
    MotionWarpingTask->SavedStartRot = SRot;
    MotionWarpingTask->SavedEndLoc = ELoc;
    MotionWarpingTask->SavedEndRot = ERot;
    return MotionWarpingTask;
}

void USpyAbilityTask_MotionWarping::Activate()
{
    if (Ability && Ability->GetActorInfo().IsNetAuthority())
    {
        Multicast_SendMotionWarpingData(SavedStartLoc, SavedStartRot, SavedEndLoc, SavedEndRot);
    }
}

void USpyAbilityTask_MotionWarping::Multicast_SendMotionWarpingData_Implementation(FVector StartLoc, FRotator StartRot, FVector EndLoc, FRotator EndRot)
{
    if (Ability && Ability->IsActive())
    {
        OnSyncMotionWarpingData.Broadcast(StartLoc, StartRot, EndLoc, EndRot);
    }

    EndTask();
}