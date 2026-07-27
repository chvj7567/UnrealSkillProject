// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Ability/SKGameplayAbility.h"

#include "SpyGA_WallClimb.generated.h"

class UInputMappingContext;
class ASpyCharacter;

UCLASS()
class SKILLPROJECT_API USpyGA_WallClimb : public USKGameplayAbility
{
	GENERATED_BODY()
	
public:
    USpyGA_WallClimb();

protected:
    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled) override;

    virtual void InputPressed(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo) override;

protected:
    UFUNCTION()
    bool TryToggleClimbAction();

    UFUNCTION()
    void StartWallClimb(const FClimbData& InClimbData, const FClimbWallData& InClimbWallData);

    UFUNCTION()
    void EndWallClimb();

protected:
	//# 이 GA 가 카메라 콜리전 억제를 건 캐릭터. 건 주체만 해제한다.
	//# 약참조로 들고 있는 이유 — 사망·파괴 경로의 EndAbility 에서는 ActorInfo 의 Avatar 가
	//# 이미 비어 있을 수 있어 다시 조회하면 해제를 놓치고 카운트가 샌다.
	TWeakObjectPtr<ASpyCharacter> CameraSuppressedCharacter;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputMappingContext> InputMappingContext;
};
