#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "SpyMovementConfig.generated.h"

UCLASS()
class SKILLPROJECT_API USpyMovementConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	//# Climbing ray cast
	UPROPERTY(EditDefaultsOnly, Category = "Climbing|RayCast")
	float ClimbHangUpRayForwardOffset = 50.f;

	UPROPERTY(EditDefaultsOnly, Category = "Climbing|RayCast")
	float ClimbHangUpRayUpOffset = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Climbing|RayCast")
	float ClimbHangUpRayDownDistance = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Climbing|RayCast")
	float HangUpCheckRayForwardOffset = 100.f;

	//# Climbing movement mode name
	UPROPERTY(EditDefaultsOnly, Category = "Climbing")
	FName ClimbingMovementModeName = TEXT("Custom");

	//# IK Bone names
	UPROPERTY(EditDefaultsOnly, Category = "Climbing|IK")
	FName IKBoneHandLeft = TEXT("hand_l");

	UPROPERTY(EditDefaultsOnly, Category = "Climbing|IK")
	FName IKBoneHandRight = TEXT("hand_r");

	UPROPERTY(EditDefaultsOnly, Category = "Climbing|IK")
	FName IKBoneFootLeft = TEXT("foot_l");

	UPROPERTY(EditDefaultsOnly, Category = "Climbing|IK")
	FName IKBoneFootRight = TEXT("foot_r");

	//# IK Curve names
	UPROPERTY(EditDefaultsOnly, Category = "Climbing|IK")
	FName IKCurveHandLeft = TEXT("Hand_L_IK_Weight");

	UPROPERTY(EditDefaultsOnly, Category = "Climbing|IK")
	FName IKCurveHandRight = TEXT("Hand_R_IK_Weight");

	UPROPERTY(EditDefaultsOnly, Category = "Climbing|IK")
	FName IKCurveFootLeft = TEXT("Foot_L_IK_Weight");

	UPROPERTY(EditDefaultsOnly, Category = "Climbing|IK")
	FName IKCurveFootRight = TEXT("Foot_R_IK_Weight");

	//# Grapple Hook
	UPROPERTY(EditDefaultsOnly, Category = "GrappleHook")
	float GrappleMaxRange = 3000.f;

	UPROPERTY(EditDefaultsOnly, Category = "GrappleHook")
	float GrappleArrivalThreshold = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "GrappleHook")
	float GrappleLaunchArcZScale = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "GrappleHook")
	float GrappleFlightTime = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category = "GrappleHook")
	float GrapplePromptRange = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "GrappleHook")
	float GrappleTargetingScreenRadius = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "GrappleHook")
	float GrapplePullSpeed = 900.f;
};
