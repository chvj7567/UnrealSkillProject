// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"

#include "SpyCharacterAnimInstance.generated.h"

class ASpyCharacter;
class UCharacterMovementComponent;

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

	UFUNCTION(BlueprintCallable)
	void AnimNotify_DisableMove(UAnimNotify* Notify);

	UFUNCTION(BlueprintCallable)
	void AnimNotify_AbleMove(UAnimNotify* Notify);

protected:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<ASpyCharacter> Player;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UCharacterMovementComponent> PlayerMovementComponent;

protected:
	UPROPERTY(BlueprintReadOnly)
	float AimPitch;

	UPROPERTY(BlueprintReadOnly)
	FVector Velocity;

	UPROPERTY(BlueprintReadOnly)
	float GroundSpeed;

	UPROPERTY(BlueprintReadOnly)
	bool ShouldMove;

	UPROPERTY(BlueprintReadOnly)
	bool IsCrouching;

	UPROPERTY(BlueprintReadOnly)
	bool IsFalling;

	UPROPERTY(BlueprintReadOnly)
	bool IsDeath;

	UPROPERTY(BlueprintReadOnly)
	bool IsWallClimbing;
};
