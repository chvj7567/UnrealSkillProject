// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "SpyAbilityTask_GrappleTick.generated.h"

class AGrappleCableActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGrappleArrivedDelegate);

UCLASS()
class SKILLPROJECT_API USpyAbilityTask_GrappleTick : public UAbilityTask
{
    GENERATED_BODY()

public:
    USpyAbilityTask_GrappleTick();

    UFUNCTION(BlueprintCallable, Category = "Ability|Tasks",
              meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = true))
    static USpyAbilityTask_GrappleTick* GrappleTick(
        UGameplayAbility* OwningAbility,
        AGrappleCableActor* InCableActor,
        float InArrivalThreshold);

    virtual void Activate() override;
    virtual void TickTask(float DeltaTime) override;

    UPROPERTY(BlueprintAssignable)
    FGrappleArrivedDelegate OnArrived;

private:
    TWeakObjectPtr<AGrappleCableActor> CableActor;
    float ArrivalThreshold = 150.f;
};
