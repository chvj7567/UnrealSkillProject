// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "SpyParkourManagerComponent.generated.h"

class UCharacterMovementComponent;

USTRUCT(BlueprintType)
struct FVaultWallInfo {

	GENERATED_BODY()

public:
	float Distance;
	float Height;
	float Depth;

	FVaultWallInfo() {
		Clear();
	}

	void Clear() {
		Distance = 0.f;
		Height = 0.f;
		Depth = 0.f;
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
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	FORCEINLINE bool IsWallClimbing() const { return bIsWallClimbing; }
	FORCEINLINE FVector GetHitNormalVector() const { return HitNormalVector; }

public:
	void CheckAbleWallClimbing();

public:
	UFUNCTION(BlueprintCallable)
	FVaultWallInfo GetVaultWallInfo();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vault")
	FName VaultStartName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vault")
	FName VaultEndName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vault")
	FVector VaultStartOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vault")
	FVector VaultEndOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vault")
	float VaildDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vault")
	float VaildHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vault")
	float VaildDepth;

private:
	bool bIsWallClimbing;
	FVector HitNormalVector;
	FVaultWallInfo VaultWallInfo;
};
