// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Util/DefineEnum.h"
#include "ManagerComponent/CommonInterface.Manager.h"
#include "ModularPlayerController.h"
#include "SpyPlayerController.generated.h"

class USpyAbilitySystemComponent;
class UCameraShakeBase;

UCLASS()
class SKILLPROJECT_API ASpyPlayerController : public AModularPlayerController
{
	GENERATED_BODY()

public:
	ASpyPlayerController();

protected:
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

	UFUNCTION(Client, Reliable)
	void Client_TriggerShake(bool bCritical, bool bFromReceivedHit);

	//# RPC 우회용 — 로컬 컨트롤러는 ProcessEvent 거치지 않고 직접 호출
	void TriggerShakeLocal(bool bCritical, bool bFromReceivedHit);

protected:
	//# OnPossess/AcknowledgePossession 에서 캐싱 시도 + IsValid 로 널/파괴 판정 후 UpdateRotation 에서 재해결
	UPROPERTY(Transient)
	TScriptInterface<ISpyTargetProvider> TargetingComp;

	bool bCursorMode = false;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Shake")
	TSubclassOf<UCameraShakeBase> HitShakeLight;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Shake")
	TSubclassOf<UCameraShakeBase> HitShakeHeavy;
};
