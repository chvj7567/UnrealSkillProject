// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularAIController.h"
#include "SpyAIController.generated.h"

class USpyAbilitySystemComponent;

UCLASS()
class SKILLPROJECT_API ASpyAIController : public AModularAIController
{
	GENERATED_BODY()
	
public:
	ASpyAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~AController interface
	virtual void InitPlayerState() override;
	virtual void CleanupPlayerState() override;
	virtual void OnRep_PlayerState() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	//~End of AController interface

public:
	void SetBehaviorTree(UBehaviorTree* InBehaviorTreeAsset);

protected:
	void BroadcastOnPlayerStateChanged();
	
	virtual void OnPlayerStateChanged();

	USpyAbilitySystemComponent* GetSpyAbilitySystemComponent() const;

private:
	UPROPERTY()
	TObjectPtr<APlayerState> LastSeenPlayerState;

	UPROPERTY()
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;
};
