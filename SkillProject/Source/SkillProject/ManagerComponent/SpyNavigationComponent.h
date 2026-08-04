// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "SpyNavigationComponent.generated.h"

class USpyMissionComponent;
class USplineComponent;
class USplineMeshComponent;
class UStaticMesh;

//# 활성 미션의 목표 지점까지 바닥 글로우 라인으로 안내하는 로컬 클라이언트 전용 연출 컴포넌트.
//# 서버/타 플레이어에 레플리케이트하지 않는다 — 소유 폰이 로컬 컨트롤일 때만 동작한다.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SKILLPROJECT_API USpyNavigationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USpyNavigationComponent();

	UFUNCTION(BlueprintPure)
	bool IsPathActive() const
	{
		return bPathActive;
	}

	UFUNCTION(BlueprintPure)
	FVector GetCurrentTargetLocation() const
	{
		return CurrentTargetLocation;
	}

	//# SpyMainHUD::TryBindMissionComponent 와 동일한 재시도 바인딩 흐름의 실제 바인딩 단계.
	//# 테스트에서도 컨트롤러/PlayerState 체인 없이 직접 호출한다(cpp-style §8 탐색 지양의
	//# 대안인 "명시적 주입"에 해당 — 프로덕션에서는 AutoDiscoverAndBindMissionComponent 가 호출)
	void BindMissionComponent(USpyMissionComponent* InMissionComponent);
	void UnbindMissionComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool AutoDiscoverAndBindMissionComponent();

	UFUNCTION()
	void HandleMissionAccepted(USpyMissionComponent* MissionComponent, int32 MissionIndex);

	UFUNCTION()
	void HandleMissionCompleted(USpyMissionComponent* MissionComponent, int32 CompletedIndex);

	UFUNCTION()
	void HandleAllMissionsCompleted(USpyMissionComponent* MissionComponent);

	void StartPathTo(const FVector& InTargetLocation);
	void StopPath();

	void RecomputePath();
	void ApplyPathPoints(const TArray<FVector>& InPathPoints);
	void EnsureSegmentPoolSize(int32 InRequiredCount);
	void HideVisual();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	float UpdateIntervalSeconds = 0.75f;

	UPROPERTY(Transient)
	TObjectPtr<USpyMissionComponent> BoundMissionComponent;

	FTimerHandle BindRetryTimerHandle;
	FTimerHandle RepathTimerHandle;

	FVector CurrentTargetLocation = FVector::ZeroVector;
	bool bPathActive = false;

	UPROPERTY(Transient)
	TObjectPtr<USplineComponent> PathSpline;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USplineMeshComponent>> PathSegmentPool;

	//# §4-3 가시성 히스테리시스의 "이전 프레임 상태" — SpyNavPathMath::EvaluateHysteresisVisibility 가
	//# 순수 함수로 남도록 이 컴포넌트가 상태를 들고 있는다(Task 3 확장 참고)
	bool bPathVisible = false;

	//# design §7-5 항목2(mission-ground-navigation.md) — §4 합성 규칙 확정값을 그대로 기본값으로 노출
	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	float StartOffsetDistanceCm = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	float ArrivalHideDistanceCm = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	float ArrivalReshowDistanceCm = 400.f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	float GroundZOffsetCm = 3.f;

	//# USplineMeshComponent 는 StaticMesh 가 없으면 아무것도 그리지 않는다(머티리얼과 별개) —
	//# 실제 "길" 형태 메시는 아트 작업 범위 밖이라 에디터에서 임시 지오메트리를 지정할 수 있게 노출
	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	TObjectPtr<UStaticMesh> SegmentMesh;
};
