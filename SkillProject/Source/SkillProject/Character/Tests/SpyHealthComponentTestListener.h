// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "SpyHealthComponentTestListener.generated.h"

//# USpyHealthComponent::OnDeath 발화 횟수를 기록하는 테스트 전용 리스너.
//# 동적 멀티캐스트 델리게이트는 UFUNCTION 바인딩 대상이 필요해 순수 람다로 검증할 수 없다
//# (System/Tests/SpyMissionComponentTestListener.h 와 동일한 이유).
UCLASS()
class USpyHealthComponentTestListener : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void HandleDeath(AActor* InOwningActor, AActor* InCauserActor);

	int32 DeathCallCount = 0;

	TWeakObjectPtr<AActor> LastOwningActor;
	TWeakObjectPtr<AActor> LastCauserActor;
};
