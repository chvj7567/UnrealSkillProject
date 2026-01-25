// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "Components/GameFrameworkInitStateInterface.h"

#include "SpyPawnExtensionComponent.generated.h"

class USpyCharacterAssetData;
class USpyKAbilitySystemComponent;

struct FComponentRequestHandle;

UCLASS()
class SKILLPROJECT_API USpyPawnExtensionComponent : public UPawnComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()
	
public:

	USpyPawnExtensionComponent(const FObjectInitializer& ObjectInitializer);

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	static const FName NAME_ActorFeatureName;

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

	const USpyCharacterAssetData* GetCharacterAssetData() const { return CharacterAssetData; }
	void SetCharacterAssetData(const USpyCharacterAssetData* InCharacterAssetData);

	UFUNCTION(BlueprintPure)
	USpyAbilitySystemComponent* GetSpyAbilitySystemComponent() const { return AbilitySystemComponent; }

public:
	void InitializeAbilitySystem(USpyAbilitySystemComponent* InASC, AActor* InOwnerActor);
	void UninitializeAbilitySystem();
	void HandleControllerChanged();
	void HandlePlayerStateReplicated();
	void SetupPlayerInputComponent();

	UFUNCTION()
	void HandleExtensionEvent(AActor* OwnerActor, FName EventName);

protected:
	UFUNCTION()
	void OnRep_CharacterAssetData();
	UPROPERTY(ReplicatedUsing = OnRep_CharacterAssetData)
	TObjectPtr<const USpyCharacterAssetData> CharacterAssetData;

	FSimpleMulticastDelegate OnAbilitySystemInitialized;
	FSimpleMulticastDelegate OnAbilitySystemUninitialized;
	UPROPERTY(Transient)
	TObjectPtr<USpyAbilitySystemComponent> AbilitySystemComponent;

	TSharedPtr<FComponentRequestHandle> ExtensionHandle;
};
