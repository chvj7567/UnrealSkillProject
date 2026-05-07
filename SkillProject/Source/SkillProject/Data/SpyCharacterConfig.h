#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "SpyCharacterConfig.generated.h"

UCLASS()
class SKILLPROJECT_API USpyCharacterConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	//# Collision
	UPROPERTY(EditDefaultsOnly, Category = "Collision")
	float CapsuleRadius = 42.f;

	UPROPERTY(EditDefaultsOnly, Category = "Collision")
	float CapsuleHalfHeight = 96.f;

	//# Movement
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float RotationRateYaw = 1080.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float JumpZVelocity = 700.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float AirControl = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float MaxWalkSpeed = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float MinAnalogWalkSpeed = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float BrakingDecelerationWalking = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float BrakingDecelerationFalling = 1500.f;

	//# Camera
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float CameraArmLength = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "-89.9", ClampMax = "0.0"))
	float ViewPitchMin = -60.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "89.9"))
	float ViewPitchMax = 60.f;

	//# UI
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	FVector HPBarOffset = FVector(0.f, 0.f, 200.f);

	//# Weapon
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float WeaponSpawnDelay = 1.0f;
};
