// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Util/DefineEnum.h"
#include "ModularPlayerController.h"
#include "SpyPlayerController.generated.h"

class USpyAbilitySystemComponent;
class USpyTargetingManagerComponent;
class UCameraShakeBase;
class USpyHealthComponent;

UCLASS()
class SKILLPROJECT_API ASpyPlayerController : public AModularPlayerController
{
	GENERATED_BODY()

public:
	ASpyPlayerController();

protected:
	virtual void OnUnPossess() override;
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void AcknowledgePossession(APawn* InPawn) override;
	virtual void OnRep_PlayerState() override;
	virtual void UpdateRotation(float DeltaTime) override;
	virtual void PreProcessInput(const float DeltaTime, const bool bGamePaused) override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;

public:
	UFUNCTION(BlueprintCallable)
	USpyAbilitySystemComponent* GetSpyAbilitySystemComponent() const;

	void ToggleCursorMode();

	UFUNCTION()
	void HandleReceivedHit(float Damage, bool bCritical, AActor* DamageCauser);
	void HandleDealtHit(bool bCritical);

	UFUNCTION(Client, Reliable)
	void Client_TriggerShake(bool bCritical, bool bFromReceivedHit);

protected:
	UPROPERTY()
	TObjectPtr<USpyTargetingManagerComponent> TargetingComp;

	bool bCursorMode = false;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Shake")
	TSubclassOf<UCameraShakeBase> HitShakeLight;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Shake")
	TSubclassOf<UCameraShakeBase> HitShakeHeavy;

	// OnUnPossess 시점에 GetPawn()은 이미 null이므로 별도 보관
	UPROPERTY()
	TWeakObjectPtr<USpyHealthComponent> BoundHealthComponent;
};
