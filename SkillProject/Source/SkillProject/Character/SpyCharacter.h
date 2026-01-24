// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "AbilitySystemInterface.h"
#include "SKAbilitySystemGlobals.h"
#include "SKAbilitySystemComponent.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"
#include "Character/SpyCharacterMovementComponent.h"
#include "ModularCharacter.h"
#include "SpyCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UBoxComponent;
class UWidgetComponent;
class UGameplayAbility;
class USKAbilitySystemComponent;
class USKAttributeSet;
class USpyAnimManagerComponent;
class USpyParkourManagerComponent;
class UMotionWarpingComponent;
class ASpyWeapon;
class UAbilitySystemComponent;
class USpyCharacterAttributeSet;
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
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaTime) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

public:
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	FORCEINLINE USKAbilitySystemComponent* GetSKAbilitySystemComponent() const { return Cast<USKAbilitySystemComponent>(AbilitySystemComponent); }
	FORCEINLINE UWidgetComponent* GetHPBarComponent() const { return HPBarComponent; }
	FORCEINLINE USpyParkourManagerComponent* GetSpyParkourManagerComponent() const { return SpyParkourManagerComponent; }
	FORCEINLINE USpyCharacterMovementComponent* GetSpyCharacterMovementComponent() const { return GetCharacterMovement<USpyCharacterMovementComponent>(); }
	FORCEINLINE ASpyWeapon* GetSpyWeapon() const { return SpyWeapon; }
	FORCEINLINE UMotionWarpingComponent* GetMotionWarpingComponent() const { return MotionWarpingComponent; }

private:
	void RegisterAbility();
	void OnClimbTagChanged(const FGameplayTag Tag, int32 NewCount);

public:
	void OnHealthChanged(const struct FOnAttributeChangeData& Data);
	void Death();
	struct FGameplayTagContainer GetActivatableAbilityTags();

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<USKAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	const USpyCharacterAttributeSet* CharacterAttributeSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayAbility>> AbilityClasses;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> HPBarComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim")
	TObjectPtr<USpyPawnExtensionComponent> SpyPawnExtensionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim")
	TObjectPtr<USpyAnimManagerComponent> SpyAnimManagerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour")
	TObjectPtr<USpyParkourManagerComponent> SpyParkourManagerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour")
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<ASpyWeapon> SpyWeapon;
};

