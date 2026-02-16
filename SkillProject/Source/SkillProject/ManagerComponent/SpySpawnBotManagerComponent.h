// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AIController.h"
#include "SpySpawnBotManagerComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SKILLPROJECT_API USpySpawnBotManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USpySpawnBotManagerComponent();

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void SpawnOneBot();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void RemoveOneBot();

	UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly)
	void ServerCreateBots();

protected:
	UPROPERTY(EditAnywhere)
	TSubclassOf<AAIController> BotControllerClass;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	UPROPERTY(EditAnywhere)
	FString BotNamePrefix = TEXT("Bot_");

	UPROPERTY()
	TArray<TObjectPtr<AAIController>> SpawnedBotList;

private:
	FString CreateBotName(int32 ID);
};
