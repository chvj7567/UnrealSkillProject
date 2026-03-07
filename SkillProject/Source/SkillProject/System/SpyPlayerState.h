// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ModularPlayerState.h"
#include "SKAbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

#include "SpyPlayerState.generated.h"

class ASpyCharacter;
class USpyAbilitySystemComponent;
class USpyCharacterAssetData;

struct FGameplayTag;

UENUM()
enum class EPlayerConnectionType : uint8
{
    ActivePlayer = 0,
    DeactivePlayer = 1,
};

UCLASS()
class SKILLPROJECT_API ASpyPlayerState : public AModularPlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
    ASpyPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

public:
    void SetPlayerConnectionType(EPlayerConnectionType NewType);
    EPlayerConnectionType GetPlayerConnectionType() { return PlayerConnectionType; }

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
    USpyCharacterAssetData* GetCharacterAssetData() const { return CharacterAssetData; }
    void SetCharacterAssetData(USpyCharacterAssetData* InCharacterAssetData, bool bInIsBot);

    UFUNCTION(BlueprintPure)
    USpyAbilitySystemComponent* GetSpyAbilitySystemComponent() const { return AbilitySystemComponent; }

    static const FName NAME_AbilityReady;

protected:
    UFUNCTION()
    void OnRep_CharacterAssetData();

    UPROPERTY(ReplicatedUsing = OnRep_CharacterAssetData)
    TObjectPtr<USpyCharacterAssetData> CharacterAssetData;

protected:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USpyAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY()
    TObjectPtr<const class USpyCharacterAttributeSet> CharacterAttributeSet;

    UPROPERTY(Replicated)
    EPlayerConnectionType PlayerConnectionType;

};
