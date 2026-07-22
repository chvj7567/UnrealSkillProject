// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"

#include "SpyCharacterMovementComponent.generated.h"

class USpyMovementConfig;
struct FClimbWallData;

UCLASS()
class SKILLPROJECT_API USpyCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
public:
	USpyCharacterMovementComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;
	virtual void PhysicsRotation(float DeltaTime) override;

public:
	void PhysWallClimb(float DeltaTime, int32 Iterations);
	void StartWallClimb(const FClimbData& InClimbData, const FClimbWallData& InClimbWallData);
	void EndWallClimb();
	bool CanHangUp();

	float GetInputAngleByForward();
	FVector CalculateBoneVectorOffset(FName BoneName, FName CurveName, FVector& CurrentOffsetVar, float DeltaTime, float Offset);

	FVector GetWallClimbSpeed();
	void SetWallClimbInput(FVector2D InputVector);

	bool IsClimbingActive() const;

	UFUNCTION(Server, Unreliable)
    void Server_SetWallClimbInput(FVector2D InputVector);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float InterpSpeed;

	UPROPERTY(Replicated)
	FVector2D SpyInputVector;

	UPROPERTY(Replicated)
	FClimbData ClimbData;

	UPROPERTY(Replicated)
	FClimbWallData ClimbWallData;

public:
	//# 이전 프레임의 IK 도달 지점을 저장 (떨림 방지용 보간 타겟)
	FVector CurrentOffsetHL;
	FVector CurrentOffsetHR;
	FVector CurrentOffsetFL;
	FVector CurrentOffsetFR;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TObjectPtr<USpyMovementConfig> MovementConfig;

private:
	bool bHangUp = false;

	//# StartWallClimb 이 중력을 0 으로 바꾼 상태인지. 이 플래그가 참일 때만 EndWallClimb 이 복구한다.
	//# (등반이 시작된 적 없는데 EndWallClimb 이 불려 CachedGravityScale 로 덮어쓰는 것을 막는다)
	bool bWallClimbing = false;

	//# 등반 전 GravityScale 백업. 캐시 전에 EndWallClimb 이 불려도 중력이 0 이 되지 않도록 기본값을 둔다
	float CachedGravityScale = 1.0f;
};
