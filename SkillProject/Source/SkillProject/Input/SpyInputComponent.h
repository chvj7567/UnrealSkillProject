// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "Components/GameFrameworkInitStateInterface.h"

#include "SpyInputComponent.generated.h"

class UInputMappingContext;

struct FInputActionValue;

UCLASS()
class SKILLPROJECT_API USpyInputComponent : public UPawnComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()
	
public:
	USpyInputComponent(const FObjectInitializer& ObjectInitializer);

public:
	void Input_AbilityInputTagPressed(FGameplayTag InputTag);
	void Input_AbilityInputTagReleased(FGameplayTag InputTag);

	virtual void InitializePlayerInput(UInputComponent* PlayerInputComponent);

	FORCEINLINE bool GetIsBindingInput() { return bIsBindingInput; }

public:
	void SetMappingContext(UInputMappingContext* InMappingContext, int32 Priority);
	void AddMappingContext(UInputMappingContext* InMappingContext, int32 Priority);
	void RemoveMappingContext(UInputMappingContext* InMappingContext);

protected:
	void Move(const FInputActionValue& InValue);
	void Look(const FInputActionValue& InValue);

	void UseSkillA(const FInputActionValue& InValue);
	void UseSkillB(const FInputActionValue& InValue);
	void UseSkillC(const FInputActionValue& InValue);
	void UseSkillD(const FInputActionValue& InValue);
	void UseSkillE(const FInputActionValue& InValue);
	void TryVault(const FInputActionValue& InValue);

protected:
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
	static USpyInputComponent* FindInputComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<USpyInputComponent>() : nullptr); }

protected:
	bool bIsBindingInput;
};
