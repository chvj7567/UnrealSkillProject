// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ModularPlayerState.h"

#include "SpyPlayerState.generated.h"

class ASpyCharacter;
class USKAbilitySystemComponent;
class USpyCharacterAssetData;

struct FGameplayTag;

/** Defines the types of client connected */
UENUM()
enum class EPlayerConnectionType : uint8
{
    ActivePlayer = 0,
    DeactivePlayer = 1,
};

UCLASS()
class SKILLPROJECT_API ASpyPlayerState : public AModularPlayerState
{
	GENERATED_BODY()
	
public:
    ASpyPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    void SetPlayerConnectionType(EPlayerConnectionType NewType);
    EPlayerConnectionType GetPlayerConnectionType() { return PlayerConnectionType; }

public:
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_Death();

public:
    //# AActor interface
    virtual void PreInitializeComponents() override;
    virtual void PostInitializeComponents() override;
    //# End of AActor interface

    //# APlayerState interface
    virtual void Reset() override;
    virtual void ClientInitialize(AController* C) override;
    virtual void CopyProperties(APlayerState* PlayerState) override;
    virtual void OnDeactivated() override;
    virtual void OnReactivated() override;
    //# End of APlayerState interface

public:
    void Initialize();

    bool HasState(FGameplayTag Tag);
    void AddState(FGameplayTag Tag);
    void RemoveState(FGameplayTag Tag);
    void ToggleState(FGameplayTag Tag);

public:
    const USpyCharacterAssetData* GetCharacterAssetData() const { return CharacterAssetData; }
    void SetCharacterAssetData(const USpyCharacterAssetData& InCharacterAssetData);

protected:
    UFUNCTION()
    void OnRep_CharacterAssetData();

    UPROPERTY(ReplicatedUsing = OnRep_CharacterAssetData)
    TObjectPtr<const USpyCharacterAssetData> CharacterAssetData;

protected:
    UPROPERTY()
    TObjectPtr<ASpyCharacter> OwnerCharacter;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USKAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY()
    TObjectPtr<const class USpyCharacterAttributeSet> CharacterAttributeSet;

    UPROPERTY(Replicated)
    EPlayerConnectionType PlayerConnectionType;

protected:
    static const FName NAME_AbilityReady;
};
