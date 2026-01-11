// Fill out your copyright notice in the Description page of Project Settings.


#include "GameAbility/Task/SpyAbilityTask_MotionWarping.h"
#include "Character/SpyCharacter.h"

#include "Net/UnrealNetwork.h"

USpyAbilityTask_MotionWarping::USpyAbilityTask_MotionWarping(const FObjectInitializer& ObjectInitializer)
{
}

USpyAbilityTask_MotionWarping* USpyAbilityTask_MotionWarping::CreateMotionWarpingSyncTask(UGameplayAbility* OwningAbility, FVaultMotionWarpingData InVaultData)
{
    USpyAbilityTask_MotionWarping* Task = NewAbilityTask<USpyAbilityTask_MotionWarping>(OwningAbility);

    Task->VaultData = InVaultData;

    return Task;
}

void USpyAbilityTask_MotionWarping::Activate()
{
    Super::Activate();

    if (Ability == nullptr)
        return;

    //# 서버는 이미 CommitAbility에서 값이 세팅됨
    if (Ability->GetActorInfo().IsNetAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("# BroadcastData"));
        BroadcastData(VaultData);
    }
}

void USpyAbilityTask_MotionWarping::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(USpyAbilityTask_MotionWarping, VaultData);
}

void USpyAbilityTask_MotionWarping::BroadcastData_Implementation(FVaultMotionWarpingData InVaultData)
{
    VaultData = InVaultData;

    //OnSyncMotionWarpingData.Broadcast(VaultData);

    UE_LOG(LogTemp, Warning, TEXT("# BroadcastData_Implementation"));
}
