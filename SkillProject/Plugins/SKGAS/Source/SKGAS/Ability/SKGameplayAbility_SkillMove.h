// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Ability/SKGameplayAbility.h"

#include "SKGameplayAbility_SkillMove.generated.h"

class UCharacterMovementComponent;
class UCapsuleComponent;

UCLASS()
class SKGAS_API USKGameplayAbility_SkillMove : public USKGameplayAbility
{
	GENERATED_BODY()
	
public:
    USKGameplayAbility_SkillMove();

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

protected:
    UFUNCTION()
    virtual void PlayMontage();

protected:
    //# bActive == true 로 실제 상태를 바꾼 경우에만 false 되돌리기가 수행된다 (아래 bMoveStateEngaged).
    void SetMoveState(bool bActive);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UAnimMontage> AbilityMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsCollide;

    UPROPERTY()
    TObjectPtr<UCharacterMovementComponent> CharacterMovementComponent;

    UPROPERTY()
    TObjectPtr<UCapsuleComponent> CapsuleComponent;

private:
    //# SetMoveState(true) 로 이동 모드·캡슐 충돌을 실제로 바꿨는지 여부.
    //# 켠 주체만 되돌린다 — 스킬이 성립하지 않아 켠 적이 없으면 종료 시 아무것도 되돌리지 않는다.
    //# 파생 클래스가 임의로 조작하지 못하도록 private 으로 두고 SetMoveState 안에서만 갱신한다.
    bool bMoveStateEngaged = false;
};
