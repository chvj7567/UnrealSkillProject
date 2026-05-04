#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_ProjectedPoints.h"
#include "EnvQueryGenerator_ArcAwayFromTarget.generated.h"

//# Querier 위치를 중심으로, 타겟의 반대 방향을 향한 호(arc) 위에 점을 생성한다.
//# 결과 점들은 NavMesh로 투영된 뒤 Item으로 저장된다.
UCLASS(meta = (DisplayName = "Arc Away From Target"))
class SKILLPROJECT_API UEnvQueryGenerator_ArcAwayFromTarget : public UEnvQueryGenerator_ProjectedPoints
{
	GENERATED_BODY()

public:
	UEnvQueryGenerator_ArcAwayFromTarget();

	virtual void GenerateItems(FEnvQueryInstance& QueryInstance) const override;

protected:
	//# Blackboard에서 타겟을 읽을 키 이름
	UPROPERTY(EditDefaultsOnly, Category = "Generator")
	FName TargetKeyName = TEXT("TargetActor");

	//# 호의 전체 각도(도). 60이면 ±30°
	UPROPERTY(EditDefaultsOnly, Category = "Generator", meta = (ClampMin = "1.0", ClampMax = "360.0"))
	float ArcAngleDegrees = 60.f;

	//# Querier로부터의 반지름(거리). 후퇴 회피용이라 짧게(100~150) 권장
	UPROPERTY(EditDefaultsOnly, Category = "Generator", meta = (ClampMin = "0.0"))
	float Radius = 150.f;

	//# 호 위에 생성할 점 개수 (1이면 중앙 한 점, 2 이상이면 균등 분포)
	UPROPERTY(EditDefaultsOnly, Category = "Generator", meta = (ClampMin = "1"))
	int32 NumPoints = 7;
};
