// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Character/CommonInterface.Character.h"

#include "SpyCharacterAnimInstance.generated.h"

class ASpyCharacter;
class USpyCharacterMovementComponent;

UCLASS()
class SKILLPROJECT_API USpyCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeBeginPlay() override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

public:
	UFUNCTION(BlueprintCallable)
	void AnimNotify_AttackHit(UAnimNotify* Notify);

protected:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<ASpyCharacter> Player;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<USpyCharacterMovementComponent> PlayerMovementComponent;

	//# NativeBeginPlay 에서 캐싱 시도 + 널이면 Tick 에서 재해결 (루트 조립 이전 타이밍 대비, cpp-style §8·§13)
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
