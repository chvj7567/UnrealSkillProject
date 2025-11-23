// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"
#include "Character/SpyCharacterMovementComponent.h"

#include "SpyCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UBoxComponent;
class UWidgetComponent;
class UGameplayAbility;
class UAbilitySystemComponent;
class USpyAttributeSet;
class USpyAnimManagerComponent;
class USpyParkourManagerComponent;
struct FOnAttributeChangeData;

UCLASS(config=Game)
class ASpyCharacter : public ACharacter, public IAbilitySystemInterface
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

public:
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE UAbilitySystemComponent* GetAbilitySystemComponent() const override { return Cast<UAbilitySystemComponent>(AbilitySystemComponent); }
	FORCEINLINE UWidgetComponent* GetHPBarComponent() const { return HPBarComponent; }
	FORCEINLINE USpyParkourManagerComponent* GetSpyParkourManagerComponent() const { return SpyParkourManagerComponent; }
	FORCEINLINE USpyCharacterMovementComponent* GetSpyCharacterMovementComponent() const { return GetCharacterMovement<USpyCharacterMovementComponent>(); }

private:
	UFUNCTION(BlueprintCallable)
	void RegisterAbility();

	UFUNCTION(BlueprintCallable)
	void BindCollision();

public:
	void OnHealthChanged(const struct FOnAttributeChangeData& Data);
	void TestHit();
	struct FGameplayTagContainer GetActivatableAbilityTags();

public:
	UFUNCTION(BlueprintCallable)
	void OnSkillHitOverlap(UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

public:
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_UseSkill(FGameplayTag SkillTag);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* LeftWeaponCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* RightWeaponCollision;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	USpyAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	const USpyAttributeSet* CharacterAttributeSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TArray<TSubclassOf<UGameplayAbility>> AbilityClasses;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> HPBarComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpyAnimManagerComponent> SpyAnimManagerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpyParkourManagerComponent> SpyParkourManagerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpyCharacterMovementComponent> SpyCharacterMovementCompnent;
};

