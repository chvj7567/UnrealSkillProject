// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Util/DefineEnum.h"
#include "ModularPlayerController.h"
#include "System/SpyTeamAgentInterface.h"

#include "SpyPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class ASpyPlayerState;
class USpyAbilitySystemComponent;

struct FInputActionValue;

UCLASS()
class SKILLPROJECT_API ASpyPlayerController : public AModularPlayerController, public ISpyTeamAgentInterface
{
	GENERATED_BODY()
	
public:
	ASpyPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnRep_PlayerState() override;
	
	virtual void PreProcessInput(const float DeltaTime, const bool bGamePaused) override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;

	//~IGenericTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	//~End of IGenericTeamAgentInterface interface

public:
	UFUNCTION(BlueprintCallable)
	USpyAbilitySystemComponent* GetSpyAbilitySystemComponent() const;

private:
	UPROPERTY(EditAnywhere)
	FGenericTeamId TeamID;
};
