// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ManagerComponent/CommonInterface.Manager.h"

#include "SpyCharacterAnimInstance.generated.h"

class ASpyCharacter;
class USpyCharacterMovementComponent;

UCLASS()
class SKILLPROJECT_API USpyCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeBeginPlay() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

public:
	UFUNCTION(BlueprintCallable)
	void AnimNotify_AttackHit(UAnimNotify* Notify);

protected:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<ASpyCharacter> Player;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<USpyCharacterMovementComponent> PlayerMovementComponent;

	//# NativeBeginPlay 에서 캐싱 시도 + 널/파괴 시 NativeUpdateAnimation(게임 스레드) 에서 재해결.
	//# 오브젝트 참조 쓰기는 게임 스레드 전용 — NativeThreadSafeUpdateAnimation(워커 스레드) 은 읽기만 한다.
	UPROPERTY(Transient)
	TScriptInterface<ISpyTargetProvider> CachedTargetProvider;

protected:
	UPROPERTY(BlueprintReadOnly)
	float AimPitch;

	UPROPERTY(BlueprintReadOnly)
	FVector Velocity;

	UPROPERTY(BlueprintReadOnly)
	float GroundSpeed;

	UPROPERTY(BlueprintReadOnly)
	float Speed;

	UPROPERTY(BlueprintReadOnly)
	bool ShouldMove;

	UPROPERTY(BlueprintReadOnly)
	bool IsCrouching;

	UPROPERTY(BlueprintReadOnly)
	bool IsFalling;

	UPROPERTY(BlueprintReadOnly)
	bool IsDeath;

	UPROPERTY(BlueprintReadOnly)
	bool IsClimbing;

	UPROPERTY(BlueprintReadOnly)
	bool IsTargeting;

	UPROPERTY(BlueprintReadOnly)
	float InputAngle;

	UPROPERTY(BlueprintReadOnly)
	float DirectionAngle;

	UPROPERTY(BlueprintReadOnly)
	FVector CurrentOffsetHL;

	UPROPERTY(BlueprintReadOnly)
	FVector CurrentOffsetHR;

	UPROPERTY(BlueprintReadOnly)
	FVector CurrentOffsetFL;

	UPROPERTY(BlueprintReadOnly)
	FVector CurrentOffsetFR;
};
