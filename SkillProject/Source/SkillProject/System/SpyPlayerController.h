// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Util/DefineEnum.h"
#include "ModularPlayerController.h"

#include "SpyPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class ASpyPlayerState;
struct FInputActionValue;

UCLASS()
class SKILLPROJECT_API ASpyPlayerController : public AModularPlayerController
{
	GENERATED_BODY()
	
public:
	ASpyPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnRep_PlayerState() override;
	
protected:
	void Move(const FInputActionValue& InValue);
	void Look(const FInputActionValue& InValue);
	void JumpPressed(const FInputActionValue& InValue);
	void JumpReleased(const FInputActionValue& InValue);
	
	void UseSkillA(const FInputActionValue& InValue);
	void UseSkillB(const FInputActionValue& InValue);
	void UseSkillC(const FInputActionValue& InValue);
	void UseSkillD(const FInputActionValue& InValue);
	void UseSkillE(const FInputActionValue& InValue);
	void TryClimb(const FInputActionValue& InValue);
	void TryVault(const FInputActionValue& InValue);

	void SetMappingContext(UInputMappingContext* InMappingContext);

public:
	void RefreshMappingContext();

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> WallClimbMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> SkillAAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> SkillBAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> SkillCAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> SkillDAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> SkillEAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> TryVaultAction;
};
