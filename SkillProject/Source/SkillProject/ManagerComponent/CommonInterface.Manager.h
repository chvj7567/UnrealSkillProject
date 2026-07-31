// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "CommonInterface.Manager.generated.h"

//# 아래 3개 USTRUCT 과 3개 델리게이트는 SpyParkourManagerComponent.h /
//# SpyGrappleTargetingComponent.h 에서 이동해 왔다 — 인터페이스가 참조하므로 순환을 피한다.

USTRUCT(BlueprintType)
struct FClimbWallData
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere)
	FVector NormalVector;
	UPROPERTY(VisibleAnywhere)
	FVector HitVector;

	FClimbWallData()
		: NormalVector(FVector::ZeroVector), HitVector(FVector::ZeroVector)
	{
	}

	void Clear()
	{
		NormalVector = FVector::ZeroVector;
		HitVector = FVector::ZeroVector;
	}
};

USTRUCT(BlueprintType)
struct FClimbData
{

	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DistanceOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HandOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FootOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CheckHangUpHeight;

	FClimbData()
		: DistanceOffset(0.0f), HandOffset(0.0f), FootOffset(0.0f), Speed(0.0f), CheckHangUpHeight(0.0f)
	{
	}

	void Clear()
	{
		DistanceOffset = 0.0f;
		HandOffset = 0.0f;
		FootOffset = 0.0f;
		Speed = 0.0f;
		CheckHangUpHeight = 0.0f;
	}
};

USTRUCT(BlueprintType)
struct FMotionWarpingData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector StartLoc;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRotator StartRot;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector EndLoc;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRotator EndRot;

	FMotionWarpingData()
		: StartLoc(FVector::ZeroVector), StartRot(FRotator::ZeroRotator), EndLoc(FVector::ZeroVector), EndRot(FRotator::ZeroRotator)
	{
	}

	void Clear()
	{
		StartLoc = FVector::ZeroVector;
		StartRot = FRotator::ZeroRotator;
		EndLoc = FVector::ZeroVector;
		EndRot = FRotator::ZeroRotator;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSyncMotionWarpingDataDelegate, FMotionWarpingData, InVaultData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSyncClilmbDataDelegate, const FClimbData&, InClimbData, const FClimbWallData&, InClimbWallData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGrappleTargetChanged, AActor*, NewTarget);

//# 파쿠르 프로토콜 제공자 — USpyParkourManagerComponent 가 구현한다.
//# GA 는 이 인터페이스로만 접근하고 구체 컴포넌트 타입을 알지 않는다.
UINTERFACE(MinimalAPI)
class USpyParkourHost : public UInterface
{
	GENERATED_BODY()
};

class ISpyParkourHost
{
	GENERATED_BODY()

public:
	virtual bool CanVaultAction() = 0;
	virtual void SetVaultMotionWarpingData() = 0;
	virtual void SetHangUpMotionWarpingData(const FVector& HitVector) = 0;
	virtual bool TryToggleClimbAction() = 0;
	virtual void SetFreeMoveMode(bool bInFreeMoveMode) = 0;

	virtual FSyncMotionWarpingDataDelegate& OnVaultMotionWarping() = 0;
	virtual FSyncMotionWarpingDataDelegate& OnHangUpMotionWarping() = 0;
	virtual FSyncClilmbDataDelegate& OnClimb() = 0;
};

//# 타깃 제공자 — USpyTargetingManagerComponent 가 구현한다.
UINTERFACE(MinimalAPI)
class USpyTargetProvider : public UInterface
{
	GENERATED_BODY()
};

class ISpyTargetProvider
{
	GENERATED_BODY()

public:
	virtual TWeakObjectPtr<AActor> GetTarget() const = 0;
	virtual bool IsTargetValid() const = 0;
	virtual bool FindTarget(float Radius) = 0;
	virtual void SetCurrentTarget(AActor* NewTarget) = 0;
};

//# 그래플 타깃 제공자 — USpyGrappleTargetingComponent 가 구현한다.
UINTERFACE(MinimalAPI)
class USpyGrappleHost : public UInterface
{
	GENERATED_BODY()
};

class ISpyGrappleHost
{
	GENERATED_BODY()

public:
	virtual AActor* GetLocalCachedTarget() const = 0;
	virtual AActor* GetCurrentGrappleTarget() const = 0;
	virtual FOnGrappleTargetChanged& OnGrappleTargetChanged() = 0;
};
