// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"

#include "SpyAbilityTask_MotionWarping.generated.h"

//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSyncMotionWarpingDataDelegate, FVaultMotionWarpingData, InVaultData);

UCLASS()
class SKILLPROJECT_API USpyAbilityTask_MotionWarping : public UAbilityTask
{
	GENERATED_BODY()
	
public:
    USpyAbilityTask_MotionWarping(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
    static USpyAbilityTask_MotionWarping* CreateMotionWarpingSyncTask(UGameplayAbility* OwningAbility, FVaultMotionWarpingData InVaultData);

    virtual void Activate() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    UFUNCTION(NetMulticast, Reliable)
    void BroadcastData(FVaultMotionWarpingData InVaultData);

public:

protected:
    FVaultMotionWarpingData VaultData;
};
