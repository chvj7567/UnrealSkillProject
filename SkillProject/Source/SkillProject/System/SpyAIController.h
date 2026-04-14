// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularAIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "System/SpyTeamAgentInterface.h"

#include "SpyAIController.generated.h"

class USpyAbilitySystemComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UAISenseConfig_Damage;
class USpyAIConfig;

UCLASS()
class SKILLPROJECT_API ASpyAIController : public AModularAIController
{
	GENERATED_BODY()
	
public:
	ASpyAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void Tick(float DeltaSeconds) override;

public:
	//~AController interface
	virtual void InitPlayerState() override;
	virtual void CleanupPlayerState() override;
	virtual void OnRep_PlayerState() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	//~End of AController interface

	//~IGenericTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	//~End of IGenericTeamAgentInterface interface

public:
	void SetBehaviorTree(UBehaviorTree* InBehaviorTreeAsset);

protected:
	void BroadcastOnPlayerStateChanged();
	
	virtual void OnPlayerStateChanged();

	USpyAbilitySystemComponent* GetSpyAbilitySystemComponent() const;

protected:
	UPROPERTY(VisibleAnywhere)
	UAIPerceptionComponent* AIPerceptionComponent;

	UPROPERTY()
	UAISenseConfig_Sight* SightConfig;

	UPROPERTY()
	UAISenseConfig_Hearing* HearingConfig;

	UPROPERTY()
	UAISenseConfig_Damage* DamageConfig;

	UFUNCTION()
	void OnTargetDetected(AActor* Actor, FAIStimulus Stimulus);

private:
	UPROPERTY()
	TObjectPtr<APlayerState> LastSeenPlayerState;

	UPROPERTY()
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	UPROPERTY(EditAnywhere)
	FGenericTeamId TeamID;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<USpyAIConfig> AIConfig;
};
