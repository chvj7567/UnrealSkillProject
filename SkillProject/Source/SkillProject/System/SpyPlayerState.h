// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Util/DefineEnum.h"

#include "SpyPlayerState.generated.h"

UCLASS()
class SKILLPROJECT_API ASpyPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
    ASpyPlayerState();

public:
    virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

public:
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_Death();

public:
    bool HasState(ESpyPlayerStateFlags Flag) const;
    void AddState(ESpyPlayerStateFlags Flag);
    void RemoveState(ESpyPlayerStateFlags Flag);
    void ToggleState(ESpyPlayerStateFlags Flag);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "State")
    ESpyPlayerStateFlags PlayerFlags;
};
