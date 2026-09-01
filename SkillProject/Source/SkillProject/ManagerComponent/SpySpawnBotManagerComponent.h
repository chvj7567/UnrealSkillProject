// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AIController.h"
#include "SpySpawnBotManagerComponent.generated.h"

//# 봇 하나의 스폰 원본 배치 — 재활용 리스폰 시 이 트랜스폼으로 되돌린다.
USTRUCT()
struct FSpySpawnedBotInfo
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AAIController> Controller;

	UPROPERTY()
	FTransform SpawnTransform;
};

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
	void SpawnOneBot(FVector InLocation, FRotator InRotator);

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
	TArray<FSpySpawnedBotInfo> SpawnedBotList;

private:
	FString CreateBotName(int32 ID);

	//# 서버 전용 — 봇 사망 신호를 구독해 리스폰 타이밍을 스케줄링한다. Cue(디졸브 연출) 트리거와는
	//# 독립적으로 같은 OnDeath 델리게이트를 각자 구독한다 (cpp-style §8, 이벤트 기반 분리).
	UFUNCTION()
	void HandleBotDeath(AActor* InOwningActor, AActor* InCauserActor);

	void RespawnBot(TWeakObjectPtr<AAIController> InController);

	UPROPERTY()
	TMap<TObjectPtr<AAIController>, FTimerHandle> RespawnTimers;
};
