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
	TObjectPtr<UAnimMontage> VaultMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vault")
	FName VaultStartName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vault")
	FName VaultEndName;

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
	FVector HandPosVector;
	FVector LandPosVector;
	float Distance;
	float Height;
	float Depth;

	FVaultWallData() {
		Clear();
	}

	void Clear() {
		FrontNormalVector = FVector::ZeroVector;
		HandPosVector = FVector::ZeroVector;
		LandPosVector = FVector::ZeroVector;
		Distance = 0.f;
		Height = 0.f;
		Depth = 0.f;
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
};

USTRUCT(BlueprintType)
struct FClimbWallData {

	GENERATED_BODY()

public:
	FVector NormalVector;
	FVector HitVector;

	FClimbWallData() {
		Clear();
	}

	void Clear() {
		NormalVector = FVector::ZeroVector;
		HitVector = FVector::ZeroVector;
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SKILLPROJECT_API USpyParkourManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USpyParkourManagerComponent();

protected:
	virtual void BeginPlay() override;

public:
	FORCEINLINE FVaultData GetVaultData() const { return VaultData; }
	FORCEINLINE FVaultWallData GetVaultWallData() const { return VaultWallData; }

	FORCEINLINE FClimbData GetClimbData() const { return ClimbData; }
	FORCEINLINE FClimbWallData GetClimbWallData() const { return ClimbWallData; }

public:
	UFUNCTION(BlueprintCallable)
	void TryClimbAction();

public:
	UFUNCTION(BlueprintCallable)
	bool TryVaultAction();

	void SetVaultWallInfo();
	void SetMotionWarping();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour")
	FVaultData VaultData;

	FVaultWallData VaultWallData;
	FOnMontageEnded VaultMontageEndDelegate;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour")
	FClimbData ClimbData;

	FClimbWallData ClimbWallData;
	
};
