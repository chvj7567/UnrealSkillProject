#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "GameplayTagContainer.h"
#include "BTTask_CircleStrafe.generated.h"

class UEnvQuery;
class UBehaviorTreeComponent;

UCLASS()
class SKILLPROJECT_API UBTTask_CircleStrafe : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_CircleStrafe();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// 스트레이프 지속 시간 (초)
	UPROPERTY(EditAnywhere, Category = "Config")
	float StrafeDuration = 3.f;

	UPROPERTY(EditAnywhere, Category = "Config")
	float StrafeWalkSpeed = 200.f;

	UPROPERTY(EditAnywhere, Category = "Config")
	float StrafeAcceptanceRadius = 50.f;

	UPROPERTY(EditAnywhere, Category = "EQS")
	TObjectPtr<UEnvQuery> EQSQuery;

	UPROPERTY(EditAnywhere, Category = "Config")
	FBlackboardKeySelector TargetKey;

	UPROPERTY(EditAnywhere, Category = "Config")
	FBlackboardKeySelector StrafeLeftKey;

	UPROPERTY(EditAnywhere, Category = "Debug")
	float DebugDrawDuration = 2.f;

private:
	void RunEQS(UBehaviorTreeComponent& OwnerComp);
	void OnQueryFinished(TSharedPtr<FEnvQueryResult> Result, TWeakObjectPtr<UBehaviorTreeComponent> WeakOwner);
	void DrawDebugEQSResults(UWorld* World, const TSharedPtr<FEnvQueryResult>& Result) const;
	void SetStrafeSpeed(AAIController* InController, float Speed);
	void FinishStrafe(UBehaviorTreeComponent& OwnerComp, EBTNodeResult::Type Result);

	int32 ActiveQueryId = INDEX_NONE;
	float StartTime = 0.f;
	float OriginalMaxWalkSpeed = 0.f;
	bool bTaskActive = false;
	bool bEQSPending = false;            //# 동시에 여러 쿼리가 날아가지 않도록
	float EQSAccumulator = 0.f;          //# 다음 RunEQS까지의 누적 시간
	FTimerHandle DurationTimerHandle;
};
