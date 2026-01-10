// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"

#include "SpyAbilityTask_MotionWarping.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FSyncMotionWarpingDataDelegate, FVector, StartLoc, FRotator, StartRot, FVector, EndLoc, FRotator, EndRot);

UCLASS()
class SKILLPROJECT_API USpyAbilityTask_MotionWarping : public UAbilityTask
{
	GENERATED_BODY()
	
public:
    UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
    static USpyAbilityTask_MotionWarping* CreateMotionWarpingSyncTask(UGameplayAbility* OwningAbility, FVector SLoc, FRotator SRot, FVector ELoc, FRotator ERot);

    virtual void Activate() override;

protected:
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_SendMotionWarpingData(FVector StartLoc, FRotator StartRot, FVector EndLoc, FRotator EndRot);

public:
    UPROPERTY(BlueprintAssignable)
    FSyncMotionWarpingDataDelegate OnSyncMotionWarpingData;

private:
    FVector SavedStartLoc;
    FRotator SavedStartRot;
    FVector SavedEndLoc;
    FRotator SavedEndRot;

};
