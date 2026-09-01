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
	//#~AController interface
	virtual void InitPlayerState() override;
	virtual void CleanupPlayerState() override;
	virtual void OnRep_PlayerState() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	//#~End of AController interface

	//#~IGenericTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	//#~End of IGenericTeamAgentInterface interface

public:
	void SetBehaviorTree(UBehaviorTree* InBehaviorTreeAsset);

	//# AIConfig 가 유효하면 그 값, 아니면 DefaultDissolveDurationSeconds 를 반환.
	float GetDissolveDurationSeconds() const;

	//# 리스폰 시 재활용된 컨트롤러의 BT/Perception 을 재시작한다. OnPossess 는 최초 1회만 호출되므로 별도 API로 노출.
	void RestartAILogic();

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

	UFUNCTION()
	void OnTrackedTargetDestroyed(AActor* DestroyedActor);

	//# 현재 BB.TargetActor가 죽었거나 비었을 때 가장 가까운 살아있는 적으로 교체
	void RefreshBlackboardTarget();

	bool IsHostileAndAlive(AActor* InActor) const;
	APawn* ResolvePawnFromActor(AActor* InActor) const;

private:
	float TargetRefreshAccumulator = 0.f;
	static constexpr float TargetRefreshInterval = 0.3f;
	static constexpr float DefaultDissolveDurationSeconds = 2.0f;

	//# 폰 라이프사이클 추적 — destroy 감지용
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> TrackedTargetForDestroyDetection;

	UPROPERTY()
	TObjectPtr<APlayerState> LastSeenPlayerState;

	UPROPERTY()
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<USpyAIConfig> AIConfig;
};
