// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Character/SpyCharacterMovementComponent.h"
#include "ModularCharacter.h"
#include "AbilitySystemInterface.h"

#include "SpyCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UBoxComponent;
class UWidgetComponent;
class ASpyWeapon;
class USpyPawnExtensionComponent;

struct FOnAttributeChangeData;

UCLASS(config=Game)
class ASpyCharacter : public AModularCharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ASpyCharacter(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	//# IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//# ~IAbilitySystemInterface
public:
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE UWidgetComponent* GetHPBarComponent() const { return HPBarComponent; }
	FORCEINLINE USpyCharacterMovementComponent* GetSpyCharacterMovementComponent() const { return GetCharacterMovement<USpyCharacterMovementComponent>(); }
	FORCEINLINE ASpyWeapon* GetSpyWeapon() const { return SpyWeapon; }

public:
	void Death();

protected:
	virtual void OnAbilitySystemInitialized();
	virtual void OnAbilitySystemUninitialized();

	void InitializeGameplayTags();

public:
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_UseSkill(FGameplayTag SkillTag);

	UFUNCTION(BlueprintCallable)
	void UseSkill(FGameplayTag SkillTag);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> HPBarComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim")
	TObjectPtr<USpyPawnExtensionComponent> SpyPawnExtensionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<ASpyWeapon> SpyWeapon;
};

