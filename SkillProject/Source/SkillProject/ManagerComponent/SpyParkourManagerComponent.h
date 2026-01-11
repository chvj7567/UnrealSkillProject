// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "SpyParkourManagerComponent.generated.h"

class UCharacterMovementComponent;

USTRUCT(BlueprintType)
struct FVaultData {

	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vault")
	FVector VaultStartOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vault")
	FVector VaultEndOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vault")
	float RayInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vault")
	float RayIntervalReapeatCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vault")
	float VaildDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vault")
	float VaildHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vault")
	float VaildDepth;
};

USTRUCT(BlueprintType)
struct FVaultWallData {

	GENERATED_BODY()

public:
	FVector FrontNormalVector;
	FVector HandLocVector;
	FVector LandLocVector;
	float Distance;
	float Height;
	float Depth;

	FVaultWallData() {
		Clear();
	}

	void Clear() {
		FrontNormalVector = FVector::ZeroVector;
		HandLocVector = FVector::ZeroVector;
		LandLocVector = FVector::ZeroVector;
		Distance = 0.f;
		Height = 0.f;
		Depth = 0.f;
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

	FClimbWallData() {
		Clear();
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climb")
	float DistanceOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climb")
	float Speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climb")
	FClimbWallData WallData;

	void Clear() {
		WallData.Clear();
	}
};

USTRUCT()
struct FVaultMotionWarpingData
{
	GENERATED_BODY()

	FVector StartLoc;
	FRotator StartRot;
	FVector EndLoc;
	FRotator EndRot;
};

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
	UFUNCTION(BlueprintCallable)
	bool TryToggleClimbAction();

	UFUNCTION()
	void OnRep_ClimbData();

public:
	UFUNCTION(BlueprintCallable)
	bool CanVaultAction();

	void SetVaultWallInfo();
	FVaultMotionWarpingData GetVaultMotionWarpingData();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVaultData VaultData;

	FVaultWallData VaultWallData;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_ClimbData, EditAnywhere, BlueprintReadWrite)
	FClimbData ClimbData;
};
