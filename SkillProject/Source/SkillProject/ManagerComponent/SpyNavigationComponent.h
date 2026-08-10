// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "SpyNavigationComponent.generated.h"

class UMaterialInstanceDynamic;
class USpyMissionComponent;
class USpyMissionTargetRegistrySubsystem;
class USplineComponent;
class USplineMeshComponent;
class UStaticMesh;
class UPrimitiveComponent;

//# 활성 미션의 목표 지점까지 바닥 글로우 라인으로 안내하는 로컬 클라이언트 전용 연출 컴포넌트.
//# 서버/타 플레이어에 레플리케이트하지 않는다 — 소유 폰이 로컬 컨트롤일 때만 동작한다.
UCLASS(Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
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

	//# design 2026-08-10 §6 테스트/디버그 훅 — 현재 라인이 실제로 그려지고 있는지
	UFUNCTION(BlueprintPure)
	bool IsPathVisible() const
	{
		return bPathVisible;
	}

	//# design 2026-08-10 §6 테스트 훅 — 현재 타겟이 트리거 볼륨을 노출해 구독 중인지
	UFUNCTION(BlueprintPure)
	bool IsHideTriggerBound() const
	{
		return BoundHideTrigger.IsValid();
	}

	UFUNCTION(BlueprintPure)
	FVector GetCurrentTargetLocation() const
	{
		return CurrentTargetLocation;
	}

	//# design 2026-08-10 §6 테스트 주입 지점(§5-7 과 동일 목적) — 실제 오버랩 델리게이트와 동일한
	//# 진입점이라 테스트가 물리 없이 직접 호출한다. OtherActor 가 소유 폰이 아니면 무시한다.
	void NotifyHideTriggerEntered(AActor* OtherActor);
	void NotifyHideTriggerExited(AActor* OtherActor);

	//# SpyMainHUD::TryBindMissionComponent 와 동일한 재시도 바인딩 흐름의 실제 바인딩 단계.
	//# 테스트에서도 컨트롤러/PlayerState 체인 없이 직접 호출한다(cpp-style §8 탐색 지양의
	//# 대안인 "명시적 주입"에 해당 — 프로덕션에서는 AutoDiscoverAndBindMissionComponent 가 호출)
	void BindMissionComponent(USpyMissionComponent* InMissionComponent);
	void UnbindMissionComponent();

	//# §5-7 테스트 주입 지점 — BindMissionComponent 와 같은 목적. World 없이 만든 테스트
	//# 픽스처가 실제 UWorldSubsystem 조회 없이 좌표 조회를 검증할 수 있게 한다.
	void SetMissionTargetRegistry(USpyMissionTargetRegistrySubsystem* InRegistry) { OverrideTargetRegistry = InRegistry; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool AutoDiscoverAndBindMissionComponent();

	//# design mission-ground-navigation.md §5-8 — OnMissionAccepted/OnMissionCompleted/
	//# OnAllMissionsCompleted 3개 대신 OnMissionProgressChanged 1개만 구독한다.
	UFUNCTION()
	void HandleMissionProgressChanged(USpyMissionComponent* MissionComponent, int32 MissionIndex, int32 Count, int32 TargetCount);

	void StartPathTo(const FVector& InTargetLocation, AActor* InTargetActor);
	void StopPath();

	void RecomputePath();
	void ApplyPathPoints(const TArray<FVector>& InPathPoints);
	void EnsureSegmentPoolSize(int32 InRequiredCount);
	void HideVisual();

	//# design 2026-08-10 §6 — 타겟이 ISpyMissionTargetHideVolume 를 구현하고 트리거를 노출하면
	//# 구독하고, 그렇지 않으면 아무 것도 하지 않는다(거리 히스테리시스 폴백).
	void BindHideTrigger(AActor* TargetActor);
	void UnbindHideTrigger();

	UFUNCTION()
	void HandleHideTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
										UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleHideTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
									  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	//# design §5-5·§5-6 — 레지스트리 조회 1회 시도. 실패하면 0.2초 간격 재시도 타이머를 건다.
	USpyMissionTargetRegistrySubsystem* GetMissionTargetRegistry();
	void TryResolveTarget();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	float UpdateIntervalSeconds = 0.75f;

	UPROPERTY(Transient)
	TObjectPtr<USpyMissionComponent> BoundMissionComponent;

	//# §5-7 테스트 주입 지점 — 설정돼 있으면 GetWorld()->GetSubsystem<>() 대신 이걸 쓴다
	UPROPERTY(Transient)
	TObjectPtr<USpyMissionTargetRegistrySubsystem> OverrideTargetRegistry;

	FTimerHandle BindRetryTimerHandle;
	FTimerHandle RepathTimerHandle;

	//# HandleMissionProgressChanged 가 채우는 대기 중인 조회 키(§5-5) — 재시도 타이머가 참조한다.
	//# bPendingIsDialogue==true 는 "NPCId 키 공간으로 조회"를 뜻한다 — Dialogue 타입뿐 아니라
	//# 미수락 상태(§5-2-1, MissionType 무관)에서도 이 경로를 재사용한다.
	bool bPendingIsDialogue = false;
	int32 PendingNPCId = 0;
	FGameplayTag PendingMatchTag;

	FTimerHandle TargetRetryTimerHandle;
	int32 TargetRetryCount = 0;

	//# design §5-8 과잉 재계산 가드 — 직전 호출의 (MissionIndex, bAccepted) 와 동일하면
	//# Count 만 바뀐 진행 이벤트로 보고 레지스트리 재질의를 생략한다
	int32 LastResolvedMissionIndex = INDEX_NONE;
	bool bLastResolvedAccepted = false;

	//# design §5-6 — 0.2초 간격 최대 약 10회(≈2초) 재시도 후 포기
	static constexpr float TargetRetryIntervalSeconds = 0.2f;
	static constexpr int32 TargetRetryMaxCount = 10;

	//# Dialogue 조회 실패는 데이터 버그를 뜻하므로 경고 로그 1회(bWarnedMissingConfig 와 동일 패턴, §5-6)
	bool bWarnedMissingNPCLocation = false;

	FVector CurrentTargetLocation = FVector::ZeroVector;
	bool bPathActive = false;

	//# design 2026-08-10 §6 — 현재 타겟이 노출한 트리거 컴포넌트(없으면 invalid). 타겟 전환/StopPath 시 해제된다.
	UPROPERTY(Transient)
	TWeakObjectPtr<UPrimitiveComponent> BoundHideTrigger;

	bool bInsideHideTrigger = false;

	UPROPERTY(Transient)
	TObjectPtr<USplineComponent> PathSpline;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USplineMeshComponent>> PathSegmentPool;

	//# PathSegmentPool 과 같은 인덱스로 관리되는 다이나믹 머티리얼 인스턴스 풀 —
	//# 세그먼트별 SegmentWorldLength 파라미터를 세팅하기 위함
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> PathSegmentMaterialPool;

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

	//# PathSpline(Catmull-Rom) 을 따라 렌더 세그먼트용 점을 재샘플링하는 간격 — 코너를
	//# 곡선으로 그리기 위함(원본 NavMesh 경로점을 직선으로만 잇지 않는다)
	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	float CurveSampleIntervalCm = 60.f;

	//# CurveSampleIntervalCm 하한 — 디자이너가 극단적으로 작은 값을 넣어도 재샘플링 루프가
	//# 무한에 가깝게 돌지 않게 막는다.
	static constexpr float MinCurveSampleIntervalCm = 10.f;

	//# 매우 긴 경로에서도 세그먼트(=USplineMeshComponent+다이나믹 머티리얼) 개수가 폭증하지
	//# 않게 하는 상한 — 초과하면 간격을 늘려 상한 이하로 맞춘다(첫 프레임 동기 생성 히치 방지).
	static constexpr int32 MaxCurveSampleCount = 120;

	//# 마지막 재샘플 지점과 스플라인 끝점 사이 최소 거리 — 이보다 가까우면 거의 0-length인
	//# 꼬리 세그먼트(SetStartAndEnd 에 거의 0인 탄젠트)가 생겨 렌더/머티리얼이 깨진다.
	static constexpr float MinTailSegmentCm = 1.f;

	//# USplineMeshComponent 는 StaticMesh 가 없으면 아무것도 그리지 않는다(머티리얼과 별개) —
	//# 실제 "길" 형태 메시는 아트 작업 범위 밖이라 에디터에서 임시 지오메트리를 지정할 수 있게 노출
	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	TObjectPtr<UStaticMesh> SegmentMesh;
};
