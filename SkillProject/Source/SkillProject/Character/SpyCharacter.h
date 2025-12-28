// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemInterface.h"
#include "SpyAbilitySystemComponent.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"
#include "Character/SpyCharacterMovementComponent.h"

#include "SpyCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UBoxComponent;
class UWidgetComponent;
class UGameplayAbility;
class USpyAbilitySystemComponent;
class USpyAttributeSet;
class USpyAnimManagerComponent;
class USpyParkourManagerComponent;
class ASpyWeapon;
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
	FORCEINLINE USpyAbilitySystemComponent* GetSpyAbilitySystemComponent() const { return Cast<USpyAbilitySystemComponent>(AbilitySystemComponent); }
	FORCEINLINE UWidgetComponent* GetHPBarComponent() const { return HPBarComponent; }
	FORCEINLINE USpyParkourManagerComponent* GetSpyParkourManagerComponent() const { return SpyParkourManagerComponent; }
	FORCEINLINE USpyCharacterMovementComponent* GetSpyCharacterMovementComponent() const { return GetCharacterMovement<USpyCharacterMovementComponent>(); }
	FORCEINLINE ASpyWeapon* GetSpyWeapon() const { return SpyWeapon; }

private:
	UFUNCTION(BlueprintCallable)
	void RegisterAbility();

public:
	void OnHealthChanged(const struct FOnAttributeChangeData& Data);
	void Death();
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

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_UseSkill(FGameplayTag SkillTag);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	USpyAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<ASpyWeapon> SpyWeapon;
};

