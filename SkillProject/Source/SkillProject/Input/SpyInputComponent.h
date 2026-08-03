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
	//# 기존 컨텍스트를 전부 제거하고 하나만 새로 등록 (완전 교체)
	void ReplaceMappingContext(UInputMappingContext* InMappingContext, int32 Priority);

	//# 기존 컨텍스트를 유지하면서 추가 등록 (스택 방식, Priority로 우선순위 결정)
	void AddMappingContext(UInputMappingContext* InMappingContext, int32 Priority);

	//# 지정한 컨텍스트만 제거 (나머지 컨텍스트는 유지)
	void RemoveMappingContext(UInputMappingContext* InMappingContext);

protected:
	void Move(const FInputActionValue& InValue);
	void MoveEnd(const FInputActionValue& InValue);
	void Look(const FInputActionValue& InValue);
	void CursorToggle(const FInputActionValue& InValue);
	void Interact(const FInputActionValue& InValue);

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
