// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "Components/GameFrameworkInitStateInterface.h"

#include "SpyPawnExtensionComponent.generated.h"

class USpyCharacterAssetData;

UCLASS()
class SKILLPROJECT_API USpyPawnExtensionComponent : public UPawnComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()
	
public:

	USpyPawnExtensionComponent(const FObjectInitializer& ObjectInitializer);

	static const FName NAME_ActorFeatureName;

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	//# Begin IGameFrameworkInitStateInterface interface
	virtual FName GetFeatureName() const override { return NAME_ActorFeatureName; }
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	virtual void CheckDefaultInitialization() override;
	//# End IGameFrameworkInitStateInterface interface

public:
	UFUNCTION(BlueprintPure)
	static USpyPawnExtensionComponent* FindPawnExtensionComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<USpyPawnExtensionComponent>() : nullptr); }

	void SetCharacterAssetData(const USpyCharacterAssetData& InCharacterAssetData);

protected:
	UFUNCTION()
	void OnRep_CharacterAssetData();

	UPROPERTY(ReplicatedUsing = OnRep_CharacterAssetData)
	TObjectPtr<const USpyCharacterAssetData> CharacterAssetData;
};
