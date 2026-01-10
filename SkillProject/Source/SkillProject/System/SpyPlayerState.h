// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Util/DefineEnum.h"

#include "SpyPlayerState.generated.h"

class ASpyCharacter;
struct FGameplayTag;

UCLASS()
class SKILLPROJECT_API ASpyPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
    ASpyPlayerState();

public:
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_Death();

public:
    void Initialize();

    bool HasState(FGameplayTag Tag);
    void AddState(FGameplayTag Tag);
    void RemoveState(FGameplayTag Tag);
    void ToggleState(FGameplayTag Tag);

protected:
    UPROPERTY()
    TObjectPtr<ASpyCharacter> OwnerCharacter;
};
