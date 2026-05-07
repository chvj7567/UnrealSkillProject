#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_StrafeDirection.generated.h"

UCLASS()
class SKILLPROJECT_API UEnvQueryContext_StrafeDirection : public UEnvQueryContext
{
    GENERATED_BODY()

public:
    virtual void ProvideContext(FEnvQueryInstance& QueryInstance,
                                FEnvQueryContextData& ContextData) const override;

    //# 이 키 이름은 BTTask_CircleStrafe의 TargetKey, StrafeLeftKey와 반드시 일치해야 합니다.
    //# 에디터에서 EQS 에셋의 Dot 테스트 컨텍스트로 이 클래스를 선택 시 기본값 확인 필수.
    UPROPERTY(EditDefaultsOnly, Category = "EQS")
    FName TargetKeyName = TEXT("Target");

    //# 이 키 이름은 BTTask_CircleStrafe의 TargetKey, StrafeLeftKey와 반드시 일치해야 합니다.
    //# 에디터에서 EQS 에셋의 Dot 테스트 컨텍스트로 이 클래스를 선택 시 기본값 확인 필수.
    UPROPERTY(EditDefaultsOnly, Category = "EQS")
    FName StrafeLeftKeyName = TEXT("bStrafeLeft");

    UPROPERTY(EditDefaultsOnly, Category = "EQS")
    float ContextOffset = 100.f;
};
