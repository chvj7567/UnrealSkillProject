// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "SpyParkourManagerComponent.generated.h"

class UCharacterMovementComponent;

// ============================================================
// FParkourWallBaseData — 벽 감지 결과 구조체들의 공통 기반
// Distance / Height / Depth 측정값을 공유합니다.
// FVaultWallData, FWallData가 이 구조체를 상속합니다.
// ============================================================
USTRUCT(BlueprintType)
struct FParkourWallBaseData
{
	GENERATED_BODY()

	float Distance = 0.f;
	float Height   = 0.f;
	float Depth    = 0.f;

	void ClearBase()
	{
		Distance = 0.f;
		Height   = 0.f;
		Depth    = 0.f;
	}
};

USTRUCT(BlueprintType)
struct FVaultData {

	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector VaultStartOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector VaultEndOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RayInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RayIntervalReapeatCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float VaildDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float VaildHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float VaildDepth;

	FVaultData()
		: VaultStartOffset(FVector::ZeroVector)
		, VaultEndOffset(FVector::ZeroVector)
		, RayInterval(0.0f)
		, RayIntervalReapeatCount(0.0f)
		, VaildDistance(0.0f)
		, VaildHeight(0.0f)
		, VaildDepth(0.0f)
	{
	}

	void Clear()
	{
		VaultStartOffset = FVector::ZeroVector;
		VaultEndOffset = FVector::ZeroVector;
		RayInterval = 0.0f;
		RayIntervalReapeatCount = 0.0f;
		VaildDistance = 0.0f;
		VaildHeight = 0.0f;
		VaildDepth = 0.0f;
	}
};

USTRUCT(BlueprintType)
struct FVaultWallData : public FParkourWallBaseData {

	GENERATED_BODY()

public:
	FVector FrontNormalVector;
	FVector HandLocVector;
	FVector LandLocVector;

	FVaultWallData()
		: FrontNormalVector(FVector::ZeroVector)
		, HandLocVector(FVector::ZeroVector)
		, LandLocVector(FVector::ZeroVector)
	{
	}

	void Clear()
	{
		FrontNormalVector = FVector::ZeroVector;
		HandLocVector     = FVector::ZeroVector;
		LandLocVector     = FVector::ZeroVector;
		ClearBase();
	}
};

USTRUCT(BlueprintType)
struct FWallData : public FParkourWallBaseData {

	GENERATED_BODY()

public:
	FVector NormalVector;
	FVector HitVector;
	FVector LandVector;

	FWallData()
		: NormalVector(FVector::ZeroVector)
		, HitVector(FVector::ZeroVector)
		, LandVector(FVector::ZeroVector)
	{
	}

	void Clear()
	{
		NormalVector = FVector::ZeroVector;
		HitVector    = FVector::ZeroVector;
		LandVector   = FVector::ZeroVector;
		ClearBase();
	}
};

USTRUCT(BlueprintType)
struct FClimbWallData {

	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere)
	FVector NormalVector;
	UPROPERTY(VisibleAnywhere)
	FVector HitVector;

	FClimbWallData()
		: NormalVector(FVector::ZeroVector)
		, HitVector(FVector::ZeroVector)
	{
	}

	void Clear() {
		NormalVector = FVector::ZeroVector;
		HitVector = FVector::ZeroVector;
	}
};

USTRUCT(BlueprintType)
struct FClimbData {

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
		: DistanceOffset(0.0f)
		, HandOffset(0.0f)
		, FootOffset(0.0f)
		, Speed(0.0f)
		, CheckHangUpHeight(0.0f)
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
		: StartLoc(FVector::ZeroVector)
		, StartRot(FRotator::ZeroRotator)
		, EndLoc(FVector::ZeroVector)
		, EndRot(FRotator::ZeroRotator)
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

USTRUCT(BlueprintType)
struct FHangUpData {

	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector HangUpStartOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector HangUpEndOffset;

	FHangUpData()
		: HangUpStartOffset(FVector::ZeroVector)
		, HangUpEndOffset(FVector::ZeroVector)
	{
	}

	void Clear()
	{
		HangUpStartOffset = FVector::ZeroVector;
		HangUpEndOffset = FVector::ZeroVector;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSyncMotionWarpingDataDelegate, FMotionWarpingData, InVaultData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSyncClilmbDataDelegate, const FClimbData&, InClimbData, const FClimbWallData&, InClimbWallData);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SKILLPROJECT_API USpyParkourManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USpyParkourManagerComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	FORCEINLINE FVaultData GetVaultData() const { return VaultData; }
	FORCEINLINE FVaultWallData GetVaultWallData() const { return VaultWallData; }

	FORCEINLINE FClimbData GetClimbData() const { return ClimbData; }

public:
	UFUNCTION()
	void OnRep_FreeMoveMode();

	UFUNCTION(BlueprintCallable)
	bool TryToggleClimbAction();

	UFUNCTION()
	void OnRep_ClimbWallData();

	UFUNCTION(BlueprintCallable)
	bool CanVaultAction();

	UFUNCTION()
	void OnRep_VaultMotionWarpingData();

	UFUNCTION()
	void OnRep_HangUpMotionWarpingData();

	void SetVaultWallData();

	void SetVaultMotionWarpingData();

	bool SetValidWallData(float InValidDistance, float InValidHeight, float InValidDepth, int InRayInterval, int InRayIntervalReapeatCount);

	void SetHangUpMotionWarpingData(const FVector& HitVector);

public: //# 공용 사용
	UPROPERTY(ReplicatedUsing = OnRep_FreeMoveMode)
	bool bFreeMoveMode;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FWallData WallData;

public: //# 델리게이트
	FSyncMotionWarpingDataDelegate OnVaultMotionWarpingData;
	FSyncClilmbDataDelegate OnClimbData;
	FSyncMotionWarpingDataDelegate OnHangUpMotionWarpingData;

protected: //# 벽 넘기
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVaultData VaultData;

	FVaultWallData VaultWallData;

	UPROPERTY(ReplicatedUsing = OnRep_VaultMotionWarpingData)
	FMotionWarpingData VaultMotionWarpingData;

protected: //# 벽 넘기
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FClimbData ClimbData;

	UPROPERTY(ReplicatedUsing = OnRep_ClimbWallData)
	FClimbWallData ClimbWallData;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FHangUpData HangUpData;

	UPROPERTY(ReplicatedUsing = OnRep_HangUpMotionWarpingData)
	FMotionWarpingData HangUpMotionWarpingData;
};
